#ifndef HAL_TOUCH_H
#define HAL_TOUCH_H

#include <lvgl.h>
#include <TFT_eSPI.h>

void hal_touch_init(void);
void hal_touch_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);

#endif // HAL_TOUCH_H
