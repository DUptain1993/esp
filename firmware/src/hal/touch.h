#pragma once
#include <Arduino.h>
#include <lvgl.h>

// XPT2046 touch controller (CS=GPIO33, IRQ=GPIO36) shared on the TFT SPI bus.
#define HAL_TOUCH_IRQ_PIN 36

// Initialise touch controller and register the LVGL input device.
void hal_touch_init(void);

// LVGL input read callback.
void hal_touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data);
