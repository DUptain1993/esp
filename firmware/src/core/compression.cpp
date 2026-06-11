#include "compression.h"
#include <string.h>

size_t compression_rle_encode(const uint8_t *src, size_t src_len, uint8_t *dst) {
    if (src_len == 0) return 0;
    size_t dst_idx = 0;
    for (size_t i = 0; i < src_len; ) {
        uint8_t count = 1;
        while (i + count < src_len && src[i + count] == src[i] && count < 255) {
            count++;
        }
        dst[dst_idx++] = count;
        dst[dst_idx++] = src[i];
        i += count;
    }
    return dst_idx;
}

size_t compression_rle_decode(const uint8_t *src, size_t src_len, uint8_t *dst) {
    size_t dst_idx = 0;
    for (size_t i = 0; i < src_len; i += 2) {
        uint8_t count = src[i];
        uint8_t value = src[i + 1];
        for (uint8_t j = 0; j < count; j++) {
            dst[dst_idx++] = value;
        }
    }
    return dst_idx;
}

void compression_delta_encode(uint8_t *data, size_t len, uint8_t *prev) {
    for (size_t i = 0; i < len; i++) {
        uint8_t val = data[i];
        data[i] = val - prev[i];
        prev[i] = val;
    }
}

void compression_delta_decode(uint8_t *data, size_t len, uint8_t *prev) {
    for (size_t i = 0; i < len; i++) {
        data[i] = data[i] + prev[i];
        prev[i] = data[i];
    }
}
