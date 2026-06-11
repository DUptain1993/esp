#ifndef CORE_COMMS_H
#define CORE_COMMS_H

#include "protocol.h"

void comms_init(void);
void comms_update(void);
void comms_send_packet(channel_t channel, uint8_t device_id, const uint8_t *payload, uint8_t length);

typedef void (*comms_callback_t)(const packet_t *pkt);
void comms_register_callback(channel_t channel, comms_callback_t cb);

#endif
