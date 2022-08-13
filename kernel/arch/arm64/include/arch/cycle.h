/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm64
 * @{
 */
/** @file
 * @brief Information about a current cycle.
 */

#ifndef KERN_arm64_CYCLE_H_
#define KERN_arm64_CYCLE_H_

#include <arch/asm.h>
#include <trace.h>

/** Get a current cycle.
 *
 * No instruction exists on ARM64 to get the actual CPU cycle. The function
 * instead returns the value of the virtual counter.
 */
_NO_TRACE static inline uint64_t get_cycle(void)
{
	return CNTVCT_EL0_read();
}

#endif

/** @}
 */
