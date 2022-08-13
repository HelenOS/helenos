/*
 * SPDX-FileCopyrightText: 2012 Adam Hraska
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */
#ifndef _LIBC_ADT_HASH_H_
#define _LIBC_ADT_HASH_H_

#include <stdint.h>
#include <types/common.h>

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
#ifdef __32_BITS__
	return hash_mix32(hash);
#elif defined(__64_BITS__)
	return hash_mix64(hash);
#else
#error Unknown size_t size - cannot select proper hash mix function.
#endif
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

#endif
