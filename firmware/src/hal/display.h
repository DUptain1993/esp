#ifndef HAL_DISPLAY_H
#define HAL_DISPLAY_H

#include <lvgl.h>
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

void hal_display_init(void);
void hal_display_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);

#endif // HAL_DISPLAY_H
