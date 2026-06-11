#include "ota.h"
#include <Update.h>
#include <Arduino.h>

static lv_obj_t *progress_bar;
static lv_obj_t *status_label;
static size_t update_size = 0;
static size_t received_size = 0;

void app_ota_init(lv_obj_t *parent) {
    progress_bar = lv_bar_create(parent);
    lv_obj_set_size(progress_bar, 200, 20);
    lv_obj_center(progress_bar);
    lv_bar_set_range(progress_bar, 0, 100);
    lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);

    status_label = lv_label_create(parent);
    lv_label_set_text(status_label, "Ready for OTA");
    lv_obj_align_to(status_label, progress_bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}

void app_ota_handle_packet(const packet_t *pkt) {
    if (pkt->channel != CH_OTA) return;

    uint8_t cmd = pkt->payload[0];
    if (cmd == 0x08) { // START
        update_size = *((uint32_t *)&pkt->payload[1]);
        received_size = 0;
        if (Update.begin(update_size)) {
            lv_label_set_text(status_label, "OTA Started...");
        } else {
            lv_label_set_text(status_label, "OTA Failed to begin");
        }
    } else if (cmd == 0x09) { // CHUNK
        size_t chunk_len = pkt->length - 1;
        if (Update.write((uint8_t *)&pkt->payload[1], chunk_len) == chunk_len) {
            received_size += chunk_len;
            lv_bar_set_value(progress_bar, (received_size * 100) / update_size, LV_ANIM_OFF);
        }
    } else if (cmd == 0x0A) { // END
        if (Update.end(true)) {
            lv_label_set_text(status_label, "OTA Success! Rebooting...");
            delay(1000);
            ESP.restart();
        } else {
            lv_label_set_text(status_label, "OTA End Failed");
        }
    }
}
