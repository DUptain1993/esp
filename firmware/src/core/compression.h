#ifndef CORE_COMPRESSION_H
#define CORE_COMPRESSION_H

#include <stdint.h>
#include <stddef.h>

size_t compression_rle_encode(const uint8_t *src, size_t src_len, uint8_t *dst);
size_t compression_rle_decode(const uint8_t *src, size_t src_len, uint8_t *dst);

void compression_delta_encode(uint8_t *data, size_t len, uint8_t *prev);
void compression_delta_decode(uint8_t *data, size_t len, uint8_t *prev);

#endif // CORE_COMPRESSION_H
