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
#include <arch/stack.h>

/** Perform sparc64 specific tasks needed before the new task is run. */
void before_task_runs_arch(void)
{
}

/** Perform sparc64 specific steps before scheduling a thread.
 *
 * For userspace threads, initialize reserved global registers in the alternate
 * and interrupt sets.
 */
void before_thread_runs_arch(void)
{
	if (THREAD->uspace) {
		uint64_t sp;

		/*
		 * Write kernel stack address to %g6 of the alternate and
		 * interrupt global sets.
		 *
		 * Write pointer to the last item in the userspace window buffer
		 * to %g7 in the alternate set. Write to the interrupt %g7 is
		 * not necessary because:
		 * - spill traps operate only in the alternate global set,
		 * - preemptible trap handler switches to alternate globals
		 *   before it explicitly uses %g7.
		 */
		sp = (uintptr_t) THREAD->kstack + STACK_SIZE - STACK_BIAS;
		write_to_ig_g6(sp);
		write_to_ag_g6(sp);
		write_to_ag_g7((uintptr_t) THREAD->arch.uspace_window_buffer);
	}
}

/** Perform sparc64 specific steps before a thread stops running. */
void after_thread_ran_arch(void)
{
	if (THREAD->uspace) {
		/* sample the state of the userspace window buffer */
		THREAD->arch.uspace_window_buffer =
		    (uint8_t *) read_from_ag_g7();
	}
}

/** @}
 */
