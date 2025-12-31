/*
 * Cryptographic Wrapper Header
 */

#ifndef CRYPTO_WRAPPER_H
#define CRYPTO_WRAPPER_H

#include <stdint.h>

void Crypto_SHA256(const uint8_t *data, uint32_t len, uint8_t *hash);
int Crypto_VerifySignature(const uint8_t *message, uint32_t msg_len,
                            const uint8_t *signature, const uint8_t *public_key);

#endif // CRYPTO_WRAPPER_H
