#ifndef CORE_ENCRYPTION_H
#define CORE_ENCRYPTION_H

#include <stdint.h>
#include <stddef.h>

#define SHARED_SECRET 0xDEADC0DE

void encryption_init(uint32_t session_key);
void encryption_process(uint8_t *data, size_t length);
uint32_t encryption_derive_key(uint32_t nonce);

#endif // CORE_ENCRYPTION_H
