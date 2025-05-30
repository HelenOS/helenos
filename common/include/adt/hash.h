/*
 * Copyright (c) 2012 Adam Hraska
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

/** @addtogroup libc
 * @{
 */
/** @file
 */
#ifndef _LIBC_ADT_HASH_H_
#define _LIBC_ADT_HASH_H_

#include <stddef.h>
#include <stdint.h>

/** Produces a uniform hash affecting all output bits from the skewed input. */
static inline uint32_t hash_mix32(uint32_t hash)
{
	/*
	 * Thomas Wang's modification of Bob Jenkin's hash mixing function:
	 * http://www.concentric.net/~Ttwang/tech/inthash.htm
	 * Public domain.
	 */
	hash = ~hash + (hash << 15);
	hash = hash ^ (hash >> 12);
	hash = hash + (hash << 2);
	hash = hash ^ (hash >> 4);
	hash = hash * 2057;
	hash = hash ^ (hash >> 16);
	return hash;
}

/** Produces a uniform hash affecting all output bits from the skewed input. */
static inline uint64_t hash_mix64(uint64_t hash)
{
	/*
	 * Thomas Wang's public domain 64-bit hash mixing function:
	 * http://www.concentric.net/~Ttwang/tech/inthash.htm
	 */
	hash = (hash ^ 61) ^ (hash >> 16);
	hash = hash + (hash << 3);
	hash = hash ^ (hash >> 4);
	hash = hash * 0x27d4eb2d;
	hash = hash ^ (hash >> 15);
	/*
	 * Lower order bits are mixed more thoroughly. Swap them with
	 * the higher order bits and make the resulting higher order bits
	 * more usable.
	 */
	return (hash << 32) | (hash >> 32);
}

/** Produces a uniform hash affecting all output bits from the skewed input. */
static inline size_t hash_mix(size_t hash)
{
	if (sizeof(long) == 4)
		return hash_mix32(hash);
	else
		return hash_mix64(hash);
}

/** Use to create a hash from multiple values.
 *
 * Typical usage:
 * @code
 * int car_id;
 * bool car_convertible;
 * // ..
 * size_t hash = 0;
 * hash = hash_combine(hash, car_id);
 * hash = hash_combine(hash, car_convertible);
 * // Now use hash as a hash of both car_id and car_convertible.
 * @endcode
 */
static inline size_t hash_combine(size_t seed, size_t hash)
{
	/*
	 * todo: use Bob Jenkin's proper mixing hash pass:
	 * http://burtleburtle.net/bob/c/lookup3.c
	 */
	seed ^= hash + 0x9e3779b9 +
	    ((seed << 5) | (seed >> (sizeof(size_t) * 8 - 5)));
	return seed;
}

/** Hash a NUL-terminated string.
 * The algorithm may change in the future, so never use it for hashes
 * that will be stored to a file or sent over a network.
 */
static inline size_t hash_string(const char *str)
{
	/* djb2 hash + extra mixing at the end */

	char c;
	size_t hash = 5381;

	while ((c = *(str++)))
		hash = (hash << 5) + hash + c;

	return hash_mix(hash);
}

/** Hash an arbitrarily sized sequence of bytes.
 * The algorithm may change in the future, so never use it for hashes
 * that will be stored to a file or sent over a network.
 */
static inline size_t hash_bytes(const void *b, size_t len)
{
	/* djb2 hash + extra mixing at the end */

	// TODO: work in bigger chunks for faster hashing

	const char *str = b;

	size_t hash = 5381;

	for (size_t i = 0; i < len; i++)
		hash = (hash << 5) + hash + str[i];

	return hash_mix(hash);
}

#endif
