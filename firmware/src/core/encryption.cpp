#include "encryption.h"

Crypto::Crypto()
{
    _secret = CRYPTO_SHARED_SECRET;
    _key = _secret;
}

void Crypto::begin(uint32_t shared_secret, uint32_t nonce)
{
    _secret = shared_secret;
    _key = _secret ^ nonce;
}

void Crypto::set_nonce(uint32_t nonce)
{
    _key = _secret ^ nonce;
}

void Crypto::evolve()
{
    // 32-bit left rotate by 1.
    _key = (_key << 1) | (_key >> 31);
}

void Crypto::process(uint8_t *data, size_t len)
{
    if (!data) return;
    for (size_t i = 0; i < len; i++) {
        // Key byte selected by position within the 32-bit key.
        uint8_t k = (uint8_t)(_key >> ((i % 4) * 8));
        data[i] ^= k;
    }
    // Evolve key for the next packet.
    evolve();
}
