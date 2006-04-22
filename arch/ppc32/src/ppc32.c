/*
 * Copyright (C) 2005 Martin Decky
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

#include <arch.h>
#include <arch/boot/boot.h>
#include <arch/console.h>
#include <arch/drivers/cuda.h>
#include <arch/mm/memory_init.h>
#include <arch/interrupt.h>
#include <userspace.h>
#include <proc/uarg.h>

bootinfo_t bootinfo;

void arch_pre_main(void)
{
	/* Setup usermode */
	init.cnt = 1;
	init.tasks[0].addr = PA2KA(bootinfo.init.addr);
	init.tasks[0].size = bootinfo.init.size;
}

void arch_pre_mm_init(void)
{
	/* Initialize dispatch table */
	interrupt_init();
	
	/* Start decrementer */
	start_decrementer();

	ppc32_console_init();
	cuda_init();
}

void arch_post_mm_init(void)
{
}

void arch_pre_smp_init(void)
{
	memory_print_map();
}

void arch_post_smp_init(void)
{
}

void calibrate_delay_loop(void)
{
}

void userspace(uspace_arg_t *kernel_uarg)
{
	userspace_asm((__address) kernel_uarg->uspace_uarg, (__address) kernel_uarg->uspace_stack, (__address) kernel_uarg->uspace_entry);
	
	/* Unreachable */
	for (;;)
		;
}
