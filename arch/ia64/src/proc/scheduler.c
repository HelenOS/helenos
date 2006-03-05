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

#include <proc/scheduler.h>
#include <proc/thread.h>
#include <arch.h>
#include <arch/register.h>
#include <arch/context.h>
#include <arch/mm/tlb.h>
#include <config.h>
#include <align.h>

/** Record kernel stack address in bank 0 r23 and make sure it is mapped in DTR. */
void before_thread_runs_arch(void)
{
	__address base;
	
	base = ALIGN_DOWN(config.base, 1<<KERNEL_PAGE_WIDTH);

	if ((__address) THREAD->kstack < base || (__address) THREAD->kstack > base + (1<<KERNEL_PAGE_WIDTH)) {
		/*
		 * Kernel stack of this thread is not mapped by DTR[TR_KERNEL].
		 * Use DTR[TR_KSTACK] to map it.
		 */
		 dtlb_kernel_mapping_insert((__address) THREAD->kstack, KA2PA(THREAD->kstack), true, DTR_KSTACK);
	}
	
	/*
	 * Record address of kernel stack to bank 0 r23
	 * where it will be found after switch from userspace.
	 */
	__asm__ volatile (
		"bsw.0\n"
		"mov r23 = %0\n"
		"bsw.1\n"
		 : : "r" (&THREAD->kstack[THREAD_STACK_SIZE - SP_DELTA]));
}

void after_thread_ran_arch(void)
{
}
