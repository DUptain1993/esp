#pragma once
#include <Arduino.h>
#include <lvgl.h>

// Application pages.
enum UiPage : int {
    PAGE_HOME = 0,
    PAGE_TERMINAL,   // USB: live terminal stream (CH_STREAM)
    PAGE_WIFI,       // WiFi: live network scanner
    PAGE_SYSTEM,     // System: live monitor + OTA progress
    PAGE_SETTINGS,   // Settings: backlight, link, reboot
    PAGE_MIRROR,     // mirror mode (canvas)
    PAGE_DEVICES,    // multi-device routing
    PAGE_COUNT
};

// Build the full UI. Must be visible on first boot ("DISPLAY OK").
void ui_init(void);

// Page navigation (driven by buttons and gestures).
void ui_show_page(int page);
void ui_next_page(void);
void ui_prev_page(void);
int  ui_current_page(void);

// Bottom HUD fields.
void ui_set_status(const char *txt);
void ui_set_status_color(lv_color_t c);
void ui_set_cpu(int load_pct);

// Quick action / execute hooks (triggered by swipe-up / long-press).
void ui_quick_action(void);
void ui_execute(void);

// Accessors used by app modules to render into the UI.
lv_obj_t *ui_stream_textarea(void);
lv_obj_t *ui_mirror_canvas(void);
void      ui_ota_set_progress(int pct, const char *status);
void      ui_devices_clear(void);
void      ui_devices_add(uint8_t id, bool active, uint32_t age_sec);
void      ui_set_active_device_label(uint8_t id);

// LVGL is not thread-safe: guard every LVGL call across tasks.
void ui_lock(void);
void ui_unlock(void);
bool ui_try_lock(uint32_t timeout_ms);
