/*
 * Copyright 2009 Colin Percival, 2011 ArtForz, 2012-2013 pooler
 * All rights reserved.
 */

#ifndef BITCOIN_CRYPTO_SCRYPT_H
#define BITCOIN_CRYPTO_SCRYPT_H

#include <stdint.h>
#include <stdlib.h>

static const int SCRYPT_SCRATCHPAD_SIZE = 131072 + 63;

void scrypt_1024_1_1_256(const uint8_t *input, uint8_t *output);
void scrypt_1024_1_1_256_sp_generic(const uint8_t *input, uint8_t *output,
                                    uint8_t *scratchpad);

#if defined(USE_SSE2)

#if defined(_M_X64) || defined(__x86_64__) || defined(_M_AMD64) ||             \
    (defined(MAC_OSX) && defined(__i386__))
#define USE_SSE2_ALWAYS 1
#define scrypt_1024_1_1_256_sp(input, output, scratchpad)                      \
    scrypt_1024_1_1_256_sp_sse2((input), (output), (scratchpad))
#else
#define scrypt_1024_1_1_256_sp(input, output, scratchpad)                      \
    scrypt_1024_1_1_256_sp_detected((input), (output), (scratchpad))
#endif

void scrypt_detect_sse2();
void scrypt_1024_1_1_256_sp_sse2(const uint8_t *input, uint8_t *output,
                                 uint8_t *scratchpad);
extern void (*scrypt_1024_1_1_256_sp_detected)(const uint8_t *input,
                                               uint8_t *output,
                                               uint8_t *scratchpad);
#else
#define scrypt_1024_1_1_256_sp(input, output, scratchpad)                      \
    scrypt_1024_1_1_256_sp_generic((input), (output), (scratchpad))

#endif // defined(USE_SSE2)

void PBKDF2_SHA256(const uint8_t *passwd, size_t passwdlen, const uint8_t *salt,
                   size_t saltlen, uint64_t c, uint8_t *buf, size_t dkLen);

#endif // BITCOIN_CRYPTO_SCRYPT_H
