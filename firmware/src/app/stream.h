#pragma once
#include <Arduino.h>

// Live terminal stream (CH_STREAM). Parses a useful subset of ANSI/VT100
// escape sequences (SGR colours, cursor movement, erase line/screen) and
// renders into the LVGL textarea with a bounded ring buffer + auto-scroll.

void stream_init(void);

// Feed raw stream bytes (already de-framed/decrypted/decompressed).
void stream_feed(const uint8_t *data, size_t len);

// Clear the terminal view.
void stream_clear(void);
