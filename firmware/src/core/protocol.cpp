#include "protocol.h"
#include <string.h>

typedef enum {
    STATE_WAIT_START,
    STATE_GET_CHANNEL,
    STATE_GET_DEVICE_ID,
    STATE_GET_LEN,
    STATE_GET_PAYLOAD,
    STATE_GET_CHK,
    STATE_WAIT_END
} parse_state_t;

static parse_state_t current_state = STATE_WAIT_START;
static uint8_t payload_idx = 0;
static packet_t rx_pkt;

void protocol_init(void) {
    current_state = STATE_WAIT_START;
}

uint8_t protocol_calculate_checksum(const packet_t *pkt) {
    uint8_t chk = pkt->channel ^ pkt->device_id ^ pkt->length;
    for (uint8_t i = 0; i < pkt->length; i++) {
        chk ^= pkt->payload[i];
    }
    return chk;
}

bool protocol_parse_byte(uint8_t byte, packet_t *out_pkt) {
    switch (current_state) {
        case STATE_WAIT_START:
            if (byte == PKT_START_BYTE) {
                current_state = STATE_GET_CHANNEL;
            }
            break;
        case STATE_GET_CHANNEL:
            rx_pkt.channel = byte;
            current_state = STATE_GET_DEVICE_ID;
            break;
        case STATE_GET_DEVICE_ID:
            rx_pkt.device_id = byte;
            current_state = STATE_GET_LEN;
            break;
        case STATE_GET_LEN:
            rx_pkt.length = byte;
            payload_idx = 0;
            if (rx_pkt.length == 0) {
                current_state = STATE_GET_CHK;
            } else {
                current_state = STATE_GET_PAYLOAD;
            }
            break;
        case STATE_GET_PAYLOAD:
            rx_pkt.payload[payload_idx++] = byte;
            if (payload_idx >= rx_pkt.length) {
                current_state = STATE_GET_CHK;
            }
            break;
        case STATE_GET_CHK:
            rx_pkt.checksum = byte;
            current_state = STATE_WAIT_END;
            break;
        case STATE_WAIT_END:
            current_state = STATE_WAIT_START;
            if (byte == PKT_END_BYTE) {
                if (rx_pkt.checksum == protocol_calculate_checksum(&rx_pkt)) {
                    memcpy(out_pkt, &rx_pkt, sizeof(packet_t));
                    return true;
                }
            }
            break;
    }
    return false;
}

void protocol_build_packet(packet_t *pkt, channel_t channel, uint8_t device_id, const uint8_t *payload, uint8_t length) {
    pkt->channel = (uint8_t)channel;
    pkt->device_id = device_id;
    pkt->length = length;
    if (payload && length > 0) {
        memcpy(pkt->payload, payload, length);
    }
    pkt->checksum = protocol_calculate_checksum(pkt);
}
