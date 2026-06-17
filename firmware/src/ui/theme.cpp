#include "theme.h"

lv_style_t style_screen;
lv_style_t style_panel;
lv_style_t style_btn;
lv_style_t style_btn_pressed;
lv_style_t style_title;
lv_style_t style_text;
lv_style_t style_accent;

void theme_init(void)
{
    // Screen background: flat, opaque.
    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, THEME_COL_BG);
    lv_style_set_bg_opa(&style_screen, LV_OPA_COVER);
    lv_style_set_border_width(&style_screen, 0);
    lv_style_set_pad_all(&style_screen, 0);
    lv_style_set_text_color(&style_screen, THEME_COL_TEXT);
    lv_style_set_text_font(&style_screen, &lv_font_montserrat_14);

    // Panel: flat dark card with accent border, no shadow/gradient.
    lv_style_init(&style_panel);
    lv_style_set_bg_color(&style_panel, THEME_COL_PANEL);
    lv_style_set_bg_opa(&style_panel, LV_OPA_COVER);
    lv_style_set_border_color(&style_panel, THEME_COL_DIM);
    lv_style_set_border_width(&style_panel, 1);
    lv_style_set_radius(&style_panel, 4);
    lv_style_set_pad_all(&style_panel, 6);
    lv_style_set_text_color(&style_panel, THEME_COL_TEXT);

    // Button: flat panel colour with accent text/border.
    lv_style_init(&style_btn);
    lv_style_set_bg_color(&style_btn, THEME_COL_PANEL);
    lv_style_set_bg_opa(&style_btn, LV_OPA_COVER);
    lv_style_set_border_color(&style_btn, THEME_COL_ACCENT);
    lv_style_set_border_width(&style_btn, 1);
    lv_style_set_radius(&style_btn, 3);
    lv_style_set_text_color(&style_btn, THEME_COL_ACCENT);
    lv_style_set_pad_all(&style_btn, 4);

    // Pressed state: invert accent.
    lv_style_init(&style_btn_pressed);
    lv_style_set_bg_color(&style_btn_pressed, THEME_COL_ACCENT);
    lv_style_set_bg_opa(&style_btn_pressed, LV_OPA_COVER);
    lv_style_set_text_color(&style_btn_pressed, THEME_COL_BG);

    // Title text.
    lv_style_init(&style_title);
    lv_style_set_text_color(&style_title, THEME_COL_ACCENT);
    lv_style_set_text_font(&style_title, &lv_font_montserrat_20);

    // Body text.
    lv_style_init(&style_text);
    lv_style_set_text_color(&style_text, THEME_COL_TEXT);
    lv_style_set_text_font(&style_text, &lv_font_montserrat_14);

    // Accent text.
    lv_style_init(&style_accent);
    lv_style_set_text_color(&style_accent, THEME_COL_ACCENT);
    lv_style_set_text_font(&style_accent, &lv_font_montserrat_24);
}

void theme_apply_screen(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &style_screen, LV_PART_MAIN);
}

void theme_apply_panel(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &style_panel, LV_PART_MAIN);
}

void theme_apply_button(lv_obj_t *btn)
{
    lv_obj_add_style(btn, &style_btn, LV_PART_MAIN);
    lv_obj_add_style(btn, &style_btn_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
}
