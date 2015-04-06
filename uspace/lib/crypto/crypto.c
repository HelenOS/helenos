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

/** @file crypto.c
 * 
 * Cryptographic functions library.
 */

#include <unistd.h>
#include <str.h>
#include <macros.h>
#include <errno.h>
#include <byteorder.h>

#include "crypto.h"

typedef int (*HASH_FUNC)(uint8_t*, size_t, uint8_t*);

#define ceil_uint32(val) (((val) - (uint32_t)(val)) > 0 ? \
	(uint32_t)((val) + 1) : (uint32_t)(val))
#define floor_uint32(val) (((val) - (uint32_t)(val)) < 0 ? \
	(uint32_t)((val) - 1) : (uint32_t)(val))
#define rotl_uint32(val, shift) (((val) << shift) | ((val) >> (32 - shift)))
#define get_at(input, size, i) (i < size ? input[i] : 0)

/**
 * Setup hash function properties for use in crypto functions.
 * 
 * @param hash_sel Hash function selector.
 * @param hash_func Output parameter where hash function pointer is stored.
 * @param hash_length Output parameter for setup result hash length.
 */
static void config_hash_func(hash_func_t hash_sel, HASH_FUNC *hash_func,
	size_t *hash_length)
{
	switch(hash_sel) {
		case HASH_MD5:
			if(hash_func) *hash_func = md5;
			*hash_length = MD5_HASH_LENGTH;
			break;
		case HASH_SHA1:
			if(hash_func) *hash_func = sha1;
			*hash_length = SHA1_HASH_LENGTH;
			break;
	}
}

/**
 * MD5 cryptographic hash function.
 * 
 * @param input Input sequence to be encrypted.
 * @param input_size Size of input sequence.
 * @param hash Output parameter for result hash (32 byte value).
 * 
 * @return EINVAL when input not specified, ENOMEM when pointer for output
 * hash result is not allocated, otherwise EOK.
 */
int md5(uint8_t *input, size_t input_size, uint8_t *hash)
{
	if(!input)
		return EINVAL;
	
	if(!hash)
		return ENOMEM;
	
	// TODO
	
	return EOK;
}

/**
 * SHA-1 cryptographic hash function.
 * 
 * @param input Input sequence to be encrypted.
 * @param input_size Size of input sequence.
 * @param hash Output parameter for result hash (20 byte value).
 * 
 * @return EINVAL when input not specified, ENOMEM when pointer for output
 * hash result is not allocated, otherwise EOK.
 */
int sha1(uint8_t *input, size_t input_size, uint8_t *hash)
{
	if(!input)
		return EINVAL;
	
	if(!hash)
		return ENOMEM;
	
	uint32_t a, b, c, d, e, f, cf, temp;
	uint32_t h[5] = {
		0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0
	};
	
	uint8_t work_input[input_size + 1];
	memcpy(work_input, input, input_size);
	work_input[input_size] = 0x80;
	
	size_t blocks = ceil_uint32((((double)input_size + 1) / 4 + 2) / 16);
	
	uint32_t work_arr[blocks * 16 * sizeof(uint32_t)];
	for(size_t i = 0; i < blocks; i++) {
		for(size_t j = 0; j < 16; j++) {
			work_arr[i*16 + j] = 
			(get_at(work_input, input_size+1, i*64+j*4) << 24) |
			(get_at(work_input, input_size+1, i*64+j*4+1) << 16) |
			(get_at(work_input, input_size+1, i*64+j*4+2) << 8) |
			get_at(work_input, input_size+1, i*64+j*4+3);
		}
	}
	
	work_arr[(blocks - 1) * 16 + 14] = (uint64_t)(input_size * 8) >> 32;
	work_arr[(blocks - 1) * 16 + 15] = (input_size * 8) & 0xFFFFFFFF;

	uint32_t sched_arr[80 * sizeof(uint32_t)];
	for(size_t i = 0; i < blocks; i++) {
		for(size_t k = 0; k < 16; k++) {
			sched_arr[k] = work_arr[i*16 + k];
		}
		
		for(size_t k = 16; k < 80; k++) {
			sched_arr[k] = 
				rotl_uint32(
				sched_arr[k-3] ^
				sched_arr[k-8] ^
				sched_arr[k-14] ^
				sched_arr[k-16], 
				1);
		}
		
		a = h[0]; b = h[1]; c = h[2]; d = h[3]; e = h[4];
		
		for(size_t k = 0; k < 80; k++) {
			if(k < 20) {
				f = (b & c) | (~b & d);
				cf = 0x5A827999;
			} else if(k >= 20 && k < 40) {
				f = b ^ c ^ d;
				cf = 0x6ED9EBA1;
			} else if(k >= 40 && k < 60) {
				f = (b & c) | (b & d) | (c & d);
				cf = 0x8F1BBCDC;
			} else {
				f = b ^ c ^ d;
				cf = 0xCA62C1D6;
			}
				
			temp = (rotl_uint32(a, 5) + f + e + cf + sched_arr[k]) &
				0xFFFFFFFF;
			
			e = d;
			d = c;
			c = rotl_uint32(b, 30);
			b = a;
			a = temp;
		}
		
		h[0] = (h[0] + a) & 0xFFFFFFFF;
		h[1] = (h[1] + b) & 0xFFFFFFFF;
		h[2] = (h[2] + c) & 0xFFFFFFFF;
		h[3] = (h[3] + d) & 0xFFFFFFFF;
		h[4] = (h[4] + e) & 0xFFFFFFFF;
	}
	
	for(size_t i = 0; i < 5; i++) {
		h[i] = uint32_t_be2host(h[i]);
		memcpy(hash + i*sizeof(uint32_t), &h[i], sizeof(uint32_t));
	}
	
	return EOK;
}

/**
 * Hash-based message authentication code using SHA-1 algorithm.
 * 
 * @param key Cryptographic key sequence.
 * @param key_size Size of key sequence.
 * @param msg Message sequence.
 * @param msg_size Size of message sequence.
 * @param hash Output parameter for result hash.
 * @param hash_sel Hash function selector.
 * 
 * @return EINVAL when key or message not specified, ENOMEM when pointer for 
 * output hash result is not allocated, otherwise EOK.
 */
int hmac(uint8_t *key, size_t key_size, uint8_t *msg, size_t msg_size, 
	uint8_t *hash, hash_func_t hash_sel)
{
	if(!key || !msg)
		return EINVAL;
	
	if(!hash)
		return ENOMEM;
	
	size_t hash_length = 0;
	HASH_FUNC hash_func = NULL;
	config_hash_func(hash_sel, &hash_func, &hash_length);
	
	uint8_t work_key[HMAC_BLOCK_LENGTH];
	uint8_t o_key_pad[HMAC_BLOCK_LENGTH];
	uint8_t i_key_pad[HMAC_BLOCK_LENGTH];
	uint8_t temp_hash[hash_length];
	memset(work_key, 0, HMAC_BLOCK_LENGTH);
	
	if(key_size > HMAC_BLOCK_LENGTH) {
		hash_func(key, key_size, work_key);
	} else {
		memcpy(work_key, key, key_size);
	}
	
	for(size_t i = 0; i < HMAC_BLOCK_LENGTH; i++) {
		o_key_pad[i] = work_key[i] ^ 0x5C;
		i_key_pad[i] = work_key[i] ^ 0x36;
	}
	
	uint8_t temp_work[HMAC_BLOCK_LENGTH + msg_size];
	memcpy(temp_work, i_key_pad, HMAC_BLOCK_LENGTH);
	memcpy(temp_work + HMAC_BLOCK_LENGTH, msg, msg_size);
	
	hash_func(temp_work, HMAC_BLOCK_LENGTH + msg_size, temp_hash);
	
	memcpy(temp_work, o_key_pad, HMAC_BLOCK_LENGTH);
	memcpy(temp_work + HMAC_BLOCK_LENGTH, temp_hash, hash_length);
	
	hash_func(temp_work, HMAC_BLOCK_LENGTH + hash_length, hash);
	
	return EOK;
}

/**
 * Password-Based Key Derivation Function 2 as defined in RFC 2898,
 * using HMAC-SHA1 with 4096 iterations and 32 bytes key result used
 * for WPA2.
 * 
 * @param pass Password sequence.
 * @param pass_size Password sequence length.
 * @param salt Salt sequence to be used with password.
 * @param salt_size Salt sequence length.
 * @param hash Output parameter for result hash (32 byte value).
 * @param hash_sel Hash function selector.
 * 
 * @return EINVAL when pass or salt not specified, ENOMEM when pointer for 
 * output hash result is not allocated, otherwise EOK.
 */
int pbkdf2(uint8_t *pass, size_t pass_size, uint8_t *salt, size_t salt_size, 
	uint8_t *hash, hash_func_t hash_sel)
{
	if(!pass || !salt)
		return EINVAL;
	
	if(!hash)
		return ENOMEM;
	
	size_t hash_length = 0;
	config_hash_func(hash_sel, NULL, &hash_length);

	uint8_t work_salt[salt_size + sizeof(uint32_t)];
	memcpy(work_salt, salt, salt_size);
	uint8_t work_hmac[hash_length];
	uint8_t temp_hmac[hash_length];
	uint8_t xor_hmac[hash_length];
	uint8_t temp_hash[hash_length*2];
	
	for(size_t i = 0; i < 2; i++) {
		uint32_t big_i = host2uint32_t_be(i+1);
		memcpy(work_salt + salt_size, &big_i, sizeof(uint32_t));
		hmac(pass, pass_size, work_salt, salt_size + sizeof(uint32_t),
			work_hmac, hash_sel);
		memcpy(xor_hmac, work_hmac, hash_length);
		for(size_t k = 1; k < 4096; k++) {
			memcpy(temp_hmac, work_hmac, hash_length);
			hmac(pass, pass_size, temp_hmac, hash_length, 
				work_hmac, hash_sel);
			for(size_t t = 0; t < hash_length; t++) {
				xor_hmac[t] ^= work_hmac[t];
			}
		}
		memcpy(temp_hash + i*hash_length, xor_hmac, hash_length);
	}
	
	memcpy(hash, temp_hash, PBKDF2_KEY_LENGTH);
	
	return EOK;
}
