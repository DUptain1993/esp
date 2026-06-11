#include "device_manager.h"
#include <string.h>

static uint8_t active_device_id = 1;
static lv_obj_t *device_list;

static void device_event_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    const char *text = lv_list_get_btn_text(device_list, btn);
    if (text && strstr(text, "Pi-")) {
        active_device_id = atoi(text + 3);
    }
}

void app_device_manager_init(lv_obj_t *parent) {
    device_list = lv_list_create(parent);
    lv_obj_set_size(device_list, LV_PCT(100), LV_PCT(80));
    lv_obj_align(device_list, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *btn;
    btn = lv_list_add_btn(device_list, LV_SYMBOL_VIDEO, "Pi-01 (Main)");
    lv_obj_add_event_cb(btn, device_event_cb, LV_EVENT_CLICKED, NULL);
    btn = lv_list_add_btn(device_list, LV_SYMBOL_VIDEO, "Pi-02 (Aux)");
    lv_obj_add_event_cb(btn, device_event_cb, LV_EVENT_CLICKED, NULL);
    btn = lv_list_add_btn(device_list, LV_SYMBOL_VIDEO, "Pi-03 (Security)");
    lv_obj_add_event_cb(btn, device_event_cb, LV_EVENT_CLICKED, NULL);
}

void app_device_manager_handle_packet(const packet_t *pkt) {
    if (pkt->channel != CH_ROUTING) return;
    // Handle device registry updates if needed
}

uint8_t app_device_manager_get_active_id(void) {
    return active_device_id;
}
