#include "comms.h"
#include "encryption.h"
#include "protocol.h"
#include <Arduino.h>

static comms_callback_t callbacks[6]; // 1-5 channels
static unsigned long next_send_time = 0;

void comms_init(void) {
    Serial.begin(115200);
    protocol_init();
}

void comms_register_callback(channel_t channel, comms_callback_t cb) {
    if (channel >= 1 && channel <= 5) {
        callbacks[channel] = cb;
    }
}

void comms_send_packet(channel_t channel, uint8_t device_id, const uint8_t *payload, uint8_t length) {
    packet_t pkt;
    uint8_t temp_payload[255];
    uint8_t final_len = length;

    if (payload && length > 0) {
        memcpy(temp_payload, payload, length);
        
        // Add random padding (1-4 bytes) if there is space
        uint8_t padding_len = random(1, 5);
        if (final_len + padding_len <= 255) {
            for (uint8_t i = 0; i < padding_len; i++) {
                temp_payload[final_len + i] = random(0, 256);
            }
            final_len += padding_len;
        }

        // Encrypt payload
        encryption_process(temp_payload, final_len);
    }

    protocol_build_packet(&pkt, channel, device_id, temp_payload, final_len);

    // Jitter: Wait until next_send_time
    while (millis() < next_send_time) {
        delay(1);
    }

    // Send packet
    Serial.write(PKT_START_BYTE);
    Serial.write(pkt.channel);
    Serial.write(pkt.device_id);
    Serial.write(pkt.length);
    Serial.write(pkt.payload, pkt.length);
    Serial.write(pkt.checksum);
    Serial.write(PKT_END_BYTE);

    // Schedule next jitter (15-90ms)
    next_send_time = millis() + random(15, 91);
}

void comms_update(void) {
    while (Serial.available() > 0) {
        uint8_t b = Serial.read();
        packet_t rx_pkt;
        if (protocol_parse_byte(b, &rx_pkt)) {
            // Decrypt payload
            if (rx_pkt.length > 0) {
                encryption_process(rx_pkt.payload, rx_pkt.length);
            }

            // Route to callback
            if (rx_pkt.channel >= 1 && rx_pkt.channel <= 5 && callbacks[rx_pkt.channel]) {
                callbacks[rx_pkt.channel](&rx_pkt);
            }
        }
    }

    // Periodically send noise packets (CMD=0x00)
    static unsigned long last_noise_time = 0;
    if (millis() - last_noise_time > 5000) {
        uint8_t noise_cmd = 0x00;
        comms_send_packet(CH_CONTROL, 0, &noise_cmd, 1);
        last_noise_time = millis();
    }
}
