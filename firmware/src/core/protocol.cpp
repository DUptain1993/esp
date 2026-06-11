#include "protocol.h"
#include <string.h>

static enum {
    S_WAIT_START, S_GET_CH, S_GET_ID, S_GET_LEN, S_GET_PAYLOAD, S_GET_CHK, S_WAIT_END
} state = S_WAIT_START;

static uint8_t p_idx = 0;
static packet_t rx;

void protocol_init(void) { state = S_WAIT_START; }

uint8_t protocol_calculate_checksum(const packet_t *pkt) {
    uint8_t chk = pkt->channel ^ pkt->device_id ^ pkt->length;
    for (int i = 0; i < pkt->length; i++) chk ^= pkt->payload[i];
    return chk;
}

bool protocol_parse_byte(uint8_t b, packet_t *out) {
    switch (state) {
        case S_WAIT_START: if (b == PKT_START_BYTE) state = S_GET_CH; break;
        case S_GET_CH: rx.channel = b; state = S_GET_ID; break;
        case S_GET_ID: rx.device_id = b; state = S_GET_LEN; break;
        case S_GET_LEN: rx.length = b; p_idx = 0; state = (rx.length == 0) ? S_GET_CHK : S_GET_PAYLOAD; break;
        case S_GET_PAYLOAD: rx.payload[p_idx++] = b; if (p_idx >= rx.length) state = S_GET_CHK; break;
        case S_GET_CHK: rx.checksum = b; state = S_WAIT_END; break;
        case S_WAIT_END:
            state = S_WAIT_START;
            if (b == PKT_END_BYTE && rx.checksum == protocol_calculate_checksum(&rx)) {
                *out = rx; return true;
            }
            break;
    }
    return false;
}

void protocol_build_packet(packet_t *pkt, channel_t ch, uint8_t id, const uint8_t *p, uint8_t len) {
    pkt->channel = (uint8_t)ch; pkt->device_id = id; pkt->length = len;
    if (p && len > 0) memcpy(pkt->payload, p, len);
    pkt->checksum = protocol_calculate_checksum(pkt);
}
