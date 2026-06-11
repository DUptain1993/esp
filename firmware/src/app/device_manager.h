#ifndef APP_DEVICE_MANAGER_H
#define APP_DEVICE_MANAGER_H

#include <lvgl.h>
#include "../core/protocol.h"

void app_device_manager_init(lv_obj_t *parent);
void app_device_manager_handle_packet(const packet_t *pkt);
uint8_t app_device_manager_get_active_id(void);

#endif // APP_DEVICE_MANAGER_H
