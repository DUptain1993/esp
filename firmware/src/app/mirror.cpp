#include "mirror.h"
#include "../ui/ui.h"
#include "../ui/theme.h"

#define MIRROR_CANVAS_W 160
#define MIRROR_CANVAS_H 160

static MirrorMode s_mode = MIRROR_TERMINAL;

static inline lv_color_t color565(uint16_t v)
{
    lv_color_t c;
    c.full = v; // depth 16, native order
    return c;
}

static inline uint16_t rd16(const uint8_t *p) // little-endian
{
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

void mirror_init(void)
{
    s_mode = MIRROR_TERMINAL;
}

void mirror_set_mode(MirrorMode mode)
{
    s_mode = mode;
    ui_lock();
    lv_obj_t *cv = ui_mirror_canvas();
    if (cv) {
        if (mode == MIRROR_GRAPHICAL)
            lv_obj_clear_flag(cv, LV_OBJ_FLAG_HIDDEN);
        else
            lv_canvas_fill_bg(cv, THEME_COL_PANEL, LV_OPA_COVER);
    }
    ui_unlock();
}

MirrorMode mirror_get_mode(void) { return s_mode; }

void mirror_feed(const uint8_t *data, size_t len)
{
    if (s_mode != MIRROR_GRAPHICAL) return;
    if (!data || len == 0) return;

    ui_lock();
    lv_obj_t *cv = ui_mirror_canvas();
    if (!cv) { ui_unlock(); return; }

    size_t i = 0;
    while (i < len) {
        uint8_t op = data[i++];
        switch (op) {
        case MOP_FILL:
            if (i + 2 <= len) {
                lv_canvas_fill_bg(cv, color565(rd16(&data[i])), LV_OPA_COVER);
                i += 2;
            } else i = len;
            break;

        case MOP_PIXEL:
            if (i + 6 <= len) {
                uint16_t x = rd16(&data[i]);
                uint16_t y = rd16(&data[i + 2]);
                uint16_t c = rd16(&data[i + 4]);
                if (x < MIRROR_CANVAS_W && y < MIRROR_CANVAS_H)
                    lv_canvas_set_px(cv, x, y, color565(c));
                i += 6;
            } else i = len;
            break;

        case MOP_RECT:
            if (i + 10 <= len) {
                uint16_t x = rd16(&data[i]);
                uint16_t y = rd16(&data[i + 2]);
                uint16_t w = rd16(&data[i + 4]);
                uint16_t h = rd16(&data[i + 6]);
                lv_color_t c = color565(rd16(&data[i + 8]));
                for (uint16_t yy = 0; yy < h && (y + yy) < MIRROR_CANVAS_H; yy++)
                    for (uint16_t xx = 0; xx < w && (x + xx) < MIRROR_CANVAS_W; xx++)
                        lv_canvas_set_px(cv, x + xx, y + yy, c);
                i += 10;
            } else i = len;
            break;

        case MOP_HLINE:
            if (i + 8 <= len) {
                uint16_t x = rd16(&data[i]);
                uint16_t y = rd16(&data[i + 2]);
                uint16_t l = rd16(&data[i + 4]);
                lv_color_t c = color565(rd16(&data[i + 6]));
                for (uint16_t xx = 0; xx < l && (x + xx) < MIRROR_CANVAS_W; xx++)
                    if (y < MIRROR_CANVAS_H) lv_canvas_set_px(cv, x + xx, y, c);
                i += 8;
            } else i = len;
            break;

        default:
            // Unknown op: abort to avoid mis-parsing the rest.
            i = len;
            break;
        }
    }
    lv_obj_invalidate(cv);
    ui_unlock();
}
