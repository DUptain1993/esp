#pragma once
#include <lvgl.h>

// Cyberpunk Stealth HUD palette (flat: no transparency, no gradients).
#define THEME_COL_BG     lv_color_hex(0x0A0A0F)  // Background
#define THEME_COL_PANEL  lv_color_hex(0x12121A)  // Panel
#define THEME_COL_ACCENT lv_color_hex(0x00FFD1)  // Accent
#define THEME_COL_TEXT   lv_color_hex(0xE0E0E0)  // Text
#define THEME_COL_DIM    lv_color_hex(0x303040)  // Borders / inactive
#define THEME_COL_OK     lv_color_hex(0x00FF6A)
#define THEME_COL_WARN   lv_color_hex(0xFFB000)
#define THEME_COL_ERR    lv_color_hex(0xFF3B3B)

// Reusable styles.
extern lv_style_t style_screen;
extern lv_style_t style_panel;
extern lv_style_t style_btn;
extern lv_style_t style_btn_pressed;
extern lv_style_t style_title;
extern lv_style_t style_text;
extern lv_style_t style_accent;

// Initialise all styles. Call once before building the UI.
void theme_init(void);

// Apply the flat dark background to a screen/container.
void theme_apply_screen(lv_obj_t *obj);
void theme_apply_panel(lv_obj_t *obj);
void theme_apply_button(lv_obj_t *btn);
