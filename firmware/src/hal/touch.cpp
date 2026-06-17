#include "touch.h"
#include "display.h"

static lv_indev_drv_t indev_drv;

// Calibration for the 3.5" ST7796 + XPT2046 panel in rotation 0.
// TFT_eSPI format: {x0, x_span, y0, y_span, flags}
//   flags bit0 = swap X/Y, bit1 = invert X, bit2 = invert Y
// This panel needs only Y inverted (top/bottom flipped); X maps correctly.
static uint16_t touch_cal[5] = {300, 3600, 300, 3600, 0x04};

void hal_touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    uint16_t x = 0, y = 0;

    // getTouch() drives TOUCH_CS internally and returns true while pressed.
    bool pressed = tft.getTouch(&x, &y, 300);

    if (pressed) {
        // Clamp into panel bounds for safety.
        if (x >= HAL_DISP_HOR_RES) x = HAL_DISP_HOR_RES - 1;
        if (y >= HAL_DISP_VER_RES) y = HAL_DISP_VER_RES - 1;
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void hal_touch_init(void)
{
    pinMode(HAL_TOUCH_IRQ_PIN, INPUT); // IRQ line (optional wake / poll hint)

    tft.setTouch(touch_cal);

    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = hal_touch_read;
    lv_indev_drv_register(&indev_drv);
}
