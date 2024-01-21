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
		asm volatile ("flushw");

		/* sample the state of the userspace window buffer */
		THREAD->arch.uspace_window_buffer =
		    (uint8_t *) read_from_ag_g7();
	}
}

/** @}
 */
