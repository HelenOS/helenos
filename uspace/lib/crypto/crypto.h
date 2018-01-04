/*
 * Copyright (c) 2015 Jan Kolarik
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
