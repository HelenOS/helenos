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
#include <arch/mm/tlb.h>
#include <arch/mm/page.h>
#include <config.h>
#include <align.h>

/** Perform sparc64 specific tasks needed before the new task is run. */
void before_task_runs_arch(void)
{
}

/** Ensure that thread's kernel stack is locked in TLB. */
void before_thread_runs_arch(void)
{
	uintptr_t base;
	
	base = ALIGN_DOWN(config.base, 1<<KERNEL_PAGE_WIDTH);

	if ((uintptr_t) THREAD->kstack < base || (uintptr_t) THREAD->kstack > base + (1<<KERNEL_PAGE_WIDTH)) {
		/*
		 * Kernel stack of this thread is not locked in DTLB.
		 * First, make sure it is not mapped already.
		 * If not, create a locked mapping for it.
		 */
		 dtlb_demap(TLB_DEMAP_PAGE, TLB_DEMAP_NUCLEUS, (uintptr_t) THREAD->kstack);
		 dtlb_insert_mapping((uintptr_t) THREAD->kstack, KA2PA(THREAD->kstack), PAGESIZE_8K, true, true);
	}	
}

/** Unlock thread's stack from TLB, if necessary. */
void after_thread_ran_arch(void)
{
	uintptr_t base;

	base = ALIGN_DOWN(config.base, 1<<KERNEL_PAGE_WIDTH);

	if ((uintptr_t) THREAD->kstack < base || (uintptr_t) THREAD->kstack > base + (1<<KERNEL_PAGE_WIDTH)) {
		/*
		 * Kernel stack of this thread is locked in DTLB.
		 * Destroy the mapping.
		 */
		dtlb_demap(TLB_DEMAP_PAGE, TLB_DEMAP_NUCLEUS, (uintptr_t) THREAD->kstack);
	}
}

/** @}
 */
