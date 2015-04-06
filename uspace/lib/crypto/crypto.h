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

#include <sys/types.h>

#define AES_CIPHER_LENGTH 16
#define MD5_HASH_LENGTH 16
#define SHA1_HASH_LENGTH 20
#define HMAC_BLOCK_LENGTH 64
#define PBKDF2_KEY_LENGTH 32

/** Hash function selector. */
typedef enum {
	HASH_MD5,
	HASH_SHA1
} hash_func_t;

extern int aes_encrypt(uint8_t *key, uint8_t *input, uint8_t *output);
extern int aes_decrypt(uint8_t *key, uint8_t *input, uint8_t *output);
extern int sha1(uint8_t *input, size_t input_size, uint8_t *hash);
extern int md5(uint8_t *input, size_t input_size, uint8_t *hash);
extern int hmac(uint8_t *key, size_t key_size, uint8_t *msg, size_t msg_size, 
	uint8_t *hash, hash_func_t hash_sel);
extern int pbkdf2(uint8_t *pass, size_t pass_size, uint8_t *salt, 
	size_t salt_size, uint8_t *hash, hash_func_t hash_sel);

#endif
