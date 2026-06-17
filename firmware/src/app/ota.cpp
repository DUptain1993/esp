#include "ota.h"
#include "../ui/ui.h"
#include "../ui/theme.h"
#include "../core/comms.h"
#include "../core/commands.h"
#include <Update.h>

static bool     s_active = false;
static uint32_t s_total = 0;
static uint32_t s_written = 0;
static uint16_t s_next_seq = 0;
static bool     s_reboot_pending = false;
static uint32_t s_reboot_at = 0;

static void ui_progress(int pct, const char *msg, lv_color_t col)
{
    ui_lock();
    ui_show_page(PAGE_SYSTEM);
    ui_ota_set_progress(pct, msg);
    ui_set_status("OTA");
    ui_set_status_color(col);
    ui_unlock();
}

static void ota_report_progress(int pct)
{
    uint8_t m[2] = {OTA_PROGRESS, (uint8_t)pct};
    comms_send(CH_OTA, DEVICE_SELF, m, 2, true, false);
}

static void ota_report_fail(void) { comms_send_cmd(CH_OTA, DEVICE_SELF, OTA_FAIL); }
static void ota_report_done(void) { comms_send_cmd(CH_OTA, DEVICE_SELF, OTA_DONE); }

void ota_init(void)
{
    s_active = false;
    s_total = 0;
    s_written = 0;
    s_next_seq = 0;
    s_reboot_pending = false;
}

bool ota_begin(uint32_t total)
{
    if (s_active) Update.abort();

    s_total = total;
    s_written = 0;
    s_next_seq = 0;

    uint32_t space = (total == 0) ? UPDATE_SIZE_UNKNOWN : total;
    if (!Update.begin(space)) {
        s_active = false;
        ui_progress(0, "begin failed", THEME_COL_ERR);
        ota_report_fail();
        return false;
    }
    s_active = true;
    ui_progress(0, "update started", THEME_COL_WARN);
    ota_report_progress(0);
    return true;
}

bool ota_chunk(uint16_t seq, const uint8_t *data, size_t len)
{
    if (!s_active || !data || len == 0) return false;

    // Validate sequence ordering.
    if (seq != s_next_seq) {
        ui_progress((int)((uint64_t)s_written * 100 / (s_total ? s_total : 1)),
                    "seq error", THEME_COL_ERR);
        ota_report_fail();
        return false;
    }

    size_t w = Update.write((uint8_t *)data, len);
    if (w != len) {
        ota_abort();
        ui_progress(0, "write failed", THEME_COL_ERR);
        ota_report_fail();
        return false;
    }

    s_written += w;
    s_next_seq++;

    int pct = s_total ? (int)((uint64_t)s_written * 100 / s_total) : 0;
    static int last_report = -1;
    if (pct != last_report) {
        ota_report_progress(pct);
        last_report = pct;
    }

    char msg[40];
    snprintf(msg, sizeof(msg), "%u / %u bytes", (unsigned)s_written, (unsigned)s_total);
    ui_progress(pct, msg, THEME_COL_WARN);
    return true;
}

bool ota_end(uint32_t expected_size)
{
    if (!s_active) return false;

    if (expected_size != 0 && s_written != expected_size) {
        ota_abort();
        ui_progress(0, "size mismatch", THEME_COL_ERR);
        ota_report_fail();
        return false;
    }

    if (!Update.end(true)) { // true = set the size to the written bytes
        s_active = false;
        ui_progress(0, "validation failed", THEME_COL_ERR);
        ota_report_fail();
        return false;
    }

    s_active = false;
    ui_progress(100, "complete - rebooting", THEME_COL_OK);
    ota_report_done();
    s_reboot_pending = true;
    s_reboot_at = millis() + 1200; // brief delay so the UI can render
    return true;
}

void ota_abort(void)
{
    if (s_active) Update.abort();
    s_active = false;
    s_written = 0;
    s_next_seq = 0;
    ota_report_fail();
}

bool ota_active(void) { return s_active; }

// Called from the comms task pump to perform the deferred reboot.
void ota_service(void)
{
    if (s_reboot_pending && (int32_t)(millis() - s_reboot_at) >= 0) {
        s_reboot_pending = false;
        ESP.restart();
    }
}
