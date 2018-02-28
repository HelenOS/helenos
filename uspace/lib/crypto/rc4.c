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

/** @file rc4.c
 *
 * Implementation of ARC4 symmetric cipher cryptographic algorithm.
 *
 */

#include <errno.h>
#include <mem.h>
#include "crypto.h"

/* Sbox table size. */
#define SBOX_SIZE  256

/** Swap two values in sbox.
 *
 * @param i    First index of value in sbox to be swapped.
 * @param j    Second index of value in sbox to be swapped.
 * @param sbox Sbox to be modified.
 *
 */
static void swap(size_t i, size_t j, uint8_t *sbox)
{
	uint8_t temp = sbox[i];
	sbox[i] = sbox[j];
	sbox[j] = temp;
}

/** Sbox initialization procedure.
 *
 * @param key      Input key.
 * @param key_size Size of key sequence.
 * @param sbox     Place for result sbox.
 *
 */
static void create_sbox(uint8_t *key, size_t key_size, uint8_t *sbox)
{
	for (size_t i = 0; i < SBOX_SIZE; i++)
		sbox[i] = i;

	uint8_t j = 0;
	for (size_t i = 0; i < SBOX_SIZE; i++) {
		j = j + sbox[i] + key[i % key_size];
		swap(i, j, sbox);
	}
}

/** ARC4 encryption/decryption algorithm.
 *
 * @param key        Input key.
 * @param key_size   Size of key sequence.
 * @param input      Input data sequence to be processed.
 * @param input_size Size of input data sequence.
 * @param skip       Number of bytes to be skipped from
 *                   the beginning of key stream.
 * @param output     Result data sequence.
 *
 * @return EINVAL when input or key not specified,
 *         ENOMEM when pointer for output is not allocated,
 *         otherwise EOK.
 *
 */
errno_t rc4(uint8_t *key, size_t key_size, uint8_t *input, size_t input_size,
    size_t skip, uint8_t *output)
{
	if ((!key) || (!input))
		return EINVAL;

	if (!output)
		return ENOMEM;

	/* Initialize sbox. */
	uint8_t sbox[SBOX_SIZE];
	create_sbox(key, key_size, sbox);

	/* Skip first x bytes. */
	uint8_t i = 0;
	uint8_t j = 0;
	for (size_t k = 0; k < skip; k++) {
		i = i + 1;
		j = j + sbox[i];
		swap(i, j, sbox);
	}

	/* Processing loop. */
	uint8_t val;
	for (size_t k = 0; k < input_size; k++) {
		i = i + 1;
		j = j + sbox[i];
		swap(i, j, sbox);
		val = sbox[sbox[i] + sbox[j]];
		output[k] = val ^ input[k];
	}

	return EOK;
}
