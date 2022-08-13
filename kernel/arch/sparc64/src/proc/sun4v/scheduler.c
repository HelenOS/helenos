/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_proc
 * @{
 */
/** @file
 */

#include <proc/scheduler.h>
#include <proc/thread.h>
#include <arch.h>
#include <arch/asm.h>
#include <arch/arch.h>
#include <arch/stack.h>
#include <arch/sun4v/cpu.h>
#include <arch/sun4v/hypercall.h>

/** Perform sparc64 specific tasks needed before the new task is run. */
void before_task_runs_arch(void)
{
}

/** Perform sparc64 specific steps before scheduling a thread.
 *
 * For userspace threads, initialize pointer to the kernel stack and for the
 * userspace window buffer.
 */
void before_thread_runs_arch(void)
{
	if (THREAD->uspace) {
		uint64_t sp;

		sp = (uintptr_t) THREAD->kstack + STACK_SIZE - STACK_BIAS;
		asi_u64_write(ASI_SCRATCHPAD, SCRATCHPAD_KSTACK, sp);
		asi_u64_write(ASI_SCRATCHPAD, SCRATCHPAD_WBUF,
		    (uintptr_t) THREAD->arch.uspace_window_buffer);
	}
}

/** Perform sparc64 specific steps before a thread stops running. */
void after_thread_ran_arch(void)
{
	if (THREAD->uspace) {
		/* sample the state of the userspace window buffer */
		THREAD->arch.uspace_window_buffer =
		    (uint8_t *) asi_u64_read(ASI_SCRATCHPAD, SCRATCHPAD_WBUF);

	}
}

/** @}
 */
