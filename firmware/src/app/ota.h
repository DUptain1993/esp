#pragma once
#include <Arduino.h>

// Chunked OTA firmware update over CH_OTA with progress tracking, validation
// and automatic reboot on success. UI shows a progress bar + status messages.

void ota_init(void);

// Begin an update; total = expected firmware size in bytes.
bool ota_begin(uint32_t total);

// Apply a chunk. seq is the expected sequential index (0-based).
bool ota_chunk(uint16_t seq, const uint8_t *data, size_t len);

// Finalise: validate and (on success) schedule a reboot.
bool ota_end(uint32_t expected_size);

// Abort an in-progress update.
void ota_abort(void);

bool ota_active(void);

// Periodic service: performs the deferred reboot after a successful update.
void ota_service(void);
