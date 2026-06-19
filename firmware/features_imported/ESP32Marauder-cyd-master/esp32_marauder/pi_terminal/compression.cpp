#include "compression.h"

namespace compression {

size_t rle_encode(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_cap)
{
    if (!in || !out) return 0;
    size_t oi = 0;
    size_t i = 0;
    while (i < in_len) {
        uint8_t val = in[i];
        size_t run = 1;
        while (i + run < in_len && in[i + run] == val && run < 255) run++;
        if (oi + 2 > out_cap) return 0; // not enough room
        out[oi++] = (uint8_t)run;
        out[oi++] = val;
        i += run;
    }
    return oi;
}

size_t rle_decode(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_cap)
{
    if (!in || !out) return 0;
    if (in_len % 2 != 0) return 0; // must be count/value pairs
    size_t oi = 0;
    for (size_t i = 0; i + 1 < in_len; i += 2) {
        uint8_t count = in[i];
        uint8_t val = in[i + 1];
        if (oi + count > out_cap) return 0;
        for (uint8_t c = 0; c < count; c++) out[oi++] = val;
    }
    return oi;
}

size_t delta_encode(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_cap)
{
    if (!in || !out) return 0;
    if (out_cap < in_len) return 0;
    if (in_len == 0) return 0;
    out[0] = in[0];
    for (size_t i = 1; i < in_len; i++) {
        out[i] = (uint8_t)(in[i] - in[i - 1]);
    }
    return in_len;
}

size_t delta_decode(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_cap)
{
    if (!in || !out) return 0;
    if (out_cap < in_len) return 0;
    if (in_len == 0) return 0;
    out[0] = in[0];
    for (size_t i = 1; i < in_len; i++) {
        out[i] = (uint8_t)(out[i - 1] + in[i]);
    }
    return in_len;
}

} // namespace compression
