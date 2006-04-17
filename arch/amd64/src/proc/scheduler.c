/*
 * Copyright (C) 2005 Ondrej Palkovsky
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
#include <arch/asm.h>
#include <arch/debugger.h>
#include <print.h>
#include <arch/pm.h>
#include <adt/bitmap.h>

/** Perform amd64 specific tasks needed before the new task is run.
 *
 * Interrupts are disabled.
 */
void before_task_runs_arch(void)
{
	count_t bits;
	ptr_16_64_t cpugdtr;
	descriptor_t *gdt_p;

	/*
	 * Switch the I/O Permission Bitmap, if necessary.
	 */
	 
	/* First, copy the I/O Permission Bitmap. */
	spinlock_lock(&TASK->lock);
	if ((bits = TASK->arch.iomap.bits)) {
		bitmap_t iomap;
	
		ASSERT(TASK->arch.iomap.map);
		bitmap_initialize(&iomap, CPU->arch.tss->iomap, TSS_IOMAP_SIZE * 8);
		bitmap_copy(&iomap, &TASK->arch.iomap, TASK->arch.iomap.bits);
		/*
		 * It is safe to set the trailing four bits because of the extra
		 * convenience byte in TSS_IOMAP_SIZE.
		 */
		bitmap_set_range(&iomap, TASK->arch.iomap.bits, 4);
	}
	spinlock_unlock(&TASK->lock);

	/* Second, adjust TSS segment limit. */
	gdtr_store(&cpugdtr);
	gdt_p = (descriptor_t *) cpugdtr.base;
	gdt_tss_setlimit(&gdt_p[TSS_DES], TSS_BASIC_SIZE + BITS2BYTES(bits) - 1);
	gdtr_load(&cpugdtr);
}

/** Perform amd64 specific tasks needed before the new thread is scheduled. */
void before_thread_runs_arch(void)
{
	CPU->arch.tss->rsp0 = (__address) &THREAD->kstack[THREAD_STACK_SIZE-SP_DELTA];

	/* Syscall support - write address of thread stack pointer to 
	 * hidden part of gs */
	swapgs();
	write_msr(AMD_MSR_GS, (__u64)&THREAD->kstack);
	swapgs();

	/* TLS support - set FS to thread local storage */
	write_msr(AMD_MSR_FS, THREAD->arch.tls);

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
