/*
 * Copyright (c) 2007 Michal Kebrt
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
