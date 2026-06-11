#ifndef UI_THEME_H
#define UI_THEME_H

#include <lvgl.h>

#define COLOR_BG      lv_color_hex(0x0A0A0F)
#define COLOR_PANEL   lv_color_hex(0x12121A)
#define COLOR_ACCENT  lv_color_hex(0x00FFD1)
#define COLOR_WARNING lv_color_hex(0xFF3B3B)
#define COLOR_TEXT    lv_color_hex(0xE0E0E0)
#define COLOR_DIM     lv_color_hex(0x606060)

void ui_theme_init(void);
lv_style_t *ui_get_style_panel(void);
lv_style_t *ui_get_style_btn(void);
lv_style_t *ui_get_style_text(void);

#endif // UI_THEME_H
