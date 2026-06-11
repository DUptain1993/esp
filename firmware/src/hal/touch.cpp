#include "touch.h"

extern TFT_eSPI tft;

void hal_touch_init(void) {
    /* Initialize the touch driver for LVGL */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = hal_touch_read;
    lv_indev_drv_register(&indev_drv);
}

void hal_touch_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    uint16_t touchX, touchY;
    bool touched = tft.getTouch(&touchX, &touchY, 600); // 600 is threshold

    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touchX;
        data->point.y = touchY;
    }
}
