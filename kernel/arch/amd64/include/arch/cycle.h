/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64
 * @{
 */
/** @file
 */

#ifndef KERN_amd64_CYCLE_H_
#define KERN_amd64_CYCLE_H_

#include <trace.h>

_NO_TRACE static inline uint64_t get_cycle(void)
{
	uint32_t lower;
	uint32_t upper;

	asm volatile (
	    "rdtsc\n"
	    : "=a" (lower),
	      "=d" (upper)
	);

	return ((uint64_t) lower) | (((uint64_t) upper) << 32);
}

#endif

/** @}
 */
