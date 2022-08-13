/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <memstr.h>
#include <stddef.h>
#include <stdint.h>

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
