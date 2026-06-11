#include "theme.h"

static lv_style_t style_panel;
static lv_style_t style_btn;
static lv_style_t style_text;

void ui_theme_init(void) {
    // Panel Style
    lv_style_init(&style_panel);
    lv_style_set_bg_color(&style_panel, COLOR_PANEL);
    lv_style_set_bg_opa(&style_panel, LV_OPA_COVER);
    lv_style_set_border_width(&style_panel, 1);
    lv_style_set_border_color(&style_panel, COLOR_ACCENT);
    lv_style_set_radius(&style_panel, 0);

    // Button Style
    lv_style_init(&style_btn);
    lv_style_set_bg_color(&style_btn, COLOR_PANEL);
    lv_style_set_bg_opa(&style_btn, LV_OPA_COVER);
    lv_style_set_border_width(&style_btn, 1);
    lv_style_set_border_color(&style_btn, COLOR_ACCENT);
    lv_style_set_text_color(&style_btn, COLOR_TEXT);
    lv_style_set_radius(&style_btn, 0);

    // Text Style
    lv_style_init(&style_text);
    lv_style_set_text_color(&style_text, COLOR_TEXT);
    lv_style_set_text_font(&style_text, &lv_font_montserrat_14);
}

lv_style_t *ui_get_style_panel(void) { return &style_panel; }
lv_style_t *ui_get_style_btn(void) { return &style_btn; }
lv_style_t *ui_get_style_text(void) { return &style_text; }
