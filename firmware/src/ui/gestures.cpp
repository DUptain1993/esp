#include "gestures.h"
#include "ui.h"

// Debounce: ignore repeat gestures fired within the same press burst.
static uint32_t s_last_gesture_ms = 0;
#define GESTURE_DEBOUNCE_MS 250

static void gesture_cb(lv_event_t *e)
{
    (void)e;
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) return;

    uint32_t now = lv_tick_get();
    if (now - s_last_gesture_ms < GESTURE_DEBOUNCE_MS) return;

    lv_dir_t dir = lv_indev_get_gesture_dir(indev);
    switch (dir) {
    case LV_DIR_LEFT:  ui_prev_page();    s_last_gesture_ms = now; break;
    case LV_DIR_RIGHT: ui_next_page();    s_last_gesture_ms = now; break;
    case LV_DIR_TOP:   ui_quick_action(); s_last_gesture_ms = now; break;
    default: break;
    }
}

static void longpress_cb(lv_event_t *e)
{
    (void)e;
    ui_execute();
}

void gestures_init(lv_obj_t *scr)
{
    if (!scr) return;
    lv_obj_add_event_cb(scr, gesture_cb, LV_EVENT_GESTURE, nullptr);
    lv_obj_add_event_cb(scr, longpress_cb, LV_EVENT_LONG_PRESSED, nullptr);
}
