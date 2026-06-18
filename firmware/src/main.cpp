#include <Arduino.h>
#include <lvgl.h>
#include <WiFi.h>

#include "hal/display.h"
#include "hal/touch.h"

#include "core/comms.h"
#include "core/commands.h"

#include "ui/ui.h"
#include "ui/theme.h"

#include "app/stream.h"
#include "app/mirror.h"
#include "app/ota.h"
#include "app/device_manager.h"
#include "app/settings.h"

// ---------------------------------------------------------------------------
//  Little-endian helpers
// ---------------------------------------------------------------------------
static inline uint16_t le16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static inline uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

// ---------------------------------------------------------------------------
//  Central message dispatcher (runs in the comms task context)
// ---------------------------------------------------------------------------
static void app_dispatch(uint8_t channel, uint8_t device_id,
                         const uint8_t *msg, uint8_t len)
{
    if (len == 0) return;
    dm_seen(device_id);

    uint8_t cmd = msg[0];
    switch (channel) {

    case CH_CONTROL:
        switch (cmd) {
        case CMD_PING:
            comms_send_cmd(CH_CONTROL, device_id, CMD_PONG);
            break;
        case CMD_HANDSHAKE:
            if (len >= 5) {
                // Re-seed both cipher streams; ACK with PONG (not HANDSHAKE,
                // to avoid a reseed feedback loop on the peer).
                comms_set_nonce(le32(msg + 1));
                comms_send_cmd(CH_CONTROL, device_id, CMD_PONG);
                ui_lock();
                ui_set_status("LINK OK");
                ui_set_status_color(THEME_COL_OK);
                ui_unlock();
            }
            break;
        case CMD_SET_MODE:
            if (len >= 2) {
                mirror_set_mode((MirrorMode)msg[1]);
                ui_lock();
                ui_set_status(msg[1] == MIRROR_GRAPHICAL ? "MIRROR GFX" : "MIRROR TXT");
                ui_set_status_color(THEME_COL_ACCENT);
                ui_unlock();
            }
            break;
        case CMD_QUICK_ACTION:
            ui_lock(); ui_set_status("RMT QUICK"); ui_unlock();
            break;
        case CMD_EXECUTE:
            ui_lock(); ui_set_status("RMT EXEC"); ui_unlock();
            break;
        default:
            break;
        }
        break;

    case CH_STATUS:
        // Activity already recorded via dm_seen(); refresh the list.
        dm_render();
        break;

    case CH_STREAM:
        switch (cmd) {
        case STR_DATA:   stream_feed(msg + 1, len - 1); break;
        case STR_CLEAR:  stream_clear();                break;
        case STR_MIRROR: mirror_feed(msg + 1, len - 1); break;
        default: break;
        }
        break;

    case CH_OTA:
        switch (cmd) {
        case OTA_BEGIN:
            if (len >= 5) ota_begin(le32(msg + 1));
            break;
        case OTA_CHUNK:
            if (len >= 3) ota_chunk(le16(msg + 1), msg + 3, len - 3);
            break;
        case OTA_END:
            ota_end(len >= 5 ? le32(msg + 1) : 0);
            break;
        case OTA_ABORT:
            ota_abort();
            break;
        default: break;
        }
        break;

    case CH_ROUTING:
        switch (cmd) {
        case RT_SELECT:
            if (len >= 2) dm_select(msg[1]);
            break;
        case RT_LIST:
            dm_render();
            break;
        case RT_BROADCAST:
            if (len >= 3) dm_broadcast_cmd(msg[1], msg[2]);
            break;
        default: break;
        }
        break;

    default:
        break;
    }
}

// ---------------------------------------------------------------------------
//  FreeRTOS tasks
// ---------------------------------------------------------------------------
static void lvgl_task(void *arg)
{
    (void)arg;
    uint32_t window_start = micros();
    uint32_t busy_us = 0;
    for (;;) {
        uint32_t t0 = micros();
        ui_lock();
        lv_timer_handler();   // partial redraw only, no full-screen refresh
        ui_unlock();
        busy_us += (micros() - t0);

        // Update CPU (render) load roughly once per second.
        uint32_t now = micros();
        uint32_t elapsed = now - window_start;
        if (elapsed >= 1000000UL) {
            int load = (int)((uint64_t)busy_us * 100 / elapsed);
            if (load > 100) load = 100;
            ui_lock();
            ui_set_cpu(load);
            ui_unlock();
            window_start = now;
            busy_us = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(5)); // 5 ms UI tick, no delay() blocking
    }
}

static void comms_task_fn(void *arg)
{
    (void)arg;
    uint32_t hb = millis();
    for (;;) {
        comms_task();   // non-blocking RX/parse/dispatch + noise
        ota_service();  // deferred reboot after OTA
        dm_service();   // expire stale devices

        // Periodic heartbeat / status beacon.
        uint32_t now = millis();
        if (now - hb >= 5000) {
            hb = now;
            comms_send_cmd(CH_STATUS, dm_active(), ST_HEARTBEAT);

            // Link liveness check.
            uint32_t last = comms_last_pong();
            ui_lock();
            if (last == 0 || now - last > 15000) {
                ui_set_status("LINK LOST");
                ui_set_status_color(THEME_COL_ERR);
            } else {
                ui_set_status("LINK OK");
                ui_set_status_color(THEME_COL_OK);
            }
            ui_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

// ---------------------------------------------------------------------------
//  Setup / loop
// ---------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);
    delay(50);

    // LVGL core.
    lv_init();

    // Hardware abstraction: display first (shows UI), then touch.
    hal_display_init();
    hal_touch_init();

    // Build and show the UI immediately ("DISPLAY OK").
    ui_init();

    // Application modules.
    Settings::getInstance().begin();
    stream_init();
    mirror_init();
    ota_init();
    dm_init();

    // Communications (binary protocol over USB serial).
    comms_init(&Serial, 115200);
    comms_set_handler(app_dispatch);
    comms_set_obfuscation(Settings::getInstance().getObfuscation());

    hal_display_backlight(Settings::getInstance().getBrightness());

    // WiFi setup & auto-reconnect.
    WiFi.mode(WIFI_STA);
    String ssid = Settings::getInstance().getWifiSSID();
    String pass = Settings::getInstance().getWifiPass();
    if (ssid.length() > 0) {
        WiFi.begin(ssid.c_str(), pass.c_str());
    }

    randomSeed((uint32_t)esp_random());

    // Tasks: LVGL on core 1, comms on core 0.
    xTaskCreatePinnedToCore(lvgl_task,     "lvgl",  8192, nullptr, 3, nullptr, 1);
    xTaskCreatePinnedToCore(comms_task_fn, "comms", 8192, nullptr, 2, nullptr, 0);
}

void loop()
{
    // All work runs in tasks; keep the Arduino loop idle.
    vTaskDelay(pdMS_TO_TICKS(1000));
}
