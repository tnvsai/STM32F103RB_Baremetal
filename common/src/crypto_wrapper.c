/*
 * Cryptographic Wrapper for Secure Boot
 * 
 * High-level API for ECDSA signature verification and SHA-256 hashing
 */

#include "crypto_wrapper.h"
#include "sha256.h"
#include "uECC.h"
#include <string.h>

void Crypto_SHA256(const uint8_t *data, uint32_t len, uint8_t *hash) {
    SHA256_Hash(data, len, hash);
}

int Crypto_VerifySignature(const uint8_t *message, uint32_t msg_len,
                            const uint8_t *signature, const uint8_t *public_key) {
    
    const struct uECC_Curve_t *curve = uECC_secp256r1();
    int result = uECC_verify(public_key, message, msg_len, signature, curve);
    
    return result;
}
