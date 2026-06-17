#include "protocol.h"

uint8_t proto_checksum(uint8_t channel, uint8_t device_id, uint8_t len,
                       const uint8_t *payload)
{
    uint8_t chk = 0;
    chk ^= channel;
    chk ^= device_id;
    chk ^= len;
    for (uint8_t i = 0; i < len; i++) {
        chk ^= payload[i];
    }
    return chk;
}

size_t proto_build(uint8_t channel, uint8_t device_id,
                   const uint8_t *payload, uint8_t len,
                   uint8_t *out, size_t out_cap)
{
    // Frame overhead: SOF, CH, DEV, LEN, CHK, EOF = 6 bytes.
    const size_t total = (size_t)len + 6;
    if (out == nullptr || out_cap < total) return 0;
    if (len > 0 && payload == nullptr) return 0;

    size_t i = 0;
    out[i++] = PROTO_SOF;
    out[i++] = channel;
    out[i++] = device_id;
    out[i++] = len;
    for (uint8_t p = 0; p < len; p++) out[i++] = payload[p];
    out[i++] = proto_checksum(channel, device_id, len, payload);
    out[i++] = PROTO_EOF;
    return i;
}

ProtocolParser::ProtocolParser()
{
    _cb = nullptr;
    _good = 0;
    _bad = 0;
    reset();
}

void ProtocolParser::reset()
{
    _state = S_WAIT_SOF;
    _payload_idx = 0;
    _chk_calc = 0;
    _chk_recv = 0;
    _pkt.channel = 0;
    _pkt.device_id = 0;
    _pkt.len = 0;
}

void ProtocolParser::feed(const uint8_t *data, size_t n)
{
    if (!data) return;
    for (size_t i = 0; i < n; i++) feed(data[i]);
}

bool ProtocolParser::feed(uint8_t b)
{
    switch (_state) {
    case S_WAIT_SOF:
        if (b == PROTO_SOF) {
            _state = S_CHANNEL;
            _chk_calc = 0;
        }
        break;

    case S_CHANNEL:
        _pkt.channel = b;
        _chk_calc ^= b;
        _state = S_DEVICE;
        break;

    case S_DEVICE:
        _pkt.device_id = b;
        _chk_calc ^= b;
        _state = S_LEN;
        break;

    case S_LEN:
        _pkt.len = b;
        _chk_calc ^= b;
        _payload_idx = 0;
        // LEN is a single byte so it can never exceed PROTO_MAX_PAYLOAD,
        // but guard anyway for buffer safety.
        if (_pkt.len > PROTO_MAX_PAYLOAD) {
            _bad++;
            reset();
            break;
        }
        _state = (_pkt.len == 0) ? S_CHK : S_PAYLOAD;
        break;

    case S_PAYLOAD:
        if (_payload_idx < PROTO_MAX_PAYLOAD) {
            _pkt.payload[_payload_idx] = b;
        }
        _chk_calc ^= b;
        _payload_idx++;
        if (_payload_idx >= _pkt.len) {
            _state = S_CHK;
        }
        break;

    case S_CHK:
        _chk_recv = b;
        _state = S_EOF;
        break;

    case S_EOF:
        if (b == PROTO_EOF && _chk_recv == _chk_calc) {
            _good++;
            if (_cb) _cb(&_pkt);
            reset();
            return true;
        } else {
            _bad++;
            reset();
            // The byte that broke the frame might itself be a new SOF.
            if (b == PROTO_SOF) {
                _state = S_CHANNEL;
                _chk_calc = 0;
            }
        }
        break;

    default:
        reset();
        break;
    }
    return false;
}
