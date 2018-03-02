/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

/** @addtogroup generic
 * @{
 */
/** @file
 */

#ifndef LIBC_BITOPS_H_
#define LIBC_BITOPS_H_

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
