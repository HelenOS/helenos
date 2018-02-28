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

/** @file aes.c
 *
 * Implementation of AES-128 symmetric cipher cryptographic algorithm.
 *
 * Based on FIPS 197.
 */

#include <stdbool.h>
#include <errno.h>
#include <mem.h>
#include "crypto.h"

/* Number of elements in rows/columns in AES arrays. */
#define ELEMS  4

/* Number of elements (words) in cipher. */
#define CIPHER_ELEMS  4

/* Length of AES block. */
#define BLOCK_LEN  16

/* Number of iterations in AES algorithm. */
#define ROUNDS  10

/** Irreducible polynomial used in AES algorithm.
 *
 * NOTE: x^8 + x^4 + x^3 + x + 1.
 *
 */
#define AES_IP  0x1b

/** Precomputed values for AES sub_byte transformation. */
static const uint8_t sbox[BLOCK_LEN][BLOCK_LEN] = {
	{
		0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5,
		0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76
	},
	{
		0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
		0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0
	},
	{
		0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc,
		0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15
	},
	{
		0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a,
		0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75
	},
	{
		0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
		0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84
	},
	{
		0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b,
		0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf
	},
	{
		0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85,
		0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8
	},
	{
		0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
		0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2
	},
	{
		0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17,
		0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73
	},
	{
		0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88,
		0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb
	},
	{
		0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
		0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79
	},
	{
		0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9,
		0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08
	},
	{
		0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6,
		0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a
	},
	{
		0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
		0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e
	},
	{
		0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94,
		0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf
	},
	{
		0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68,
		0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
	}
};

/** Precomputed values for AES inv_sub_byte transformation. */
static uint8_t inv_sbox[BLOCK_LEN][BLOCK_LEN] = {
	{
		0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38,
		0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb
	},
	{
		0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87,
		0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb
	},
	{
		0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d,
		0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e
	},
	{
		0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2,
		0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25
	},
	{
		0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16,
		0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92
	},
	{
		0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda,
		0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84
	},
	{
		0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a,
		0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06
	},
	{
		0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02,
		0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b
	},
	{
		0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea,
		0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73
	},
	{
		0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85,
		0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e
	},
	{
		0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89,
		0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b
	},
	{
		0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20,
		0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4
	},
	{
		0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31,
		0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f
	},
	{
		0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d,
		0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef
	},
	{
		0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0,
		0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61
	},
	{
		0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26,
		0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
	}
};

/** Precomputed values of powers of 2 in GF(2^8) left shifted by 24b. */
static const uint32_t r_con_array[] = {
	0x01000000, 0x02000000, 0x04000000, 0x08000000,
	0x10000000, 0x20000000, 0x40000000, 0x80000000,
	0x1b000000, 0x36000000
};

/** Perform substitution transformation on given byte.
 *
 * @param byte Input byte.
 * @param inv  Flag indicating whether to use inverse table.
 *
 * @return Substituted value.
 *
 */
static uint8_t sub_byte(uint8_t byte, bool inv)
{
	uint8_t i = byte >> 4;
	uint8_t j = byte & 0xF;

	if (!inv)
		return sbox[i][j];

	return inv_sbox[i][j];
}

/** Perform substitution transformation on state table.
 *
 * @param state State table to be modified.
 * @param inv   Flag indicating whether to use inverse table.
 *
 */
static void sub_bytes(uint8_t state[ELEMS][ELEMS], bool inv)
{
	uint8_t val;

	for (size_t i = 0; i < ELEMS; i++) {
		for (size_t j = 0; j < ELEMS; j++) {
			val = state[i][j];
			state[i][j] = sub_byte(val, inv);
		}
	}
}

/** Perform shift rows transformation on state table.
 *
 * @param state State table to be modified.
 *
 */
static void shift_rows(uint8_t state[ELEMS][ELEMS])
{
	uint8_t temp[ELEMS];

	for (size_t i = 1; i < ELEMS; i++) {
		memcpy(temp, state[i], i);
		memcpy(state[i], state[i] + i, ELEMS - i);
		memcpy(state[i] + ELEMS - i, temp, i);
	}
}

/** Perform inverted shift rows transformation on state table.
 *
 * @param state State table to be modified.
 *
 */
static void inv_shift_rows(uint8_t state[ELEMS][ELEMS])
{
	uint8_t temp[ELEMS];

	for (size_t i = 1; i < ELEMS; i++) {
		memcpy(temp, state[i], ELEMS - i);
		memcpy(state[i], state[i] + ELEMS - i, i);
		memcpy(state[i] + i, temp, ELEMS - i);
	}
}

/** Multiplication in GF(2^8).
 *
 * @param x First factor.
 * @param y Second factor.
 *
 * @return Multiplication of given factors in GF(2^8).
 *
 */
static uint8_t galois_mult(uint8_t x, uint8_t y)
{
	uint8_t result = 0;
	uint8_t f_bith;

	for (size_t i = 0; i < 8; i++) {
		if (y & 1)
			result ^= x;

		f_bith = (x & 0x80);
		x <<= 1;

		if (f_bith)
			x ^= AES_IP;

		y >>= 1;
	}

	return result;
}

/** Perform mix columns transformation on state table.
 *
 * @param state State table to be modified.
 *
 */
static void mix_columns(uint8_t state[ELEMS][ELEMS])
{
	uint8_t orig_state[ELEMS][ELEMS];
	memcpy(orig_state, state, BLOCK_LEN);

	for (size_t j = 0; j < ELEMS; j++) {
		state[0][j] =
		    galois_mult(0x2, orig_state[0][j]) ^
		    galois_mult(0x3, orig_state[1][j]) ^
		    orig_state[2][j] ^
		    orig_state[3][j];
		state[1][j] =
		    orig_state[0][j] ^
		    galois_mult(0x2, orig_state[1][j]) ^
		    galois_mult(0x3, orig_state[2][j]) ^
		    orig_state[3][j];
		state[2][j] =
		    orig_state[0][j] ^
		    orig_state[1][j] ^
		    galois_mult(0x2, orig_state[2][j]) ^
		    galois_mult(0x3, orig_state[3][j]);
		state[3][j] =
		    galois_mult(0x3, orig_state[0][j]) ^
		    orig_state[1][j] ^
		    orig_state[2][j] ^
		    galois_mult(0x2, orig_state[3][j]);
	}
}

/** Perform inverted mix columns transformation on state table.
 *
 * @param state State table to be modified.
 *
 */
static void inv_mix_columns(uint8_t state[ELEMS][ELEMS])
{
	uint8_t orig_state[ELEMS][ELEMS];
	memcpy(orig_state, state, BLOCK_LEN);

	for (size_t j = 0; j < ELEMS; j++) {
		state[0][j] =
		    galois_mult(0x0e, orig_state[0][j]) ^
		    galois_mult(0x0b, orig_state[1][j]) ^
		    galois_mult(0x0d, orig_state[2][j]) ^
		    galois_mult(0x09, orig_state[3][j]);
		state[1][j] =
		    galois_mult(0x09, orig_state[0][j]) ^
		    galois_mult(0x0e, orig_state[1][j]) ^
		    galois_mult(0x0b, orig_state[2][j]) ^
		    galois_mult(0x0d, orig_state[3][j]);
		state[2][j] =
		    galois_mult(0x0d, orig_state[0][j]) ^
		    galois_mult(0x09, orig_state[1][j]) ^
		    galois_mult(0x0e, orig_state[2][j]) ^
		    galois_mult(0x0b, orig_state[3][j]);
		state[3][j] =
		    galois_mult(0x0b, orig_state[0][j]) ^
		    galois_mult(0x0d, orig_state[1][j]) ^
		    galois_mult(0x09, orig_state[2][j]) ^
		    galois_mult(0x0e, orig_state[3][j]);
	}
}

/** Perform round key transformation on state table.
 *
 * @param state     State table to be modified.
 * @param round_key Round key to be applied on state table.
 *
 */
static void add_round_key(uint8_t state[ELEMS][ELEMS], uint32_t *round_key)
{
	uint8_t byte_round;
	uint8_t shift;
	uint32_t mask = 0xff;

	for (size_t j = 0; j < ELEMS; j++) {
		for (size_t i = 0; i < ELEMS; i++) {
			shift = 24 - 8 * i;
			byte_round = (round_key[j] & (mask << shift)) >> shift;
			state[i][j] = state[i][j] ^ byte_round;
		}
	}
}

/** Perform substitution transformation on given word.
 *
 * @param byte Input word.
 *
 * @return Substituted word.
 *
 */
static uint32_t sub_word(uint32_t word)
{
	uint32_t temp = word;
	uint8_t *start = (uint8_t *) &temp;

	for (size_t i = 0; i < 4; i++)
		*(start + i) = sub_byte(*(start + i), false);

	return temp;
}

/** Perform left rotation by one byte on given word.
 *
 * @param byte Input word.
 *
 * @return Rotated word.
 *
 */
static uint32_t rot_word(uint32_t word)
{
	return (word << 8 | word >> 24);
}

/** Key expansion procedure for AES algorithm.
 *
 * @param key     Input key.
 * @param key_exp Result key expansion.
 *
 */
static void key_expansion(uint8_t *key, uint32_t *key_exp)
{
	uint32_t temp;

	for (size_t i = 0; i < CIPHER_ELEMS; i++) {
		key_exp[i] =
		    (key[4 * i] << 24) +
		    (key[4 * i + 1] << 16) +
		    (key[4 * i + 2] << 8) +
		    (key[4 * i + 3]);
	}

	for (size_t i = CIPHER_ELEMS; i < ELEMS * (ROUNDS + 1); i++) {
		temp = key_exp[i - 1];

		if ((i % CIPHER_ELEMS) == 0) {
			temp = sub_word(rot_word(temp)) ^
			    r_con_array[i / CIPHER_ELEMS - 1];
		}

		key_exp[i] = key_exp[i - CIPHER_ELEMS] ^ temp;
	}
}

/** AES-128 encryption algorithm.
 *
 * @param key    Input key.
 * @param input  Input data sequence to be encrypted.
 * @param output Encrypted data sequence.
 *
 * @return EINVAL when input or key not specified,
 *         ENOMEM when pointer for output is not allocated,
 *         otherwise EOK.
 *
 */
errno_t aes_encrypt(uint8_t *key, uint8_t *input, uint8_t *output)
{
	if ((!key) || (!input))
		return EINVAL;

	if (!output)
		return ENOMEM;

	/* Create key expansion. */
	uint32_t key_exp[ELEMS * (ROUNDS + 1)];
	key_expansion(key, key_exp);

	/* Copy input into state array. */
	uint8_t state[ELEMS][ELEMS];
	for (size_t i = 0; i < ELEMS; i++) {
		for (size_t j = 0; j < ELEMS; j++)
			state[i][j] = input[i + ELEMS * j];
	}

	/* Processing loop. */
	add_round_key(state, key_exp);

	for (size_t k = 1; k <= ROUNDS; k++) {
		sub_bytes(state, false);
		shift_rows(state);

		if (k < ROUNDS)
			mix_columns(state);

		add_round_key(state, key_exp + k * ELEMS);
	}

	/* Copy state array into output. */
	for (size_t i = 0; i < ELEMS; i++) {
		for (size_t j = 0; j < ELEMS; j++)
			output[i + j * ELEMS] = state[i][j];
	}

	return EOK;
}

/** AES-128 decryption algorithm.
 *
 * @param key    Input key.
 * @param input  Input data sequence to be decrypted.
 * @param output Decrypted data sequence.
 *
 * @return EINVAL when input or key not specified,
 *         ENOMEM when pointer for output is not allocated,
 *         otherwise EOK.
 *
 */
errno_t aes_decrypt(uint8_t *key, uint8_t *input, uint8_t *output)
{
	if ((!key) || (!input))
		return EINVAL;

	if (!output)
		return ENOMEM;

	/* Create key expansion. */
	uint32_t key_exp[ELEMS * (ROUNDS + 1)];
	key_expansion(key, key_exp);

	/* Copy input into state array. */
	uint8_t state[ELEMS][ELEMS];
	for (size_t i = 0; i < ELEMS; i++) {
		for (size_t j = 0; j < ELEMS; j++)
			state[i][j] = input[i + ELEMS * j];
	}

	/* Processing loop. */
	add_round_key(state, key_exp + ROUNDS * ELEMS);

	for (int k = ROUNDS - 1; k >= 0; k--) {
		inv_shift_rows(state);
		sub_bytes(state, true);
		add_round_key(state, key_exp + k * ELEMS);

		if (k > 0)
			inv_mix_columns(state);
	}

	/* Copy state array into output. */
	for (size_t i = 0; i < ELEMS; i++) {
		for (size_t j = 0; j < ELEMS; j++)
			output[i + j * ELEMS] = state[i][j];
	}

	return EOK;
}
