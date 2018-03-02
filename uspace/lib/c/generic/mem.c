/*
 * Copyright (c) 2005 Martin Decky
 * Copyright (c) 2008 Jiri Svoboda
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

#include <mem.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

/** Fill memory block with a constant value. */
void *memset(void *dest, int b, size_t n)
{
	char *pb;
	unsigned long *pw;
	size_t word_size;
	size_t n_words;

	unsigned long pattern;
	size_t i;
	size_t fill;

	/* Fill initial segment. */
	word_size = sizeof(unsigned long);
	fill = word_size - ((uintptr_t) dest & (word_size - 1));
	if (fill > n) fill = n;

	pb = dest;

	i = fill;
	while (i-- != 0)
		*pb++ = b;

	/* Compute remaining size. */
	n -= fill;
	if (n == 0) return dest;

	n_words = n / word_size;
	n = n % word_size;
	pw = (unsigned long *) pb;

	/* Create word-sized pattern for aligned segment. */
	pattern = 0;
	i = word_size;
	while (i-- != 0)
		pattern = (pattern << 8) | (uint8_t) b;

	/* Fill aligned segment. */
	i = n_words;
	while (i-- != 0)
		*pw++ = pattern;

	pb = (char *) pw;

	/* Fill final segment. */
	i = n;
	while (i-- != 0)
		*pb++ = b;

	return dest;
}

struct along {
	unsigned long n;
} __attribute__ ((packed));

static void *unaligned_memcpy(void *dst, const void *src, size_t n)
{
	size_t i, j;
	struct along *adst = dst;
	const struct along *asrc = src;

	for (i = 0; i < n / sizeof(unsigned long); i++)
		adst[i].n = asrc[i].n;

	for (j = 0; j < n % sizeof(unsigned long); j++)
		((unsigned char *) (((unsigned long *) dst) + i))[j] =
		    ((unsigned char *) (((unsigned long *) src) + i))[j];

	return (char *) dst;
}

/** Copy memory block. */
void *memcpy(void *dst, const void *src, size_t n)
{
	size_t i;
	size_t mod, fill;
	size_t word_size;
	size_t n_words;

	const unsigned long *srcw;
	unsigned long *dstw;
	const uint8_t *srcb;
	uint8_t *dstb;

	word_size = sizeof(unsigned long);

	/*
	 * Are source and destination addresses congruent modulo word_size?
	 * If not, use unaligned_memcpy().
	 */

	if (((uintptr_t) dst & (word_size - 1)) !=
	    ((uintptr_t) src & (word_size - 1)))
 		return unaligned_memcpy(dst, src, n);

	/*
	 * mod is the address modulo word size. fill is the length of the
	 * initial buffer segment before the first word boundary.
	 * If the buffer is very short, use unaligned_memcpy(), too.
	 */

	mod = (uintptr_t) dst & (word_size - 1);
	fill = word_size - mod;
	if (fill > n) fill = n;

	/* Copy the initial segment. */

	srcb = src;
	dstb = dst;

	i = fill;
	while (i-- != 0)
		*dstb++ = *srcb++;

	/* Compute remaining length. */

	n -= fill;
	if (n == 0) return dst;

	/* Pointers to aligned segment. */

	dstw = (unsigned long *) dstb;
	srcw = (const unsigned long *) srcb;

	n_words = n / word_size;	/* Number of whole words to copy. */
	n -= n_words * word_size;	/* Remaining bytes at the end. */

	/* "Fast" copy. */
	i = n_words;
	while (i-- != 0)
		*dstw++ = *srcw++;

	/*
	 * Copy the rest.
	 */

	srcb = (const uint8_t *) srcw;
	dstb = (uint8_t *) dstw;

	i = n;
	while (i-- != 0)
		*dstb++ = *srcb++;

	return dst;
}

/** Move memory block with possible overlapping. */
void *memmove(void *dst, const void *src, size_t n)
{
	const uint8_t *sp;
	uint8_t *dp;

	/* Nothing to do? */
	if (src == dst)
		return dst;

	/* Non-overlapping? */
	if (dst >= src + n || src >= dst + n) {
		return memcpy(dst, src, n);
	}

	/* Which direction? */
	if (src > dst) {
		/* Forwards. */
		sp = src;
		dp = dst;

		while (n-- != 0)
			*dp++ = *sp++;
	} else {
		/* Backwards. */
		sp = src + (n - 1);
		dp = dst + (n - 1);

		while (n-- != 0)
			*dp-- = *sp--;
	}

	return dst;
}

/** Compare two memory areas.
 *
 * @param s1  Pointer to the first area to compare.
 * @param s2  Pointer to the second area to compare.
 * @param len Size of the areas in bytes.
 *
 * @return Zero if areas have the same contents. If they differ,
 *	   the sign of the result is the same as the sign of the
 *	   difference of the first pair of different bytes.
 *
 */
int memcmp(const void *s1, const void *s2, size_t len)
{
	uint8_t *u1 = (uint8_t *) s1;
	uint8_t *u2 = (uint8_t *) s2;
	size_t i;

	for (i = 0; i < len; i++) {
		if (*u1 != *u2)
			return (int)(*u1) - (int)(*u2);
		++u1;
		++u2;
	}

	return 0;
}

/** @}
 */
