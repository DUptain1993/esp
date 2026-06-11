#include "encryption.h"

static uint32_t current_key = 0;

void encryption_init(uint32_t session_key) {
    current_key = session_key;
}

uint32_t encryption_derive_key(uint32_t nonce) {
    return SHARED_SECRET ^ nonce;
}

void encryption_process(uint8_t *data, size_t length) {
    if (current_key == 0) return; // No encryption if key is 0

    for (size_t i = 0; i < length; i++) {
        uint8_t key_byte = (current_key >> ((i % 4) * 8)) & 0xFF;
        data[i] ^= key_byte;
        
        // Evolve key periodically or per byte? Requirements say "Key evolves per packet".
        // Let's evolve it after each full packet processing.
    }
    
    // Evolve key for next packet
    current_key = (current_key * 1103515245 + 12345);
}
