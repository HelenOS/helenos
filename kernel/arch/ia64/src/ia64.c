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

/** @addtogroup ia64
 * @{
 */
/** @file
 */

#include <arch.h>
#include <arch/ski/ski.h>
#include <arch/drivers/it.h>
#include <arch/interrupt.h>
#include <arch/barrier.h>
#include <arch/asm.h>
#include <arch/register.h>
#include <arch/types.h>
#include <arch/context.h>
#include <arch/stack.h>
#include <arch/mm/page.h>
#include <mm/as.h>
#include <config.h>
#include <userspace.h>
#include <console/console.h>
#include <proc/uarg.h>
#include <syscall/syscall.h>
#include <ddi/irq.h>

void arch_pre_main(void)
{
	/* Setup usermode init tasks. */
	init.cnt = 8;
	init.tasks[0].addr = INIT0_ADDRESS;
	init.tasks[0].size = INIT0_SIZE;
	init.tasks[1].addr = INIT0_ADDRESS + 0x400000;
	init.tasks[1].size = INIT0_SIZE;
	init.tasks[2].addr = INIT0_ADDRESS + 0x800000;
	init.tasks[2].size = INIT0_SIZE;
	init.tasks[3].addr = INIT0_ADDRESS + 0xc00000;
	init.tasks[3].size = INIT0_SIZE;
	init.tasks[4].addr = INIT0_ADDRESS + 0x1000000;
	init.tasks[4].size = INIT0_SIZE;
	init.tasks[5].addr = INIT0_ADDRESS + 0x1400000;
	init.tasks[5].size = INIT0_SIZE;
	init.tasks[6].addr = INIT0_ADDRESS + 0x1800000;
	init.tasks[6].size = INIT0_SIZE;
	init.tasks[7].addr = INIT0_ADDRESS + 0x1c00000;
	init.tasks[7].size = INIT0_SIZE;
}

void arch_pre_mm_init(void)
{
	/* Set Interruption Vector Address (i.e. location of interruption vector table). */
	iva_write((uintptr_t) &ivt);
	srlz_d();
	
}

void arch_post_mm_init(void)
{
	irq_init(INR_COUNT, INR_COUNT);
	ski_init_console();
	it_init();	
	ski_set_console_sysinfo();
}

void arch_post_cpu_init(void)
{
}

void arch_pre_smp_init(void)
{
}

void arch_post_smp_init(void)
{
}

/** Enter userspace and never return. */
void userspace(uspace_arg_t *kernel_uarg)
{
	psr_t psr;
	rsc_t rsc;

	psr.value = psr_read();
	psr.cpl = PL_USER;
	psr.i = true;				/* start with interrupts enabled */
	psr.ic = true;
	psr.ri = 0;				/* start with instruction #0 */
	psr.bn = 1;				/* start in bank 0 */

	__asm__ volatile ("mov %0 = ar.rsc\n" : "=r" (rsc.value));
	rsc.loadrs = 0;
	rsc.be = false;
	rsc.pl = PL_USER;
	rsc.mode = 3;				/* eager mode */

	switch_to_userspace((uintptr_t) kernel_uarg->uspace_entry,
			    ((uintptr_t) kernel_uarg->uspace_stack)+PAGE_SIZE-ALIGN_UP(STACK_ITEM_SIZE, STACK_ALIGNMENT),
			    ((uintptr_t) kernel_uarg->uspace_stack)+PAGE_SIZE,
			    (uintptr_t) kernel_uarg->uspace_uarg,
			    psr.value, rsc.value);

	while (1) {
		;
	}
}

/** Set thread-local-storage pointer.
 *
 * We use r13 (a.k.a. tp) for this purpose.
 */
unative_t sys_tls_set(unative_t addr)
{
        return 0;
}

/** Acquire console back for kernel
 *
 */
void arch_grab_console(void)
{
	ski_kbd_grab();
}
/** Return console to userspace
 *
 */
void arch_release_console(void)
{
	ski_kbd_release();
}

/** @}
 */
