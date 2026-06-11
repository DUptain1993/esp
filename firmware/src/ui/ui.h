#ifndef UI_UI_H
#define UI_UI_H

#include <lvgl.h>

void ui_init(void);
void ui_update_status(const char *device, const char *status, int cpu_load);

#endif // UI_UI_H
