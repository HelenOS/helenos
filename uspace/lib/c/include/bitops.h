/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_BITOPS_H_
#define _LIBC_BITOPS_H_

#include <stddef.h>
#include <stdint.h>

/** Mask with bit @a n set. */
#define BIT_V(type, n) \
    ((type) 1 << (n))

/** Mask with rightmost @a n bits set. */
#define BIT_RRANGE(type, n) \
    (BIT_V(type, (n)) - 1)

/** Mask with bits @a hi .. @a lo set. @a hi >= @a lo. */
#define BIT_RANGE(type, hi, lo) \
    (BIT_RRANGE(type, (hi) - (lo) + 1) << (lo))

/** Extract range of bits @a hi .. @a lo from @a value. */
#define BIT_RANGE_EXTRACT(type, hi, lo, value) \
    (((value) >> (lo)) & BIT_RRANGE(type, (hi) - (lo) + 1))

/** Insert @a value between bits @a hi .. @a lo. */
#define BIT_RANGE_INSERT(type, hi, lo, value) \
    (((value) & BIT_RRANGE(type, (hi) - (lo) + 1)) << (lo))

/** Return position of first non-zero bit from left (i.e. [log_2(arg)]).
 *
 * If number is zero, it returns 0
 */
static inline unsigned int fnzb32(uint32_t arg)
{
	unsigned int n = 0;

	if (arg >> 16) {
		arg >>= 16;
		n += 16;
	}

	if (arg >> 8) {
		arg >>= 8;
		n += 8;
	}

	if (arg >> 4) {
		arg >>= 4;
		n += 4;
	}

	if (arg >> 2) {
		arg >>= 2;
		n += 2;
	}

	if (arg >> 1) {
		arg >>= 1;
		n += 1;
	}

	return n;
}

static inline unsigned int fnzb64(uint64_t arg)
{
	unsigned int n = 0;

	if (arg >> 32) {
		arg >>= 32;
		n += 32;
	}

	return (n + fnzb32((uint32_t) arg));
}

static inline unsigned int fnzb(size_t arg)
{
	return fnzb64(arg);
}

#endif

/** @}
 */
