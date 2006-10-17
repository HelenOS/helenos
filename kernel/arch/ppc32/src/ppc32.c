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

/** @addtogroup ppc32
 * @{
 */
/** @file
 */

#include <arch.h>
#include <arch/boot/boot.h>
#include <arch/drivers/cuda.h>
#include <arch/mm/memory_init.h>
#include <arch/interrupt.h>
#include <genarch/fb/fb.h>
#include <userspace.h>
#include <proc/uarg.h>
#include <console/console.h>
#include <ddi/device.h>
#include <ddi/irq.h>
#include <arch/drivers/pic.h>

#define IRQ_COUNT	64

bootinfo_t bootinfo;

void arch_pre_main(void)
{
	/* Setup usermode */
	init.cnt = bootinfo.taskmap.count;
	
	uint32_t i;
	
	for (i = 0; i < bootinfo.taskmap.count; i++) {
		init.tasks[i].addr = PA2KA(bootinfo.taskmap.tasks[i].addr);
		init.tasks[i].size = bootinfo.taskmap.tasks[i].size;
	}
}

void arch_pre_mm_init(void)
{
	/* Initialize dispatch table */
	interrupt_init();

	/* Start decrementer */
	start_decrementer();
}

void arch_post_mm_init(void)
{
	if (config.cpu_active == 1) {
		/* Initialize framebuffer */
		fb_init(bootinfo.screen.addr, bootinfo.screen.width, bootinfo.screen.height, bootinfo.screen.bpp, bootinfo.screen.scanline, false);
		
		/* Initialize IRQ routing */
		irq_init(IRQ_COUNT, IRQ_COUNT);
	
		/* Initialize PIC */
		pic_init(bootinfo.keyboard.addr, PAGE_SIZE);
		
		/* Initialize I/O controller */
		cuda_init(device_assign_devno(), bootinfo.keyboard.addr + 0x16000, 2 * PAGE_SIZE);
		
		/* Merge all zones to 1 big zone */
		zone_merge_all();
	}
}

void arch_post_cpu_init(void)
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
	userspace_asm((uintptr_t) kernel_uarg->uspace_uarg, (uintptr_t) kernel_uarg->uspace_stack + THREAD_STACK_SIZE - SP_DELTA, (uintptr_t) kernel_uarg->uspace_entry);
	
	/* Unreachable */
	for (;;)
		;
}

/** Acquire console back for kernel
 *
 */
void arch_grab_console(void)
{
	cuda_grab();
}

/** Return console to userspace
 *
 */
void arch_release_console(void)
{
	cuda_release();
}

/** @}
 */
