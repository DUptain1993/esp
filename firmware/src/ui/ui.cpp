#include "ui.h"
#include "theme.h"
#include "../app/stream.h"
#include "../app/mirror.h"
#include "../app/ota.h"
#include "../app/device_manager.h"

static lv_obj_t *tv;
static lv_obj_t *t_menu, *t_stream, *t_mirror, *t_ota, *t_devices;
static lv_obj_t *status_bar;
static lv_obj_t *lbl_device, *lbl_status, *lbl_cpu;

void ui_init(void) {
    ui_theme_init();

    // Background
    lv_obj_set_style_bg_color(lv_scr_act(), COLOR_BG, 0);

    // TileView for screens
    tv = lv_tileview_create(lv_scr_act());
    lv_obj_set_size(tv, 320, 440); // Leave room for status bar
    lv_obj_align(tv, LV_ALIGN_TOP_MID, 0, 0);

    // Add Tiles
    t_menu = lv_tileview_add_tile(tv, 0, 0, LV_DIR_HOR);
    t_stream = lv_tileview_add_tile(tv, 1, 0, LV_DIR_HOR);
    t_mirror = lv_tileview_add_tile(tv, 2, 0, LV_DIR_HOR);
    t_ota = lv_tileview_add_tile(tv, 3, 0, LV_DIR_HOR);
    t_devices = lv_tileview_add_tile(tv, 4, 0, LV_DIR_HOR);

    // Main Menu
    lv_obj_t *menu_cont = lv_obj_create(t_menu);
    lv_obj_set_size(menu_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(menu_cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_add_style(menu_cont, ui_get_style_panel(), 0);

    const char *btns[] = {"STREAM", "MIRROR", "OTA", "DEVICES", "SYSTEM", "SETTINGS"};
    for(int i=0; i<6; i++) {
        lv_obj_t *btn = lv_btn_create(menu_cont);
        lv_obj_set_size(btn, 140, 60);
        lv_obj_add_style(btn, ui_get_style_btn(), 0);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, btns[i]);
        lv_obj_center(lbl);
    }

    // Initialize App Views
    app_stream_init(t_stream);
    app_mirror_init(t_mirror);
    app_ota_init(t_ota);
    app_device_manager_init(t_devices);

    // Status Bar
    status_bar = lv_obj_create(lv_scr_act());
    lv_obj_set_size(status_bar, 320, 40);
    lv_obj_align(status_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_style(status_bar, ui_get_style_panel(), 0);
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);

    lbl_device = lv_label_create(status_bar);
    lv_label_set_text(lbl_device, "DEV: Pi-01");
    lv_obj_align(lbl_device, LV_ALIGN_LEFT_MID, 5, 0);
    lv_obj_add_style(lbl_device, ui_get_style_text(), 0);

    lbl_cpu = lv_label_create(status_bar);
    lv_label_set_text(lbl_cpu, "CPU: 12%");
    lv_obj_align(lbl_cpu, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(lbl_cpu, ui_get_style_text(), 0);

    lbl_status = lv_label_create(status_bar);
    lv_label_set_text(lbl_status, "CONNECTED");
    lv_obj_align(lbl_status, LV_ALIGN_RIGHT_MID, -5, 0);
    lv_obj_add_style(lbl_status, ui_get_style_text(), 0);
}

void ui_update_status(const char *device, const char *status, int cpu_load) {
    if (device) lv_label_set_text_fmt(lbl_device, "DEV: %s", device);
    if (status) lv_label_set_text(lbl_status, status);
    lv_label_set_text_fmt(lbl_cpu, "CPU: %d%%", cpu_load);
}
