/*
 * SPDX-FileCopyrightText: 2011 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */

/**
 * @file
 * @brief Memory string functions.
 *
 * This file provides architecture independent functions to manipulate blocks
 * of memory. These functions are optimized as much as generic functions of
 * this type can be.
 */

#include <lib/memfnc.h>
#include <typedefs.h>

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
	uint8_t *dp = (uint8_t *) dst;

	while (cnt-- != 0)
		*dp++ = val;

	return dst;
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
	uint8_t *dp = (uint8_t *) dst;
	const uint8_t *sp = (uint8_t *) src;

	while (cnt-- != 0)
		*dp++ = *sp++;

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
