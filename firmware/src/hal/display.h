#pragma once
#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>

// Panel geometry (portrait, ST7796 320x480 RGB565)
#define HAL_DISP_HOR_RES 320
#define HAL_DISP_VER_RES 480

// Double-buffer geometry: 320 x 40 lines per buffer
#define HAL_DISP_BUF_LINES 40
#define HAL_DISP_BUF_PIXELS (HAL_DISP_HOR_RES * HAL_DISP_BUF_LINES)

// Backlight pin (active HIGH)
#define HAL_BL_PIN 27

// Shared TFT instance (also used by the touch HAL on the same SPI bus)
extern TFT_eSPI tft;

// Initialise TFT_eSPI + LVGL display driver with double buffering and DMA.
void hal_display_init(void);

// Backlight control helpers.
void hal_display_backlight(bool on);

// Set backlight brightness 0-100% (PWM on the backlight pin).
void hal_display_set_brightness(uint8_t pct);
uint8_t hal_display_get_brightness(void);

// LVGL flush callback (exposed for testing / registration).
void hal_display_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p);
