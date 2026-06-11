#include "ui.h"
#include "theme.h"
#include "../app/stream.h"
#include "../app/mirror.h"
#include "../app/ota.h"
#include "../app/device_manager.h"
#include "../core/comms.h"
#include "../core/commands.h"

static lv_obj_t *tv, *lbl_d, *lbl_s;

static void ui_ev(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_GESTURE) {
        if (lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_TOP) {
            uint8_t c = CMD_STATUS_REQ;
            comms_send_packet(CH_CONTROL, app_device_manager_get_active_id(), &c, 1);
        }
    }
}

void ui_init() {
    ui_theme_init();
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x0A0A0F), 0);
    lv_obj_add_event_cb(lv_scr_act(), ui_ev, LV_EVENT_GESTURE, NULL);

    tv = lv_tileview_create(lv_scr_act());
    lv_obj_set_size(tv, 480, 280);
    
    lv_obj_t *m = lv_tileview_add_tile(tv, 0, 0, LV_DIR_HOR);
    lv_obj_t *s = lv_tileview_add_tile(tv, 1, 0, LV_DIR_HOR);
    lv_obj_t *o = lv_tileview_add_tile(tv, 2, 0, LV_DIR_HOR);
    lv_obj_t *d = lv_tileview_add_tile(tv, 3, 0, LV_DIR_HOR);

    lv_obj_t *cont = lv_obj_create(m);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_add_style(cont, ui_get_style_panel(), 0);

    const char *bn[] = {"STREAM", "OTA", "DEVICES"};
    for(int i=0; i<3; i++) {
        lv_obj_t *b = lv_btn_create(cont);
        lv_obj_set_size(b, 100, 50);
        lv_obj_add_style(b, ui_get_style_btn(), 0);
        lv_obj_t *l = lv_label_create(b); lv_label_set_text(l, bn[i]); lv_obj_center(l);
        lv_obj_add_event_cb(b, [](lv_event_t*e){ 
            int id = (int)lv_event_get_user_data(e);
            lv_obj_set_tile_id(tv, id, 0, LV_ANIM_ON);
        }, LV_EVENT_CLICKED, (void*)(uintptr_t)(i+1));
    }

    app_stream_init(s);
    app_ota_init(o);
    app_device_manager_init(d);

    lv_obj_t *bar = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bar, 480, 40); lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_style(bar, ui_get_style_panel(), 0);
    lbl_d = lv_label_create(bar); lv_label_set_text(lbl_d, "DEV: 1"); lv_obj_align(lbl_d, LV_ALIGN_LEFT_MID, 10, 0);
    lbl_s = lv_label_create(bar); lv_label_set_text(lbl_s, "READY"); lv_obj_align(lbl_s, LV_ALIGN_RIGHT_MID, -10, 0);
}

void ui_update_status(const char *d, const char *s, int c) {
    if (d) lv_label_set_text(lbl_d, d);
    if (s) lv_label_set_text(lbl_s, s);
}
