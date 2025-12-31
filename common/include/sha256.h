/*
 * SHA-256 Hash Header
 */

#ifndef SHA256_H
#define SHA256_H

#include <stdint.h>

typedef struct {
    uint8_t data[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t state[8];
} SHA256_CTX;

void SHA256_Init(SHA256_CTX *ctx);
void SHA256_Update(SHA256_CTX *ctx, const uint8_t data[], uint32_t len);
void SHA256_Final(SHA256_CTX *ctx, uint8_t hash[]);
void SHA256_Hash(const uint8_t *data, uint32_t len, uint8_t *hash);

#endif // SHA256_H
