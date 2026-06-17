#pragma once
#include <Arduino.h>
#include "../core/commands.h"

// Mirror mode: renders remote draw-commands / framebuffer diffs to an LVGL
// canvas (graphical mode), or shows incoming text status (terminal mode).

void mirror_init(void);

// Switch between MIRROR_TERMINAL and MIRROR_GRAPHICAL.
void mirror_set_mode(MirrorMode mode);
MirrorMode mirror_get_mode(void);

// Feed a STR_MIRROR payload (sequence of MirrorOp draw commands).
void mirror_feed(const uint8_t *data, size_t len);
