#ifndef APP_STREAM_H
#define APP_STREAM_H

#include <lvgl.h>
#include "../core/protocol.h"

void app_stream_init(lv_obj_t *parent);
void app_stream_handle_packet(const packet_t *pkt);

#endif // APP_STREAM_H
