#include "stream.h"
#include "../ui/ui.h"
#include "../ui/theme.h"

// Bounded ring buffer of terminal text. When full, the oldest half is dropped
// so the textarea never grows without bound.
#define STREAM_CAP 2048

static char   s_buf[STREAM_CAP + 1];
static size_t s_len = 0;

// ANSI parser state.
enum AnsiState { A_NORMAL, A_ESC, A_CSI };
static AnsiState s_state = A_NORMAL;
static int  s_params[8];
static int  s_nparam = 0;
static bool s_dirty = false;

static void buf_reset(void)
{
    s_len = 0;
    s_buf[0] = '\0';
}

static void buf_trim_if_needed(void)
{
    if (s_len < STREAM_CAP) return;
    // Drop the oldest half, keep recent output (ring behaviour).
    size_t keep = STREAM_CAP / 2;
    memmove(s_buf, s_buf + (s_len - keep), keep);
    s_len = keep;
    s_buf[s_len] = '\0';
}

static void buf_putc(char c)
{
    buf_trim_if_needed();
    s_buf[s_len++] = c;
    s_buf[s_len] = '\0';
    s_dirty = true;
}

static void erase_current_line(void)
{
    // Remove characters back to the previous newline.
    while (s_len > 0 && s_buf[s_len - 1] != '\n') {
        s_len--;
    }
    s_buf[s_len] = '\0';
    s_dirty = true;
}

static lv_color_t sgr_to_color(int code)
{
    switch (code) {
    case 30: return lv_color_hex(0x000000);
    case 31: return THEME_COL_ERR;
    case 32: return THEME_COL_OK;
    case 33: return THEME_COL_WARN;
    case 34: return lv_color_hex(0x4080FF);
    case 35: return lv_color_hex(0xC050FF);
    case 36: return THEME_COL_ACCENT;
    case 37: return THEME_COL_TEXT;
    case 90: return THEME_COL_DIM;
    default: return THEME_COL_OK; // 0/39/reset -> default green
    }
}

static void apply_csi(char final)
{
    switch (final) {
    case 'm': { // SGR: colour
        int code = (s_nparam > 0) ? s_params[0] : 0;
        lv_obj_t *ta = ui_stream_textarea();
        if (ta) lv_obj_set_style_text_color(ta, sgr_to_color(code), LV_PART_MAIN);
        break;
    }
    case 'J': // erase display (2J = clear all)
        if (s_nparam > 0 && s_params[0] == 2) buf_reset();
        s_dirty = true;
        break;
    case 'K': // erase line
        erase_current_line();
        break;
    // Cursor movement is consumed (scrolling terminal view ignores positioning).
    case 'A': case 'B': case 'C': case 'D':
    case 'H': case 'f': case 's': case 'u':
    default:
        break;
    }
}

static void feed_byte(uint8_t b)
{
    switch (s_state) {
    case A_NORMAL:
        if (b == 0x1B) {            // ESC
            s_state = A_ESC;
        } else if (b == '\r') {
            // carriage return: move to line start (erase line content)
            erase_current_line();
        } else if (b == '\t') {
            buf_putc(' '); buf_putc(' ');
        } else if (b == '\n' || b == ' ' || (b >= 0x20 && b < 0x7F)) {
            buf_putc((char)b);
        }
        break;

    case A_ESC:
        if (b == '[') {
            s_state = A_CSI;
            s_nparam = 0;
            for (int i = 0; i < 8; i++) s_params[i] = 0;
            s_params[0] = 0;
        } else {
            s_state = A_NORMAL; // unsupported escape, ignore
        }
        break;

    case A_CSI:
        if (b >= '0' && b <= '9') {
            if (s_nparam == 0) s_nparam = 1;
            s_params[s_nparam - 1] = s_params[s_nparam - 1] * 10 + (b - '0');
        } else if (b == ';') {
            if (s_nparam < 8) s_nparam++;
        } else {
            // final byte
            apply_csi((char)b);
            s_state = A_NORMAL;
        }
        break;
    }
}

void stream_init(void)
{
    buf_reset();
    s_state = A_NORMAL;
    s_nparam = 0;
}

void stream_clear(void)
{
    ui_lock();
    buf_reset();
    lv_obj_t *ta = ui_stream_textarea();
    if (ta) lv_textarea_set_text(ta, "");
    ui_unlock();
}

void stream_feed(const uint8_t *data, size_t len)
{
    if (!data || len == 0) return;

    ui_lock();
    s_dirty = false;
    for (size_t i = 0; i < len; i++) feed_byte(data[i]);

    if (s_dirty) {
        lv_obj_t *ta = ui_stream_textarea();
        if (ta) {
            lv_textarea_set_text(ta, s_buf);
            lv_textarea_set_cursor_pos(ta, LV_TEXTAREA_CURSOR_LAST); // auto-scroll
        }
    }
    ui_unlock();
}
