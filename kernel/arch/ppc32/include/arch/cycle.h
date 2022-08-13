/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ppc32
 * @{
 */
/** @file
 */

#ifndef KERN_ppc32_CYCLE_H_
#define KERN_ppc32_CYCLE_H_

#include <trace.h>

_NO_TRACE static inline uint64_t get_cycle(void)
{
	uint32_t lower;
	uint32_t upper;
	uint32_t tmp;

	do {
		asm volatile (
		    "mftbu %[upper]\n"
		    "mftb %[lower]\n"
		    "mftbu %[tmp]\n"
		    : [upper] "=r" (upper),
		      [lower] "=r" (lower),
		      [tmp] "=r" (tmp)
		);
	} while (upper != tmp);

	return ((uint64_t) upper << 32) + (uint64_t) lower;
}

#endif

/** @}
 */
