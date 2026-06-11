#include "mirror.h"
#include <Arduino.h>

static lv_obj_t *canvas;
static lv_color_t *cbuf;
#define CW 320
#define CH 320 // Reduced for memory

void app_mirror_init(lv_obj_t *p) {
    cbuf = (lv_color_t *)heap_caps_malloc(CW * CH * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!cbuf) return;
    canvas = lv_canvas_create(p);
    lv_canvas_set_buffer(canvas, cbuf, CW, CH, LV_IMG_CF_TRUE_COLOR);
    lv_obj_center(canvas);
    lv_canvas_fill_bg(canvas, lv_color_hex(0), LV_OPA_COVER);
}

void app_mirror_handle_packet(const packet_t *pkt) {
    if (pkt->channel != CH_STREAM) return;
    uint8_t t = pkt->payload[0];
    if (t == 0x01) { // Rect
        uint16_t x = pkt->payload[1] | (pkt->payload[2] << 8);
        uint16_t y = pkt->payload[3] | (pkt->payload[4] << 8);
        uint16_t w = pkt->payload[5] | (pkt->payload[6] << 8);
        uint16_t h = pkt->payload[7] | (pkt->payload[8] << 8);
        uint16_t c = pkt->payload[9] | (pkt->payload[10] << 8);
        for(int i=x; i<x+w && i<CW; i++)
            for(int j=y; j<y+h && j<CH; j++)
                lv_canvas_set_px(canvas, i, j, lv_color_hex(c));
    } else if (t == 0x03) lv_canvas_fill_bg(canvas, lv_color_hex(0), LV_OPA_COVER);
}
