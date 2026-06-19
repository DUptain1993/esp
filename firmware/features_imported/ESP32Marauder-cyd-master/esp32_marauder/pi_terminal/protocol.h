#pragma once
#include <Arduino.h>
#include "commands.h"

// A fully decoded protocol packet.
struct ProtoPacket {
    uint8_t channel;
    uint8_t device_id;
    uint8_t len;
    uint8_t payload[PROTO_MAX_PAYLOAD];
};

// Callback fired when a valid packet has been parsed.
typedef void (*proto_packet_cb_t)(const ProtoPacket *pkt);

// Streaming, byte-at-a-time state-machine parser with buffer safety.
class ProtocolParser {
public:
    ProtocolParser();

    // Register the handler invoked on a valid frame.
    void on_packet(proto_packet_cb_t cb) { _cb = cb; }

    // Feed one received byte. Returns true when a full valid packet completes.
    bool feed(uint8_t b);

    // Feed a buffer of bytes.
    void feed(const uint8_t *data, size_t n);

    // Statistics.
    uint32_t good_packets() const { return _good; }
    uint32_t bad_packets() const { return _bad; }

    // Reset the state machine.
    void reset();

private:
    enum State : uint8_t {
        S_WAIT_SOF = 0,
        S_CHANNEL,
        S_DEVICE,
        S_LEN,
        S_PAYLOAD,
        S_CHK,
        S_EOF,
    };

    State    _state;
    ProtoPacket _pkt;
    uint16_t _payload_idx;
    uint8_t  _chk_calc;
    uint8_t  _chk_recv;
    proto_packet_cb_t _cb;
    uint32_t _good;
    uint32_t _bad;
};

// Compute the protocol checksum: XOR of CHANNEL + DEVICE_ID + LEN + PAYLOAD.
uint8_t proto_checksum(uint8_t channel, uint8_t device_id, uint8_t len,
                       const uint8_t *payload);

// Build a raw frame into `out` (must hold at least len + 6 bytes).
// Returns total number of bytes written, or 0 on error.
size_t proto_build(uint8_t channel, uint8_t device_id,
                   const uint8_t *payload, uint8_t len,
                   uint8_t *out, size_t out_cap);
