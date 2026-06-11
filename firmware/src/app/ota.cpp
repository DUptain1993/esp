#include "ota.h"
#include <Update.h>
#include <Arduino.h>

static lv_obj_t *bar, *lbl;
static size_t tsz = 0, rsz = 0;

void app_ota_init(lv_obj_t *p) {
    bar = lv_bar_create(p); lv_obj_set_size(bar, 200, 20); lv_obj_center(bar);
    lbl = lv_label_create(p); lv_label_set_text(lbl, "Ready");
    lv_obj_align_to(lbl, bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}

void app_ota_handle_packet(const packet_t *pkt) {
    if (pkt->channel != CH_OTA) return;
    uint8_t cmd = pkt->payload[0];
    if (cmd == 0x08) {
        tsz = *((uint32_t *)&pkt->payload[1]); rsz = 0;
        if (Update.begin(tsz)) lv_label_set_text(lbl, "OTA...");
    } else if (cmd == 0x09) {
        if (Update.write((uint8_t *)&pkt->payload[1], pkt->length - 1) == pkt->length - 1) {
            rsz += pkt->length - 1;
            lv_bar_set_value(bar, (rsz * 100) / tsz, LV_ANIM_OFF);
        }
    } else if (cmd == 0x0A) {
        if (Update.end(true)) { lv_label_set_text(lbl, "Done"); delay(500); ESP.restart(); }
    }
}
