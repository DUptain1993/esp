#pragma once
#include <Arduino.h>

// Lightweight, symmetric compression primitives.
//
//  - RLE   : [count][value] pairs (count 1..255).
//  - Delta : first byte stored verbatim, then byte-to-byte differences.
//
// All functions are bounds-checked and return the number of output bytes
// written, or 0 on failure (e.g. insufficient output capacity).

namespace compression {

// ---- Run-Length Encoding ----
size_t rle_encode(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_cap);
size_t rle_decode(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_cap);

// ---- Delta encoding (reversible, same length) ----
size_t delta_encode(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_cap);
size_t delta_decode(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_cap);

// Heuristic: should a payload of this size be auto-compressed?
inline bool should_compress(size_t len) { return len >= 32; }

} // namespace compression
