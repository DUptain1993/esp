#include "device_manager.h"
#include "../core/comms.h"
#include "../core/commands.h"
#include "../ui/ui.h"

struct DeviceEntry {
    uint8_t  id;
    uint32_t last_seen;
    bool     used;
};

static DeviceEntry s_devs[DM_MAX_DEVICES];
static uint8_t     s_active = DEVICE_SELF;

#define DM_STALE_MS 30000U

void dm_init(void)
{
    for (int i = 0; i < DM_MAX_DEVICES; i++) {
        s_devs[i].used = false;
        s_devs[i].id = 0;
        s_devs[i].last_seen = 0;
    }
    s_active = DEVICE_SELF;
    // Register self.
    dm_seen(DEVICE_SELF);
}

static DeviceEntry *find(uint8_t id)
{
    for (int i = 0; i < DM_MAX_DEVICES; i++)
        if (s_devs[i].used && s_devs[i].id == id) return &s_devs[i];
    return nullptr;
}

void dm_seen(uint8_t id)
{
    if (id == DEVICE_BROADCAST) return;
    DeviceEntry *e = find(id);
    if (e) {
        e->last_seen = millis();
        return;
    }
    for (int i = 0; i < DM_MAX_DEVICES; i++) {
        if (!s_devs[i].used) {
            s_devs[i].used = true;
            s_devs[i].id = id;
            s_devs[i].last_seen = millis();
            dm_render();
            return;
        }
    }
}

bool dm_select(uint8_t id)
{
    if (id != DEVICE_BROADCAST && !find(id)) return false;
    s_active = id;
    ui_lock();
    ui_set_active_device_label(id);
    ui_unlock();
    dm_render();
    return true;
}

uint8_t dm_active(void) { return s_active; }

uint8_t dm_count(void)
{
    uint8_t n = 0;
    for (int i = 0; i < DM_MAX_DEVICES; i++) if (s_devs[i].used) n++;
    return n;
}

void dm_broadcast_cmd(uint8_t channel, uint8_t cmd)
{
    comms_send_cmd(channel, DEVICE_BROADCAST, cmd);
}

void dm_render(void)
{
    uint32_t now = millis();
    ui_lock();
    ui_devices_clear();
    for (int i = 0; i < DM_MAX_DEVICES; i++) {
        if (!s_devs[i].used) continue;
        uint32_t age = (now - s_devs[i].last_seen) / 1000U;
        ui_devices_add(s_devs[i].id, s_devs[i].id == s_active, age);
    }
    ui_set_active_device_label(s_active);
    ui_unlock();
}

void dm_service(void)
{
    uint32_t now = millis();
    bool changed = false;
    for (int i = 0; i < DM_MAX_DEVICES; i++) {
        if (!s_devs[i].used) continue;
        if (s_devs[i].id == DEVICE_SELF) continue; // never expire self
        if (now - s_devs[i].last_seen > DM_STALE_MS) {
            s_devs[i].used = false;
            changed = true;
        }
    }
    if (changed) dm_render();
}
