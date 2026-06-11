#include "encryption.h"

static uint32_t current_key = 0;

void encryption_init(uint32_t key) { current_key = key; }

uint32_t encryption_derive_key(uint32_t nonce) { return SHARED_SECRET ^ nonce; }

void encryption_process(uint8_t *data, size_t length) {
    if (current_key == 0) return;
    for (size_t i = 0; i < length; i++) {
        data[i] ^= (current_key >> ((i % 4) * 8)) & 0xFF;
    }
    current_key = (current_key * 1103515245 + 12345);
}
