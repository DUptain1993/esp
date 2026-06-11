#ifndef APP_OTA_H
#define APP_OTA_H

#include <lvgl.h>
#include "../core/protocol.h"

void app_ota_init(lv_obj_t *parent);
void app_ota_handle_packet(const packet_t *pkt);

#endif // APP_OTA_H
