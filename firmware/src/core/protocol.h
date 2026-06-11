#ifndef CORE_PROTOCOL_H
#define CORE_PROTOCOL_H

#include <stdint.h>

#define PKT_START_BYTE 0xAA
#define PKT_END_BYTE   0x55

typedef enum {
    CH_CONTROL = 0x01,
    CH_STATUS  = 0x02,
    CH_STREAM  = 0x03,
    CH_OTA     = 0x04,
    CH_ROUTING = 0x05
} channel_t;

typedef struct {
    uint8_t channel;
    uint8_t device_id;
    uint8_t length;
    uint8_t payload[255];
    uint8_t checksum;
} packet_t;

void protocol_init(void);
uint8_t protocol_calculate_checksum(const packet_t *pkt);
bool protocol_parse_byte(uint8_t byte, packet_t *out_pkt);
void protocol_build_packet(packet_t *pkt, channel_t channel, uint8_t device_id, const uint8_t *payload, uint8_t length);

#endif
