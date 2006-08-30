/*
 * Copyright (C) 2006 Jakub Jermar
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

/** @addtogroup sparc64proc
 * @{
 */
/** @file
 */

#include <proc/scheduler.h>
#include <proc/thread.h>
#include <arch.h>
#include <arch/asm.h>
#include <arch/regdef.h>
#include <arch/stack.h>
#include <arch/mm/tlb.h>
#include <arch/mm/page.h>
#include <config.h>
#include <align.h>
#include <macros.h>

/** Perform sparc64 specific tasks needed before the new task is run. */
void before_task_runs_arch(void)
{
}

/** Perform sparc64 specific steps before scheduling a thread.
 *
 * Ensure that thread's kernel stack, as well as userspace window
 * buffer for userspace threads, are locked in DTLB.
 * For userspace threads, initialize reserved global registers
 * in the alternate and interrupt sets.
 */
void before_thread_runs_arch(void)
{
	uintptr_t base;
	
	base = ALIGN_DOWN(config.base, 1<<KERNEL_PAGE_WIDTH);

	if (!overlaps((uintptr_t) THREAD->kstack, PAGE_SIZE, base, (1<<KERNEL_PAGE_WIDTH))) {
		/*
		 * Kernel stack of this thread is not locked in DTLB.
		 * First, make sure it is not mapped already.
		 * If not, create a locked mapping for it.
		 */
		dtlb_demap(TLB_DEMAP_PAGE, TLB_DEMAP_NUCLEUS, (uintptr_t) THREAD->kstack);
		dtlb_insert_mapping((uintptr_t) THREAD->kstack, KA2PA(THREAD->kstack), PAGESIZE_8K, true, true);
	}
	
	if ((THREAD->flags & THREAD_FLAG_USPACE)) {
		/*
		 * If this thread executes also in userspace, we have to lock
		 * its userspace window buffer into DTLB.
		 */
		ASSERT(THREAD->arch.uspace_window_buffer);
		uintptr_t uw_buf = ALIGN_DOWN((uintptr_t) THREAD->arch.uspace_window_buffer, PAGE_SIZE);
		if (!overlaps(uw_buf, PAGE_SIZE, base, 1<<KERNEL_PAGE_WIDTH)) {
			/*
			 * The buffer is not covered by the 4M locked kernel DTLB entry.
			 */
			dtlb_demap(TLB_DEMAP_PAGE, TLB_DEMAP_NUCLEUS, uw_buf);
			dtlb_insert_mapping(uw_buf, KA2PA(uw_buf), PAGESIZE_8K, true, true);
		}
		
		/*
		 * Write kernel stack address to %g6 and a pointer to the last item
		 * in the userspace window buffer to %g7 in the alternate and interrupt sets.
		 */
		write_to_ig_g6((uintptr_t) THREAD->kstack + STACK_SIZE - STACK_BIAS);
		write_to_ag_g6((uintptr_t) THREAD->kstack + STACK_SIZE - STACK_BIAS);
		write_to_ag_g7((uintptr_t) THREAD->arch.uspace_window_buffer);
	}
}

/** Perform sparc64 specific steps before a thread stops running.
 *
 * Demap any locked DTLB entries isntalled by the thread (i.e. kernel stack
 * and userspace window buffer).
 */
void after_thread_ran_arch(void)
{
	uintptr_t base;

	base = ALIGN_DOWN(config.base, 1<<KERNEL_PAGE_WIDTH);

	if (!overlaps((uintptr_t) THREAD->kstack, PAGE_SIZE, base, (1<<KERNEL_PAGE_WIDTH))) {
		/*
		 * Kernel stack of this thread is locked in DTLB.
		 * Destroy the mapping.
		 */
		dtlb_demap(TLB_DEMAP_PAGE, TLB_DEMAP_NUCLEUS, (uintptr_t) THREAD->kstack);
	}
	
	if ((THREAD->flags & THREAD_FLAG_USPACE)) {
		/*
		 * If this thread executes also in userspace, we have to force all
		 * its still-active userspace windows into the userspace window buffer
		 * and demap the buffer from DTLB.
		 */
		ASSERT(THREAD->arch.uspace_window_buffer);
		
		flushw();	/* force all userspace windows into memory */
		
		uintptr_t uw_buf = ALIGN_DOWN((uintptr_t) THREAD->arch.uspace_window_buffer, PAGE_SIZE);
		if (!overlaps(uw_buf, PAGE_SIZE, base, 1<<KERNEL_PAGE_WIDTH)) {
			/*
			 * The buffer is not covered by the 4M locked kernel DTLB entry
			 * and therefore it was given a dedicated locked DTLB entry.
			 * Demap it.
			 */
			dtlb_demap(TLB_DEMAP_PAGE, TLB_DEMAP_NUCLEUS, uw_buf);
		}
	
		/* sample the state of the userspace window buffer */	
		THREAD->arch.uspace_window_buffer = (uint8_t *) read_from_ag_g7();
	}
}

/** @}
 */
