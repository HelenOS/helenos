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
#include <sys/types.h>

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
 * @param len Size of the first area in bytes. Both areas must have
 *            the same length.
 *
 * @return If len is 0, return zero. If the areas match, return
 *         zero. Otherwise return non-zero.
 *
 */
int bcmp(const void *s1, const void *s2, size_t len)
{
	uint8_t *u1 = (uint8_t *) s1;
	uint8_t *u2 = (uint8_t *) s2;
	
	for (; (len != 0) && (*u1++ == *u2++); len--);
	
	return len;
}

/** @}
 */
