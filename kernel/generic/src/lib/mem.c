/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
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

#include <mem.h>
#include <typedefs.h>

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
	memset(dst, val, cnt);
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

	uint8_t *dp;
	const uint8_t *sp;

	/* Which direction? */
	if (src > dst) {
		/* Forwards. */
		dp = dst;
		sp = src;

		while (cnt-- != 0)
			*dp++ = *sp++;
	} else {
		/* Backwards. */
		dp = dst + (cnt - 1);
		sp = src + (cnt - 1);

		while (cnt-- != 0)
			*dp-- = *sp--;
	}

	return dst;
}

/** @}
 */
