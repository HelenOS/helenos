/*
 * SPDX-FileCopyrightText: 2015 Jan Kolarik
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCRYPTO_H
#define LIBCRYPTO_H

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#define AES_CIPHER_LENGTH  16
#define PBKDF2_KEY_LENGTH  32

/* Left rotation for uint32_t. */
#define rotl_uint32(val, shift) \
	(((val) << shift) | ((val) >> (32 - shift)))

/* Right rotation for uint32_t. */
#define rotr_uint32(val, shift) \
	(((val) >> shift) | ((val) << (32 - shift)))

/** Hash function selector and also result hash length indicator. */
typedef enum {
	HASH_MD5 =  16,
	HASH_SHA1 = 20
} hash_func_t;

extern errno_t rc4(uint8_t *, size_t, uint8_t *, size_t, size_t, uint8_t *);
extern errno_t aes_encrypt(uint8_t *, uint8_t *, uint8_t *);
extern errno_t aes_decrypt(uint8_t *, uint8_t *, uint8_t *);
extern errno_t create_hash(uint8_t *, size_t, uint8_t *, hash_func_t);
extern errno_t hmac(uint8_t *, size_t, uint8_t *, size_t, uint8_t *, hash_func_t);
extern errno_t pbkdf2(uint8_t *, size_t, uint8_t *, size_t, uint8_t *);

extern uint16_t crc16_ibm(uint16_t crc, uint8_t *buf, size_t len);

#endif
