/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_riscv64
 * @{
 */
/** @file
 */

#ifndef KERN_riscv64_CYCLE_H_
#define KERN_riscv64_CYCLE_H_

#include <trace.h>

_NO_TRACE static inline uint64_t get_cycle(void)
{
	uint64_t cycle;

	asm volatile (
	    "rdcycle %[cycle]\n"
	    : [cycle] "=r" (cycle)
	);

	return cycle;
}

#endif

/** @}
 */
