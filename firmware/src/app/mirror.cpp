#include "mirror.h"
#include <Arduino.h>

static lv_obj_t *canvas;
static lv_color_t *canvas_buf;

#define CANVAS_WIDTH  320
#define CANVAS_HEIGHT 400 // Reduced slightly to fit in memory

void app_mirror_init(lv_obj_t *parent) {
    canvas_buf = (lv_color_t *)heap_caps_malloc(CANVAS_WIDTH * CANVAS_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!canvas_buf) {
        // Fallback or error
        return;
    }

    canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(canvas, canvas_buf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);
    lv_obj_center(canvas);
    lv_canvas_fill_bg(canvas, lv_color_hex(0x000000), LV_OPA_COVER);
}

void app_mirror_handle_packet(const packet_t *pkt) {
    if (pkt->channel != CH_STREAM) return; // Mirroring also uses stream channel or routing?
    // User said "Channel: 0x03" for terminal, and "second stream mode". 
    // Let's use a subtype byte in the payload.
    
    uint8_t type = pkt->payload[0];
    if (type == 0x01) { // Rect
        uint16_t x = pkt->payload[1] | (pkt->payload[2] << 8);
        uint16_t y = pkt->payload[3] | (pkt->payload[4] << 8);
        uint16_t w = pkt->payload[5] | (pkt->payload[6] << 8);
        uint16_t h = pkt->payload[7] | (pkt->payload[8] << 8);
        uint16_t color = pkt->payload[9] | (pkt->payload[10] << 8);
        
        lv_draw_rect_dsc_t rect_dsc;
        lv_draw_rect_dsc_init(&rect_dsc);
        rect_dsc.bg_color = lv_color_chroma_key(); // Use raw color
        // Actually lv_canvas_draw_rect is better
        for(int i=x; i<x+w && i<CANVAS_WIDTH; i++) {
            for(int j=y; j<y+h && j<CANVAS_HEIGHT; j++) {
                lv_canvas_set_px(canvas, i, j, lv_color_hex(color));
            }
        }
    } else if (type == 0x03) { // Clear
        lv_canvas_fill_bg(canvas, lv_color_hex(0x000000), LV_OPA_COVER);
    }
}
