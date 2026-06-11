#include "theme.h"

static lv_style_t s_p, s_b, s_t;

void ui_theme_init() {
    lv_style_init(&s_p);
    lv_style_set_bg_color(&s_p, lv_color_hex(0x12121A));
    lv_style_set_border_color(&s_p, lv_color_hex(0x00FFD1));
    lv_style_set_border_width(&s_p, 1);
    lv_style_set_radius(&s_p, 0);

    lv_style_init(&s_b);
    lv_style_set_bg_color(&s_b, lv_color_hex(0x12121A));
    lv_style_set_border_color(&s_b, lv_color_hex(0x00FFD1));
    lv_style_set_border_width(&s_b, 1);
    lv_style_set_text_color(&s_b, lv_color_hex(0xE0E0E0));

    lv_style_init(&s_t);
    lv_style_set_text_color(&s_t, lv_color_hex(0xE0E0E0));
}

lv_style_t *ui_get_style_panel() { return &s_p; }
lv_style_t *ui_get_style_btn() { return &s_b; }
lv_style_t *ui_get_style_text() { return &s_t; }
