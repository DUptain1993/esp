#include "stream.h"
#include <string.h>

static lv_obj_t *terminal_ta;

void app_stream_init(lv_obj_t *parent) {
    terminal_ta = lv_textarea_create(parent);
    lv_obj_set_size(terminal_ta, LV_PCT(100), LV_PCT(100));
    lv_textarea_set_cursor_click_pos(terminal_ta, false);
    lv_textarea_set_text(terminal_ta, "");
    lv_obj_set_style_bg_color(terminal_ta, lv_color_hex(0x0A0A0F), 0);
    lv_obj_set_style_text_color(terminal_ta, lv_color_hex(0xE0E0E0), 0);
}

void app_stream_handle_packet(const packet_t *pkt) {
    if (pkt->channel != CH_STREAM) return;

    // Basic ANSI parsing (placeholder for complex logic)
    // For now, just strip ANSI and add text
    char clean_buf[256];
    uint8_t clean_idx = 0;
    bool in_esc = false;

    for (uint8_t i = 0; i < pkt->length; i++) {
        uint8_t c = pkt->payload[i];
        if (c == 0x1B) {
            in_esc = true;
            continue;
        }
        if (in_esc) {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                in_esc = false;
            }
            continue;
        }
        clean_buf[clean_idx++] = (char)c;
    }
    clean_buf[clean_idx] = '\0';

    lv_textarea_add_text(terminal_ta, clean_buf);
    
    // Auto-scroll to bottom
    lv_obj_scroll_to_y(terminal_ta, LV_COORD_MAX, LV_ANIM_OFF);
}
