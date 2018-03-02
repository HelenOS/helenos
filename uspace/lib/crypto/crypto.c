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

#include <str.h>
#include <macros.h>
#include <errno.h>
#include <byteorder.h>
#include "crypto.h"

/** Hash function procedure definition. */
typedef void (*hash_fnc_t)(uint32_t *, uint32_t *);

/** Length of HMAC block. */
#define HMAC_BLOCK_LENGTH  64

/** Ceiling for uint32_t. */
#define ceil_uint32(val) \
	(((val) - (uint32_t) (val)) > 0 ? \
	(uint32_t) ((val) + 1) : (uint32_t) (val))

/** Floor for uint32_t. */
#define floor_uint32(val) \
	(((val) - (uint32_t) (val)) < 0 ? \
	(uint32_t) ((val) - 1) : (uint32_t) (val))

/** Pick value at specified index from array or zero if out of bounds. */
#define get_at(input, size, i) \
	((i) < (size) ? (input[i]) : 0)

/** Init values used in SHA1 and MD5 functions. */
static const uint32_t hash_init[] = {
	0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0
};

/** Shift amount array for MD5 algorithm. */
static const uint32_t md5_shift[] = {
	7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
	5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
	4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
	6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
};

/** Substitution box for MD5 algorithm. */
static const uint32_t md5_sbox[] = {
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
	0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
	0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
	0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
	0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
	0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
	0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
	0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
	0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

/** Working procedure of MD5 cryptographic hash function.
 *
 * @param h         Working array with interim hash parts values.
 * @param sched_arr Input array with scheduled values from input string.
 *
 */
static void md5_proc(uint32_t *h, uint32_t *sched_arr)
{
	uint32_t f, g, temp;
	uint32_t w[HASH_MD5 / 4];

	memcpy(w, h, (HASH_MD5 / 4) * sizeof(uint32_t));

	for (size_t k = 0; k < 64; k++) {
		if (k < 16) {
			f = (w[1] & w[2]) | (~w[1] & w[3]);
			g = k;
		} else if ((k >= 16) && (k < 32)) {
			f = (w[1] & w[3]) | (w[2] & ~w[3]);
			g = (5 * k + 1) % 16;
		} else if ((k >= 32) && (k < 48)) {
			f = w[1] ^ w[2] ^ w[3];
			g = (3 * k + 5) % 16;
		} else {
			f = w[2] ^ (w[1] | ~w[3]);
			g = 7 * k % 16;
		}

		temp = w[3];
		w[3] = w[2];
		w[2] = w[1];
		w[1] += rotl_uint32(w[0] + f + md5_sbox[k] +
		    uint32_t_byteorder_swap(sched_arr[g]),
		    md5_shift[k]);
		w[0] = temp;
	}

	for (uint8_t k = 0; k < HASH_MD5 / 4; k++)
		h[k] += w[k];
}

/** Working procedure of SHA-1 cryptographic hash function.
 *
 * @param h         Working array with interim hash parts values.
 * @param sched_arr Input array with scheduled values from input string.
 *
 */
static void sha1_proc(uint32_t *h, uint32_t *sched_arr)
{
	uint32_t f, cf, temp;
	uint32_t w[HASH_SHA1 / 4];

	for (size_t k = 16; k < 80; k++) {
		sched_arr[k] = rotl_uint32(
		    sched_arr[k-3] ^
		    sched_arr[k-8] ^
		    sched_arr[k-14] ^
		    sched_arr[k-16],
		    1);
	}

	memcpy(w, h, (HASH_SHA1 / 4) * sizeof(uint32_t));

	for (size_t k = 0; k < 80; k++) {
		if (k < 20) {
			f = (w[1] & w[2]) | (~w[1] & w[3]);
			cf = 0x5A827999;
		} else if ((k >= 20) && (k < 40)) {
			f = w[1] ^ w[2] ^ w[3];
			cf = 0x6ed9eba1;
		} else if ((k >= 40) && (k < 60)) {
			f = (w[1] & w[2]) | (w[1] & w[3]) | (w[2] & w[3]);
			cf = 0x8f1bbcdc;
		} else {
			f = w[1] ^ w[2] ^ w[3];
			cf = 0xca62c1d6;
		}

		temp = rotl_uint32(w[0], 5) + f + w[4] + cf + sched_arr[k];

		w[4] = w[3];
		w[3] = w[2];
		w[2] = rotl_uint32(w[1], 30);
		w[1] = w[0];
		w[0] = temp;
	}

	for (uint8_t k = 0; k < HASH_SHA1 / 4; k++)
		h[k] += w[k];
}

/** Create hash based on selected algorithm.
 *
 * @param input      Input message byte sequence.
 * @param input_size Size of message sequence.
 * @param output     Result hash byte sequence.
 * @param hash_sel   Hash function selector.
 *
 * @return EINVAL when input not specified,
 *         ENOMEM when pointer for output hash result
 *         is not allocated, otherwise EOK.
 *
 */
errno_t create_hash(uint8_t *input, size_t input_size, uint8_t *output,
    hash_func_t hash_sel)
{
	if (!input)
		return EINVAL;

	if (!output)
		return ENOMEM;

	hash_fnc_t hash_func = (hash_sel == HASH_MD5) ? md5_proc : sha1_proc;

	/* Prepare scheduled input. */
	uint8_t work_input[input_size + 1];
	memcpy(work_input, input, input_size);
	work_input[input_size] = 0x80;

	// FIXME: double?
	size_t blocks = ceil_uint32((((double) input_size + 1) / 4 + 2) / 16);
	uint32_t work_arr[blocks * 16];
	for (size_t i = 0; i < blocks; i++) {
		for (size_t j = 0; j < 16; j++) {
			work_arr[i*16 + j] =
			    (get_at(work_input, input_size + 1, i * 64 + j * 4) << 24) |
			    (get_at(work_input, input_size + 1, i * 64 + j * 4 + 1) << 16) |
			    (get_at(work_input, input_size + 1, i * 64 + j * 4 + 2) << 8) |
			    get_at(work_input, input_size + 1, i * 64 + j * 4 + 3);
		}
	}

	uint64_t bits_size = (uint64_t) (input_size * 8);
	if (hash_sel == HASH_MD5)
		bits_size = uint64_t_byteorder_swap(bits_size);

	work_arr[(blocks - 1) * 16 + 14] = bits_size >> 32;
	work_arr[(blocks - 1) * 16 + 15] = bits_size & 0xffffffff;

	/* Hash computation. */
	uint32_t h[hash_sel / 4];
	memcpy(h, hash_init, (hash_sel / 4) * sizeof(uint32_t));
	uint32_t sched_arr[80];
	for (size_t i = 0; i < blocks; i++) {
		for (size_t k = 0; k < 16; k++)
			sched_arr[k] = work_arr[i * 16 + k];

		hash_func(h, sched_arr);
	}

	/* Copy hash parts into final result. */
	for (size_t i = 0; i < hash_sel / 4; i++) {
		if (hash_sel == HASH_SHA1)
			h[i] = uint32_t_byteorder_swap(h[i]);

		memcpy(output + i * sizeof(uint32_t), &h[i], sizeof(uint32_t));
	}

	return EOK;
}

/** Hash-based message authentication code.
 *
 * @param key      Cryptographic key sequence.
 * @param key_size Size of key sequence.
 * @param msg      Message sequence.
 * @param msg_size Size of message sequence.
 * @param hash     Output parameter for result hash.
 * @param hash_sel Hash function selector.
 *
 * @return EINVAL when key or message not specified,
 *         ENOMEM when pointer for output hash result
 *         is not allocated, otherwise EOK.
 *
 */
errno_t hmac(uint8_t *key, size_t key_size, uint8_t *msg, size_t msg_size,
    uint8_t *hash, hash_func_t hash_sel)
{
	if ((!key) || (!msg))
		return EINVAL;

	if (!hash)
		return ENOMEM;

	uint8_t work_key[HMAC_BLOCK_LENGTH];
	uint8_t o_key_pad[HMAC_BLOCK_LENGTH];
	uint8_t i_key_pad[HMAC_BLOCK_LENGTH];
	uint8_t temp_hash[hash_sel];
	memset(work_key, 0, HMAC_BLOCK_LENGTH);

	if(key_size > HMAC_BLOCK_LENGTH)
		create_hash(key, key_size, work_key, hash_sel);
	else
		memcpy(work_key, key, key_size);

	for (size_t i = 0; i < HMAC_BLOCK_LENGTH; i++) {
		o_key_pad[i] = work_key[i] ^ 0x5c;
		i_key_pad[i] = work_key[i] ^ 0x36;
	}

	uint8_t temp_work[HMAC_BLOCK_LENGTH + max(msg_size, hash_sel)];
	memcpy(temp_work, i_key_pad, HMAC_BLOCK_LENGTH);
	memcpy(temp_work + HMAC_BLOCK_LENGTH, msg, msg_size);

	create_hash(temp_work, HMAC_BLOCK_LENGTH + msg_size, temp_hash,
	    hash_sel);

	memcpy(temp_work, o_key_pad, HMAC_BLOCK_LENGTH);
	memcpy(temp_work + HMAC_BLOCK_LENGTH, temp_hash, hash_sel);

	create_hash(temp_work, HMAC_BLOCK_LENGTH + hash_sel, hash, hash_sel);

	return EOK;
}

/** Password-Based Key Derivation Function 2.
 *
 * As defined in RFC 2898, using HMAC-SHA1 with 4096 iterations
 * and 32 bytes key result used for WPA/WPA2.
 *
 * @param pass      Password sequence.
 * @param pass_size Password sequence length.
 * @param salt      Salt sequence to be used with password.
 * @param salt_size Salt sequence length.
 * @param hash      Output parameter for result hash (32 byte value).
 *
 * @return EINVAL when pass or salt not specified,
 *         ENOMEM when pointer for output hash result
 *         is not allocated, otherwise EOK.
 *
 */
errno_t pbkdf2(uint8_t *pass, size_t pass_size, uint8_t *salt, size_t salt_size,
    uint8_t *hash)
{
	if ((!pass) || (!salt))
		return EINVAL;

	if (!hash)
		return ENOMEM;

	uint8_t work_salt[salt_size + 4];
	memcpy(work_salt, salt, salt_size);
	uint8_t work_hmac[HASH_SHA1];
	uint8_t temp_hmac[HASH_SHA1];
	uint8_t xor_hmac[HASH_SHA1];
	uint8_t temp_hash[HASH_SHA1 * 2];

	for (size_t i = 0; i < 2; i++) {
		uint32_t be_i = host2uint32_t_be(i + 1);

		memcpy(work_salt + salt_size, &be_i, 4);
		hmac(pass, pass_size, work_salt, salt_size + 4,
		    work_hmac, HASH_SHA1);
		memcpy(xor_hmac, work_hmac, HASH_SHA1);

		for (size_t k = 1; k < 4096; k++) {
			memcpy(temp_hmac, work_hmac, HASH_SHA1);
			hmac(pass, pass_size, temp_hmac, HASH_SHA1,
			    work_hmac, HASH_SHA1);

			for (size_t t = 0; t < HASH_SHA1; t++)
				xor_hmac[t] ^= work_hmac[t];
		}

		memcpy(temp_hash + i * HASH_SHA1, xor_hmac, HASH_SHA1);
	}

	memcpy(hash, temp_hash, PBKDF2_KEY_LENGTH);

	return EOK;
}
