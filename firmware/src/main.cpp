#include <Arduino.h>
#include <lvgl.h>
#include "hal/display.h"
#include "hal/touch.h"
#include "core/comms.h"
#include "core/encryption.h"
#include "core/commands.h"
#include "ui/ui.h"
#include "app/stream.h"
#include "app/mirror.h"
#include "app/ota.h"
#include "app/device_manager.h"

// Task handles
TaskHandle_t lvgl_task_handle = NULL;
TaskHandle_t comms_task_handle = NULL;

// Handshake state
bool handshake_done = false;

// Idle management
uint32_t last_interaction = 0;
#define IDLE_TIMEOUT 30000 // 30 seconds

void lvgl_task(void *pvParameters) {
    while (1) {
        lv_timer_handler();
        
        // Idle Dimming
        if (lv_disp_get_inactive_time(NULL) > IDLE_TIMEOUT) {
            digitalWrite(27, LOW); // Backlight OFF
        } else {
            digitalWrite(27, HIGH); // Backlight ON
        }
        
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void comms_task(void *pvParameters) {
    while (1) {
        comms_update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void on_control_packet(const packet_t *pkt) {
    uint8_t cmd = pkt->payload[0];
    if (cmd == CMD_HANDSHAKE) {
        uint32_t nonce = *((uint32_t *)&pkt->payload[1]);
        uint32_t key = encryption_derive_key(nonce);
        encryption_init(key);
        handshake_done = true;
        ui_update_status(NULL, "ENCRYPTED", 0);
    } else if (cmd == CMD_STATUS_REQ) {
        // Send back status
        uint8_t resp[] = {CMD_STATUS_REQ, 0x01}; // OK
        comms_send_packet(CH_STATUS, 0, resp, 2);
    }
}

void setup() {
    // Basic setup
    Serial.begin(115200);
    Serial.println("System Starting...");
    
    // Initialize LVGL
    lv_init();
    Serial.println("LVGL Initialized");

    // Initialize HAL
    hal_display_init();
    Serial.println("Display HAL Initialized");
    hal_touch_init();
    Serial.println("Touch HAL Initialized");

    // Initialize UI
    ui_init();
    Serial.println("UI Initialized");

    // Initialize Comms
    comms_init();
    comms_register_callback(CH_CONTROL, on_control_packet);
    comms_register_callback(CH_STREAM, app_stream_handle_packet);
    comms_register_callback(CH_OTA, app_ota_handle_packet);
    comms_register_callback(CH_ROUTING, app_device_manager_handle_packet);

    // Create Tasks
    xTaskCreatePinnedToCore(lvgl_task, "lvgl", 8192, NULL, 1, &lvgl_task_handle, 1);
    xTaskCreatePinnedToCore(comms_task, "comms", 4096, NULL, 1, &comms_task_handle, 0);
    
    ui_update_status(NULL, "WAITING HS", 0);
}

void loop() {
    // Empty, using FreeRTOS tasks
    vTaskDelay(pdMS_TO_TICKS(1000));
}
