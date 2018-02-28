/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup ia64proc
 * @{
 */
/** @file
 */

#include <proc/scheduler.h>
#include <proc/thread.h>
#include <arch.h>
#include <arch/register.h>
#include <arch/context.h>
#include <arch/stack.h>
#include <arch/mm/tlb.h>
#include <config.h>
#include <align.h>

/** Perform ia64 specific tasks needed before the new task is run. */
void before_task_runs_arch(void)
{
}

/** Prepare kernel stack pointers in bank 0 r22 and r23 and make sure the stack
 * is mapped in DTR.
 */
void before_thread_runs_arch(void)
{
	uintptr_t base;

	base = ALIGN_DOWN(config.base, 1 << KERNEL_PAGE_WIDTH);

	if ((uintptr_t) THREAD->kstack < base ||
	    (uintptr_t) THREAD->kstack > base + (1 << (KERNEL_PAGE_WIDTH))) {
		/*
		 * Kernel stack of this thread is not mapped by DTR[TR_KERNEL].
		 * Use DTR[TR_KSTACK1] and DTR[TR_KSTACK2] to map it.
		 */

		/* purge DTR[TR_STACK1] and DTR[TR_STACK2] */
		dtr_purge((uintptr_t) THREAD->kstack, PAGE_WIDTH+1);

		/* insert DTR[TR_STACK1] and DTR[TR_STACK2] */
		dtlb_kernel_mapping_insert((uintptr_t) THREAD->kstack,
		    KA2PA(THREAD->kstack), true, DTR_KSTACK1);
		dtlb_kernel_mapping_insert((uintptr_t) THREAD->kstack +
		    PAGE_SIZE, KA2PA(THREAD->kstack) + FRAME_SIZE, true,
		    DTR_KSTACK2);
	}

	/*
	 * Record address of kernel backing store to bank 0 r22.
	 * Record address of kernel stack to bank 0 r23.
	 * These values will be found there after switch from userspace.
	 *
	 * Mind the 1:1 split of the entire STACK_SIZE long region between the
	 * memory stack and the RSE stack.
	 */
	asm volatile (
		"bsw.0\n"
		"mov r22 = %0\n"
		"mov r23 = %1\n"
		"bsw.1\n"
		:
		: "r" (&THREAD->kstack[STACK_SIZE / 2]),
		  "r" (&THREAD->kstack[STACK_SIZE / 2])
		);
}

void after_thread_ran_arch(void)
{
}

/** @}
 */
