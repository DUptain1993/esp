#pragma once
#include <lvgl.h>

// Touch gesture + long-press handling, decoupled from page construction.
//
//   Swipe LEFT  -> previous screen
//   Swipe RIGHT -> next screen
//   Swipe UP    -> quick action
//   LONG PRESS  -> execute
//
// Attaches LV_EVENT_GESTURE and LV_EVENT_LONG_PRESSED handlers to `scr`.
void gestures_init(lv_obj_t *scr);
