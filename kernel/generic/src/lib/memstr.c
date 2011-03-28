/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup generic
 * @{
 */

/**
 * @file
 * @brief Memory string operations.
 *
 * This file provides architecture independent functions to manipulate blocks
 * of memory. These functions are optimized as much as generic functions of
 * this type can be.
 */

#include <memstr.h>
#include <typedefs.h>
#include <align.h>

/** Fill block of memory.
 *
 * Fill cnt bytes at dst address with the value val.
 *
 * @param dst Destination address to fill.
 * @param cnt Number of bytes to fill.
 * @param val Value to fill.
 *
 */
void memsetb(void *dst, size_t cnt, uint8_t val)
{
	__builtin_memset(dst, val, cnt);
}

/** Fill block of memory.
 *
 * Fill cnt words at dst address with the value val. The filling
 * is done word-by-word.
 *
 * @param dst Destination address to fill.
 * @param cnt Number of words to fill.
 * @param val Value to fill.
 *
 */
void memsetw(void *dst, size_t cnt, uint16_t val)
{
	size_t i;
	uint16_t *ptr = (uint16_t *) dst;
	
	for (i = 0; i < cnt; i++)
		ptr[i] = val;
}

/** Fill block of memory.
 *
 * Fill cnt bytes at dst address with the value val.
 *
 * @param dst Destination address to fill.
 * @param val Value to fill.
 * @param cnt Number of bytes to fill.
 *
 * @return Destination address.
 *
 */
void *memset(void *dst, int val, size_t cnt)
{
	return __builtin_memset(dst, val, cnt);
}

/** Move memory block without overlapping.
 *
 * Copy cnt bytes from src address to dst address. The source
 * and destination memory areas cannot overlap.
 *
 * @param dst Destination address to copy to.
 * @param src Source address to copy from.
 * @param cnt Number of bytes to copy.
 *
 * @return Destination address.
 *
 */
void *memcpy(void *dst, const void *src, size_t cnt)
{
	return __builtin_memcpy(dst, src, cnt);
}

/** Move memory block with possible overlapping.
 *
 * Copy cnt bytes from src address to dst address. The source
 * and destination memory areas may overlap.
 *
 * @param dst Destination address to copy to.
 * @param src Source address to copy from.
 * @param cnt Number of bytes to copy.
 *
 * @return Destination address.
 *
 */
void *memmove(void *dst, const void *src, size_t cnt)
{
	/* Nothing to do? */
	if (src == dst)
		return dst;
	
	/* Non-overlapping? */
	if ((dst >= src + cnt) || (src >= dst + cnt))
		return memcpy(dst, src, cnt);
	
	const uint8_t *sp;
	uint8_t *dp;
	
	/* Which direction? */
	if (src > dst) {
		/* Forwards. */
		sp = src;
		dp = dst;
		
		while (cnt-- != 0)
			*dp++ = *sp++;
	} else {
		/* Backwards. */
		sp = src + (cnt - 1);
		dp = dst + (cnt - 1);
		
		while (cnt-- != 0)
			*dp-- = *sp--;
	}
	
	return dst;
}

/** @}
 */
