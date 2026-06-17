#pragma once
#include <Arduino.h>

// Lightweight rolling XOR stream cipher.
//
//   session_key = SHARED_SECRET ^ nonce
//   per byte:    data[i] ^= (key >> (i % 4))
//   per packet:  key = (key << 1) | (key >> 31)   (32-bit left rotate)
//
// The transform is symmetric: encrypt() and decrypt() are identical, and the
// key state must evolve in lock-step on both ends.
class Crypto {
public:
    Crypto();

    // Initialise the session key from the shared secret and a nonce.
    void begin(uint32_t shared_secret, uint32_t nonce);

    // Re-seed the nonce (e.g. after a handshake) keeping the same secret.
    void set_nonce(uint32_t nonce);

    // Encrypt/decrypt a buffer in place using the current key, then evolve
    // the key for the next packet.
    void process(uint8_t *data, size_t len);

    // Aliases for readability (both call process()).
    void encrypt(uint8_t *data, size_t len) { process(data, len); }
    void decrypt(uint8_t *data, size_t len) { process(data, len); }

    uint32_t key() const { return _key; }

private:
    uint32_t _secret;
    uint32_t _key;

    void evolve();
};

// Default compile-time shared secret (override per deployment).
#ifndef CRYPTO_SHARED_SECRET
#define CRYPTO_SHARED_SECRET 0xA5C3F19Eu
#endif
