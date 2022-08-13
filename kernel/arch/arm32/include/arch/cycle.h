/*
 * SPDX-FileCopyrightText: 2007 Michal Kebrt
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32
 * @{
 */
/** @file
 *  @brief Count of CPU cycles.
 */

#ifndef KERN_arm32_CYCLE_H_
#define KERN_arm32_CYCLE_H_

#include <trace.h>
#include <arch/cp15.h>

/** Return count of CPU cycles.
 *
 * No such instruction on ARM to get count of cycles.
 *
 * @return Count of CPU cycles.
 *
 */
_NO_TRACE static inline uint64_t get_cycle(void)
{
#ifdef PROCESSOR_ARCH_armv7_a
	if ((ID_PFR1_read() & ID_PFR1_GEN_TIMER_EXT_MASK) ==
	    ID_PFR1_GEN_TIMER_EXT) {
		uint32_t low = 0, high = 0;
		asm volatile (
		    "MRRC p15, 0, %[low], %[high], c14"
		    : [low] "=r" (low), [high] "=r" (high)
		);
		return ((uint64_t)high << 32) | low;
	} else {
		return (uint64_t)PMCCNTR_read() * 64;
	}
#endif
	return 0;
}

#endif

/** @}
 */
