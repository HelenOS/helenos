/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_BARRIER_H_
#define KERN_sparc64_BARRIER_H_

#include <trace.h>

/** Flush Instruction pipeline. */
_NO_TRACE static inline void flush_pipeline(void)
{
	unsigned long pc;

	/*
	 * The FLUSH instruction takes address parameter.
	 * As such, it may trap if the address is not found in DTLB.
	 *
	 * The entire kernel text is mapped by a locked ITLB and
	 * DTLB entries. Therefore, when this function is called,
	 * the %pc register will always be in the range mapped by
	 * DTLB.
	 *
	 */

	asm volatile (
	    "rd %%pc, %[pc]\n"
	    "flush %[pc]\n"
	    : [pc] "=&r" (pc)
	);
}

/** Memory Barrier instruction. */
_NO_TRACE static inline void membar(void)
{
	asm volatile (
	    "membar #Sync\n"
	);
}

#endif

/** @}
 */
