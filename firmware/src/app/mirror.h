#ifndef APP_MIRROR_H
#define APP_MIRROR_H

#include <lvgl.h>
#include "../core/protocol.h"

void app_mirror_init(lv_obj_t *parent);
void app_mirror_handle_packet(const packet_t *pkt);

#endif // APP_MIRROR_H
