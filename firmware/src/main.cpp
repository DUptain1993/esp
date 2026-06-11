#include <Arduino.h>
#include <lvgl.h>
#include "hal/display.h"
#include "hal/touch.h"
#include "core/comms.h"
#include "core/encryption.h"
#include "core/commands.h"
#include "ui/ui.h"
#include "app/stream.h"
#include "app/ota.h"
#include "app/device_manager.h"

TaskHandle_t l_task = NULL;
TaskHandle_t c_task = NULL;

void lv_task_entry(void*) {
    while(1) {
        lv_timer_handler();
        if (lv_disp_get_inactive_time(NULL) > 30000) digitalWrite(27, LOW);
        else digitalWrite(27, HIGH);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void cm_task_entry(void*) {
    while(1) {
        comms_update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void on_ctrl(const packet_t *pkt) {
    uint8_t cmd = pkt->payload[0];
    if (cmd == 0xF0) {
        uint32_t n = *((uint32_t *)&pkt->payload[1]);
        encryption_init(encryption_derive_key(n));
        ui_update_status(NULL, "ENCRYPTED", 0);
    }
}

void setup() {
    Serial.begin(115200);
    lv_init();
    hal_display_init();
    hal_touch_init();
    ui_init();
    comms_init();
    comms_register_callback(CH_CONTROL, on_ctrl);
    comms_register_callback(CH_STREAM, app_stream_handle_packet);
    comms_register_callback(CH_OTA, app_ota_handle_packet);

    xTaskCreatePinnedToCore(lv_task_entry, "lv", 4096, NULL, 1, &l_task, 1);
    xTaskCreatePinnedToCore(cm_task_entry, "comms", 4096, NULL, 1, &c_task, 0);
}

void loop() { vTaskDelay(pdMS_TO_TICKS(1000)); }
