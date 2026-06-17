#pragma once
#include <Arduino.h>

// Multi-device routing: tracks discovered devices by DEVICE_ID, supports an
// active-device selection and broadcasting.

#define DM_MAX_DEVICES 16

void dm_init(void);

// Record activity from a device id (call on every received packet).
void dm_seen(uint8_t id);

// Select the active device (target for control/stream commands).
bool dm_select(uint8_t id);
uint8_t dm_active(void);

// Send a control opcode to every known device (broadcast).
void dm_broadcast_cmd(uint8_t channel, uint8_t cmd);

// Number of known devices.
uint8_t dm_count(void);

// Refresh the devices UI page.
void dm_render(void);

// Periodic service: expires stale devices.
void dm_service(void);
