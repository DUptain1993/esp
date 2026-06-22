#include "pi_terminal_app.h"

#include <Arduino.h>

// Mandated UI includes for the terminal module.
#include "VirtualKeyboard.h"
#include "DisplayTFT.h"
#include "PepeDraw.h"
#include "Pins.h"
#include "SoundUtils.h"

// Core comms pipeline (commands -> protocol -> encryption -> compression -> UART).
#include "comms.h"
#include "commands.h"

// ───────────────────────────────────────────────────────────────────────────
//  Build-time configuration (override with -D flags in platformio.ini)
// ───────────────────────────────────────────────────────────────────────────
#ifndef PI_UART_BAUD
  #define PI_UART_BAUD 115200
#endif
// Dedicated UART for the Pi link. UART0 stays free for Marauder's USB CLI.
#ifndef PI_UART_TX_PIN
  #define PI_UART_TX_PIN 26   // ESP TX -> Pi RX  (free GPIO on CYD 3.5")
#endif
#ifndef PI_UART_RX_PIN
  #define PI_UART_RX_PIN 35   // ESP RX <- Pi TX  (input-only GPIO, fine for RX)
#endif
#ifndef PI_UART_RX_BUFFER
  #define PI_UART_RX_BUFFER 4096  // large so streams survive blocking keyboard
#endif
#ifndef PI_TERM_ROTATION
  #define PI_TERM_ROTATION 0  // portrait 320x480, matches Marauder CYD 3.5"
#endif

// The dedicated UART instance (Serial1: not used by Marauder on this board).
static HardwareSerial &PiSerial = Serial1;

// Our address on the bus; the Pi streams replies back to this id.
#define PI_TERM_DEVICE DEVICE_SELF

// ───────────────────────────────────────────────────────────────────────────
//  Global display instance required by the provided UI headers.
//  Separate TFT_eSPI object sharing the same panel/pins as Marauder. The bus
//  is single-threaded here (the page is modal), so there is no contention.
// ───────────────────────────────────────────────────────────────────────────
DisplayTFT tft;

static inline uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

// ───────────────────────────────────────────────────────────────────────────
//  Terminal scrollback model
//
//  A fixed ring of text lines, fed by the stream handler. ANSI/VT escape
//  sequences are stripped (monochrome display). Populated continuously, even
//  when the page is closed, so the latest output is visible on open.
// ───────────────────────────────────────────────────────────────────────────
static const int TERM_COLS      = 52;
static const int TERM_MAX_LINES = 120;

static String  s_lines[TERM_MAX_LINES];
static int     s_line_count = 0;      // number of committed lines stored
static String  s_cur;                 // line under construction
static volatile bool s_dirty = true;  // body needs repaint

// ANSI escape stripper state (persists across feeds).
static uint8_t s_ansi_state = 0;      // 0=normal, 1=saw ESC, 2=in CSI

static bool s_link_ok = false;

static void term_push_line(const String &l) {
    if (s_line_count < TERM_MAX_LINES) {
        s_lines[s_line_count++] = l;
    } else {
        for (int i = 1; i < TERM_MAX_LINES; i++) s_lines[i - 1] = s_lines[i];
        s_lines[TERM_MAX_LINES - 1] = l;
    }
}

static void term_newline() {
    term_push_line(s_cur);
    s_cur = "";
}

static void term_putc(char c) {
    if ((int)s_cur.length() >= TERM_COLS) term_newline();
    s_cur += c;
}

static void term_clear() {
    s_line_count = 0;
    s_cur = "";
    s_dirty = true;
}

// Feed raw streamed bytes (post-decrypt/decompress) through the VT stripper.
static void term_feed(const uint8_t *data, int len) {
    for (int i = 0; i < len; i++) {
        char c = (char)data[i];
        switch (s_ansi_state) {
            case 0:
                if (c == 0x1B) { s_ansi_state = 1; }
                else if (c == '\n') { term_newline(); }
                else if (c == '\r') { /* ignore CR */ }
                else if (c == '\t') { for (int k = 0; k < 4; k++) term_putc(' '); }
                else if (c == 0x08) { if (s_cur.length()) s_cur.remove(s_cur.length() - 1); }
                else if ((uint8_t)c >= 0x20 && (uint8_t)c < 0x7F) { term_putc(c); }
                break;
            case 1: // after ESC
                s_ansi_state = (c == '[') ? 2 : 0;
                break;
            case 2: // inside CSI: consume until final byte 0x40..0x7E
                if (c >= 0x40 && c <= 0x7E) s_ansi_state = 0;
                break;
        }
    }
    s_dirty = true;
}

// ───────────────────────────────────────────────────────────────────────────
//  Message handler — runs inside pi_terminal_pump() (single-threaded)
// ───────────────────────────────────────────────────────────────────────────
static void link_handler(uint8_t channel, uint8_t device_id,
                         const uint8_t *msg, uint8_t len) {
    if (len == 0) return;
    uint8_t cmd = msg[0];

    if (channel == CH_CONTROL) {
        switch (cmd) {
            case CMD_HANDSHAKE:
                if (len >= 5) {
                    // Re-seed both cipher streams to the peer's nonce, then ACK
                    // with PONG so the Pi marks the link established.
                    comms_set_nonce(le32(msg + 1));
                    comms_send_cmd(CH_CONTROL, device_id, CMD_PONG);
                    s_link_ok = true;
                }
                break;
            case CMD_PING:
                comms_send_cmd(CH_CONTROL, device_id, CMD_PONG);
                break;
            case CMD_PONG:
                s_link_ok = true;
                break;
            default:
                break;
        }
    } else if (channel == CH_STREAM) {
        switch (cmd) {
            case STR_DATA:  term_feed(msg + 1, (int)len - 1); break;
            case STR_CLEAR: term_clear();                     break;
            default: break;
        }
    }
}

// ───────────────────────────────────────────────────────────────────────────
//  TX helpers
// ───────────────────────────────────────────────────────────────────────────
static void link_send_command(const String &cmd) {
    if (cmd.length() == 0) return;
    uint8_t buf[1 + 200];
    int n = cmd.length();
    if (n > 200) n = 200;
    buf[0] = CMD_EXECUTE;
    memcpy(buf + 1, cmd.c_str(), n);
    comms_send(CH_CONTROL, PI_TERM_DEVICE, buf, (uint8_t)(1 + n), true, true);
}

// ───────────────────────────────────────────────────────────────────────────
//  Public API
// ───────────────────────────────────────────────────────────────────────────
void pi_terminal_init() {
    static bool inited = false;
    if (inited) return;
    inited = true;

    // Bring up the comms core (sets parser/crypto). It begins the port on the
    // default pins; we immediately re-open on our dedicated pins with a larger
    // RX buffer so bursts survive while the modal keyboard is blocking.
    comms_init(&PiSerial, PI_UART_BAUD);
    PiSerial.end();
    PiSerial.setRxBufferSize(PI_UART_RX_BUFFER);
    PiSerial.begin(PI_UART_BAUD, SERIAL_8N1, PI_UART_RX_PIN, PI_UART_TX_PIN);

    comms_set_handler(link_handler);
    // Match the Pi (obfuscation on) so cipher streams stay in lock-step.
    comms_set_obfuscation(true);

    s_cur.reserve(TERM_COLS + 1);
}

void pi_terminal_pump() {
    comms_task();   // non-blocking drain/parse/dispatch + background noise
}

// ── UI geometry ──────────────────────────────────────────────────────────
static const int SCR_W       = 320;
static const int HDR_H       = 26;
static const int BODY_Y0     = HDR_H + 2;
static const int ROW_H       = 11;
static const int FOOT_H      = 36;
static const int FOOT_Y      = 480 - FOOT_H;
static const int BODY_Y1     = FOOT_Y - 2;
static const int VIS_ROWS    = (BODY_Y1 - BODY_Y0) / ROW_H;

static void draw_header() {
    tft.fillRect(0, 0, SCR_W, HDR_H, UI_BG);
    drawStringCustom(6, 6, "PI TERMINAL", UI_MAIN, 1);
    String st = s_link_ok ? "LINK OK" : "NO LINK";
    uint16_t c = s_link_ok ? TFT_GREEN : UI_ACCENT;
    drawStringRight(SCR_W - 6, 6, st, c, 1, FONT_SMALL);
    tft.drawFastHLine(0, HDR_H, SCR_W, UI_ACCENT);
}

static void draw_footer() {
    tft.fillRect(0, FOOT_Y - 1, SCR_W, FOOT_H + 1, UI_BG);
    tft.drawFastHLine(0, FOOT_Y - 1, SCR_W, UI_ACCENT);
    // [ TYPE CMD ]  left half
    tft.drawRect(6, FOOT_Y + 3, 150, FOOT_H - 8, UI_MAIN);
    drawStringCustom(40, FOOT_Y + 12, "TYPE CMD", UI_MAIN, 1);
    // [ EXIT ]  right half
    tft.drawRect(164, FOOT_Y + 3, 150, FOOT_H - 8, UI_MAIN);
    drawStringCustom(214, FOOT_Y + 12, "EXIT", UI_MAIN, 1);
}

static void draw_body() {
    tft.fillRect(0, BODY_Y0, SCR_W, BODY_Y1 - BODY_Y0, UI_BG);

    // Compose the visible window: committed lines + the in-progress line.
    int total = s_line_count + 1;             // include current line
    int start = total - VIS_ROWS;
    if (start < 0) start = 0;

    int y = BODY_Y0;
    for (int idx = start; idx < total; idx++) {
        const String &line = (idx < s_line_count) ? s_lines[idx] : s_cur;
        if (line.length()) drawStringCustom(4, y, line, UI_MAIN, 1);
        y += ROW_H;
    }
}

static void draw_all() {
    tft.fillScreen(UI_BG);
    draw_header();
    draw_body();
    draw_footer();
}

// Hit testing for the footer buttons. Returns 1=TYPE, 2=EXIT, 0=none.
static int footer_hit(int x, int y) {
    if (y < FOOT_Y + 3 || y > FOOT_Y + FOOT_H - 5) return 0;
    if (x >= 6 && x <= 156)  return 1;
    if (x >= 164 && x <= 314) return 2;
    return 0;
}

static bool getTouchCalibrated(uint16_t *x, uint16_t *y) {
    uint16_t z = tft.getTouchRawZ();
    if (z < 600) return false;

    uint16_t rawX = 0, rawY = 0;
    tft.getTouchRaw(&rawX, &rawY);

    const uint16_t X_RAW_LEFT  = 420;  // screen x = 0
    const uint16_t X_RAW_RIGHT = 3660; // screen x = 320
    const uint16_t Y_RAW_TOP   = 3790; // screen y = 0
    const uint16_t Y_RAW_BOTTOM = 250;  // screen y = 480

    long sx = map((long)rawX, X_RAW_LEFT, X_RAW_RIGHT, 0, 320);
    long sy = map((long)rawY, Y_RAW_TOP, Y_RAW_BOTTOM, 0, 480);
    *x = (uint16_t)constrain(sx, 0, 319);
    *y = (uint16_t)constrain(sy, 0, 479);
    return true;
}

void pi_terminal_open() {
    // Lazily configure our display instance to match Marauder's panel.
    static bool disp_ready = false;
    if (!disp_ready) {
        disp_ready = true;
        tft.init();
        tft.setRotation(PI_TERM_ROTATION);
        // Portrait calibration for the CYD 3.5" XPT2046 (matches Marauder).
        uint16_t calData[5] = { 239, 3560, 262, 3643, 4 };
        tft.setTouch(calData);
    }
    tft.setRotation(PI_TERM_ROTATION);

    draw_all();
    s_dirty = false;

    bool running = true;
    bool wasTouched = false;
    uint16_t tx = 0, ty = 0;
    uint32_t last_hdr = millis();
    bool last_link = s_link_ok;

    while (running) {
        // Keep the link live: stream output continues to flow into the model.
        pi_terminal_pump();

        if (s_dirty) {
            draw_body();
            s_dirty = false;
        }
        if (s_link_ok != last_link || millis() - last_hdr > 1000) {
            last_link = s_link_ok;
            last_hdr = millis();
            draw_header();
        }

        bool touching = getTouchCalibrated(&tx, &ty);
        if (touching && !wasTouched) {
            wasTouched = true;
            int hit = footer_hit((int)tx, (int)ty);
            if (hit == 1) {
                beep(2600, 10);
                String cmd = virtualKeyboardInput("PI TERMINAL",
                                                  "Enter command, OK to send");
                // Returning from the keyboard wiped the screen; restore it.
                if (cmd.length() > 0) {
                    // Local echo so the user sees what was sent immediately.
                    term_feed((const uint8_t *)"$ ", 2);
                    term_feed((const uint8_t *)cmd.c_str(), cmd.length());
                    term_feed((const uint8_t *)"\n", 1);
                    link_send_command(cmd);
                }
                draw_all();
                s_dirty = false;
            } else if (hit == 2) {
                beep(1800, 10);
                running = false;
            }
        } else if (!touching && wasTouched) {
            wasTouched = false;
        }

        delay(5);  // light yield; pump already keeps RX drained
    }
}
