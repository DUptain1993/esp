#include "device_manager.h"
#include <string.h>

static uint8_t aid = 1;
static lv_obj_t *list;

static void ev_cb(lv_event_t *e) {
    lv_obj_t *b = lv_event_get_target(e);
    const char *t = lv_list_get_btn_text(list, b);
    if (t && strstr(t, "Pi-")) aid = atoi(t + 3);
}

void app_device_manager_init(lv_obj_t *p) {
    list = lv_list_create(p); lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
    lv_obj_t *b;
    b = lv_list_add_btn(list, LV_SYMBOL_VIDEO, "Pi-01"); lv_obj_add_event_cb(b, ev_cb, LV_EVENT_CLICKED, NULL);
    b = lv_list_add_btn(list, LV_SYMBOL_VIDEO, "Pi-02"); lv_obj_add_event_cb(b, ev_cb, LV_EVENT_CLICKED, NULL);
}

uint8_t app_device_manager_get_active_id() { return aid; }
void app_device_manager_handle_packet(const packet_t*) {}
