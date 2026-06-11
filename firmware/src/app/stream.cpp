#include "stream.h"
#include <string.h>

static lv_obj_t *term;

void app_stream_init(lv_obj_t *parent) {
    term = lv_textarea_create(parent);
    lv_obj_set_size(term, LV_PCT(100), LV_PCT(100));
    lv_textarea_set_text(term, "");
    lv_obj_set_style_bg_color(term, lv_color_hex(0x0A0A0F), 0);
    lv_obj_set_style_text_color(term, lv_color_hex(0xE0E0E0), 0);
}

void app_stream_handle_packet(const packet_t *pkt) {
    if (pkt->channel != CH_STREAM) return;
    char buf[256];
    int j = 0;
    bool esc = false;
    for (int i = 0; i < pkt->length; i++) {
        uint8_t c = pkt->payload[i];
        if (c == 0x1B) { esc = true; continue; }
        if (esc) { if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) esc = false; continue; }
        buf[j++] = (char)c;
    }
    buf[j] = '\0';
    lv_textarea_add_text(term, buf);
    lv_obj_scroll_to_y(term, LV_COORD_MAX, LV_ANIM_OFF);
}
