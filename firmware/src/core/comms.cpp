#include "comms.h"
#include "encryption.h"
#include <Arduino.h>

static comms_callback_t callbacks[6] = {0};
static unsigned long next_jitter = 0;

void comms_init(void) {
    Serial.begin(115200);
    protocol_init();
}

void comms_register_callback(channel_t ch, comms_callback_t cb) {
    if (ch >= 1 && ch <= 5) callbacks[ch] = cb;
}

void comms_send_packet(channel_t ch, uint8_t id, const uint8_t *p, uint8_t len) {
    packet_t pkt;
    uint8_t buf[255];
    uint8_t final_len = len;

    if (p && len > 0) {
        memcpy(buf, p, len);
        // Add random padding
        uint8_t pad = random(1, 5);
        if (final_len + pad <= 255) {
            for (int i = 0; i < pad; i++) buf[final_len + i] = random(256);
            final_len += pad;
        }
        encryption_process(buf, final_len);
    }

    protocol_build_packet(&pkt, ch, id, buf, final_len);

    while (millis() < next_jitter) delay(1);

    Serial.write(PKT_START_BYTE);
    Serial.write(pkt.channel);
    Serial.write(pkt.device_id);
    Serial.write(pkt.length);
    Serial.write(pkt.payload, pkt.length);
    Serial.write(pkt.checksum);
    Serial.write(PKT_END_BYTE);

    next_jitter = millis() + random(15, 91);
}

void comms_update(void) {
    while (Serial.available()) {
        uint8_t b = Serial.read();
        packet_t rx_pkt;
        if (protocol_parse_byte(b, &rx_pkt)) {
            if (rx_pkt.length > 0) encryption_process(rx_pkt.payload, rx_pkt.length);
            if (rx_pkt.channel >= 1 && rx_pkt.channel <= 5 && callbacks[rx_pkt.channel]) {
                callbacks[rx_pkt.channel](&rx_pkt);
            }
        }
    }
}
