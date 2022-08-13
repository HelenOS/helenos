/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64_proc
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
		dtr_purge((uintptr_t) THREAD->kstack, PAGE_WIDTH + 1);

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
