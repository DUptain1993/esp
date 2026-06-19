#include "VirtualKeyboard.h"
#include "DisplayTFT.h"
#include "PepeDraw.h"
#include "Pins.h"
#include "SoundUtils.h"

// ===========================================================================
//  Touch-driven on-screen keyboard for the CYD 3.5" (portrait 320x480).
//
//  The original project polled BTN_UP / BTN_OK / BTN_DOWN. That hardware does
//  not exist on the CYD, so input is taken from the resistive touchscreen via
//  TFT_eSPI's getTouch(). The optional button path is still compiled when
//  PT_USE_BUTTONS is defined, keeping this source usable on button hardware.
// ===========================================================================

// ── Layout (physical pixels) ───────────────────────────────────────────────
static const int SCREEN_W   = 320;

static const int KB_COLS    = 10;
static const int KB_ROWS    = 4;     // alphanumeric rows
static const int KB_KEY_W   = 28;
static const int KB_KEY_H   = 24;
static const int KB_STRIDE_X= 30;    // key width + gap
static const int KB_STRIDE_Y= 28;
static const int KB_START_X = 10;
static const int KB_START_Y = 250;

static const int SP_COUNT   = 5;     // special keys
static const int SP_KEY_W   = 58;    // 2 * KB_KEY_W + gap
static const int SP_STRIDE_X= 62;
static const int SP_KEY_H   = 30;
static const int SP_START_X = 10;
static const int SP_START_Y = KB_START_Y + KB_ROWS * KB_STRIDE_Y + 6;

// Special key indices
enum { SP_SHIFT = 0, SP_SPACE, SP_DEL, SP_ENTER, SP_CANCEL };

static const char* KB_LOWER[KB_ROWS] = {
    "1234567890",
    "qwertyuiop",
    "asdfghjkl'",
    "zxcvbnm,.?"
};
static const char* KB_UPPER[KB_ROWS] = {
    "!@#$%^&*()",
    "QWERTYUIOP",
    "ASDFGHJKL:",
    "ZXCVBNM<>/"
};

static const char* SP_LABEL[SP_COUNT] = { "SHFT", "SPACE", "DEL", "OK", "X" };

// ── Helpers ────────────────────────────────────────────────────────────────
static void drawKey(int x, int y, int w, int h, const String& label,
                    bool active) {
    uint16_t bg = active ? UI_SELECT : UI_BG;
    uint16_t fg = active ? UI_BG : UI_MAIN;
    tft.fillRect(x, y, w, h, bg);
    tft.drawRect(x, y, w, h, UI_ACCENT);
    int tw = getTextWidth(label, 1, FONT_SMALL);
    int tx = x + (w - tw) / 2;
    int ty = y + (h - getFontHeight(1, FONT_SMALL)) / 2;
    drawStringCustom(tx, ty, label, fg, 1);
}

// Render the full keyboard grid; `selRow`/`selCol` get the highlight.
static void drawKeyboard(bool shift, int selRow, int selCol) {
    const char** layout = shift ? KB_UPPER : KB_LOWER;
    for (int r = 0; r < KB_ROWS; r++) {
        for (int c = 0; c < KB_COLS; c++) {
            int x = KB_START_X + c * KB_STRIDE_X;
            int y = KB_START_Y + r * KB_STRIDE_Y;
            char ch = layout[r][c];
            drawKey(x, y, KB_KEY_W, KB_KEY_H, String(ch),
                    (r == selRow && c == selCol));
        }
    }
    for (int s = 0; s < SP_COUNT; s++) {
        int x = SP_START_X + s * SP_STRIDE_X;
        String lbl = SP_LABEL[s];
        if (s == SP_SHIFT && shift) lbl = "shft";  // indicate active
        drawKey(x, SP_START_Y, SP_KEY_W, SP_KEY_H, lbl,
                (selRow == KB_ROWS && selCol == s));
    }
}

// Redraw the input/preview line.
static void drawInputField(const String& buffer, bool mask) {
    const int x = 8, y = 70, w = SCREEN_W - 16, h = 28;
    tft.fillRect(x, y, w, h, UI_BG);
    tft.drawRect(x, y, w, h, UI_MAIN);

    String shown;
    if (mask) for (uint16_t i = 0; i < buffer.length(); i++) shown += '*';
    else shown = buffer;

    // Keep the tail visible (simple horizontal scroll).
    const int maxChars = (w - 12) / 6;  // GLCD cell = 6px
    if ((int)shown.length() > maxChars)
        shown = shown.substring(shown.length() - maxChars);

    drawStringCustom(x + 4, y + (h - 8) / 2, shown + "_", UI_MAIN, 1);
}

static void drawHeader(const String& title, const String& subtitle) {
    tft.fillRect(0, 0, SCREEN_W, 64, UI_BG);
    drawStringCentered(8, title, UI_MAIN, 2, FONT_BIG);
    drawStringCentered(44, subtitle, UI_ACCENT, 1, FONT_SMALL);
}

// Map a physical touch point to a key. Returns true on hit and fills row/col
// (row == KB_ROWS marks a special key, col == special index).
static bool hitTest(int px, int py, int& row, int& col) {
    // Alphanumeric grid
    for (int r = 0; r < KB_ROWS; r++) {
        int top = KB_START_Y + r * KB_STRIDE_Y;
        if (py < top || py >= top + KB_KEY_H) continue;
        for (int c = 0; c < KB_COLS; c++) {
            int left = KB_START_X + c * KB_STRIDE_X;
            if (px >= left && px < left + KB_KEY_W) {
                row = r; col = c; return true;
            }
        }
    }
    // Special row
    if (py >= SP_START_Y && py < SP_START_Y + SP_KEY_H) {
        for (int s = 0; s < SP_COUNT; s++) {
            int left = SP_START_X + s * SP_STRIDE_X;
            if (px >= left && px < left + SP_KEY_W) {
                row = KB_ROWS; col = s; return true;
            }
        }
    }
    return false;
}

// ── Main entry ──────────────────────────────────────────────────────────────
String virtualKeyboardInput(const String& title,
                             const String& subtitle,
                             int maxLen,
                             bool maskInput) {
    String buffer = "";
    bool   shift  = false;
    bool   done   = false;
    bool   cancelled = false;

    tft.fillScreen(UI_BG);
    drawHeader(title, subtitle);
    drawInputField(buffer, maskInput);
    drawKeyboard(shift, -1, -1);

    bool wasTouched = false;
    uint16_t tx = 0, ty = 0;

    while (!done) {
        bool touching = tft.getTouch(&tx, &ty);

        if (touching && !wasTouched) {
            wasTouched = true;
            int row = -1, col = -1;
            if (hitTest((int)tx, (int)ty, row, col)) {
                // Visual press feedback
                drawKeyboard(shift, row, col);
                beep(2200, 8);

                if (row < KB_ROWS) {
                    const char** layout = shift ? KB_UPPER : KB_LOWER;
                    char ch = layout[row][col];
                    if ((int)buffer.length() < maxLen) buffer += ch;
                    drawInputField(buffer, maskInput);
                } else {
                    switch (col) {
                        case SP_SHIFT:
                            shift = !shift;
                            break;
                        case SP_SPACE:
                            if ((int)buffer.length() < maxLen) buffer += ' ';
                            drawInputField(buffer, maskInput);
                            break;
                        case SP_DEL:
                            if (buffer.length() > 0)
                                buffer.remove(buffer.length() - 1);
                            drawInputField(buffer, maskInput);
                            break;
                        case SP_ENTER:
                            done = true;
                            break;
                        case SP_CANCEL:
                            cancelled = true;
                            done = true;
                            break;
                    }
                }
            }
        } else if (!touching && wasTouched) {
            wasTouched = false;
            // Clear highlight on release
            if (!done) drawKeyboard(shift, -1, -1);
        }

        delay(8);  // debounce / yield; keeps RX pump responsive elsewhere
    }

    return cancelled ? String("") : buffer;
}
