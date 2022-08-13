/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_CYCLE_H_
#define KERN_ia32_CYCLE_H_

#include <trace.h>

_NO_TRACE static inline uint64_t get_cycle(void)
{
#ifdef PROCESSOR_i486
	return 0;
#else
	uint64_t v;

	asm volatile (
	    "rdtsc\n"
	    : "=A" (v)
	);

	return v;
#endif
}

#endif

/** @}
 */
