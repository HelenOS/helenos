/*
 * Copyright (C) 2005 Jakub Jermar
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
#include <cpu.h>
#include <proc/task.h>
#include <proc/thread.h>
#include <arch.h>
#include <arch/context.h>	/* SP_DELTA */
#include <arch/debugger.h>
#include <arch/pm.h>
#include <arch/asm.h>

/** Perform ia32 specific tasks needed before the new task is run.
 *
 * Interrupts are disabled.
 */
void before_task_runs_arch(void)
{
	size_t iomap_size;
	ptr_16_32_t cpugdtr;
	descriptor_t *gdt_p;

	/*
	 * Switch the I/O Permission Bitmap, if necessary.
	 */

	/* First, copy the I/O Permission Bitmap. */
	spinlock_lock(&TASK->lock);
	iomap_size = TASK->arch.iomap_size;
	if (iomap_size) {
		ASSERT(TASK->arch.iomap);
		memcpy(CPU->arch.tss->iomap, TASK->arch.iomap, iomap_size);
		CPU->arch.tss->iomap[iomap_size] = 0xff;	/* terminating byte */
	}
	spinlock_unlock(&TASK->lock);	

	/* Second, adjust TSS segment limit. */
	gdtr_store(&cpugdtr);
	gdt_p = (descriptor_t *) cpugdtr.base;
	gdt_setlimit(&gdt_p[TSS_DES], TSS_BASIC_SIZE + iomap_size - 1);
	gdtr_load(&cpugdtr);
}

/** Perform ia32 specific tasks needed before the new thread is scheduled.
 *
 * THREAD is locked and interrupts are disabled.
 */
void before_thread_runs_arch(void)
{
	CPU->arch.tss->esp0 = (__address) &THREAD->kstack[THREAD_STACK_SIZE-SP_DELTA];
	CPU->arch.tss->ss0 = selector(KDATA_DES);

	/* Set up TLS in GS register */
	set_tls_desc(THREAD->arch.tls);

#ifdef CONFIG_DEBUG_AS_WATCHPOINT
	/* Set watchpoint on AS to ensure that nobody sets it to zero */
	if (CPU->id < BKPOINTS_MAX)
		breakpoint_add(&((the_t *) THREAD->kstack)->as, 
			       BKPOINT_WRITE | BKPOINT_CHECK_ZERO,
			       CPU->id);
#endif
}

void after_thread_ran_arch(void)
{
}
