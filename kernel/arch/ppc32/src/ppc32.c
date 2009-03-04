/*
 * Copyright (c) 2005 Martin Decky
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

#include <config.h>
#include <arch.h>
#include <arch/boot/boot.h>
#include <arch/drivers/cuda.h>
#include <arch/interrupt.h>
#include <genarch/fb/fb.h>
#include <genarch/fb/visuals.h>
#include <userspace.h>
#include <proc/uarg.h>
#include <console/console.h>
#include <ddi/device.h>
#include <ddi/irq.h>
#include <arch/drivers/pic.h>
#include <macros.h>
#include <string.h>

#define IRQ_COUNT  64

bootinfo_t bootinfo;

/** Performs ppc32-specific initialization before main_bsp() is called. */
void arch_pre_main(void)
{
	init.cnt = bootinfo.taskmap.count;
	
	uint32_t i;
	
	for (i = 0; i < min3(bootinfo.taskmap.count, TASKMAP_MAX_RECORDS, CONFIG_INIT_TASKS); i++) {
		init.tasks[i].addr = PA2KA(bootinfo.taskmap.tasks[i].addr);
		init.tasks[i].size = bootinfo.taskmap.tasks[i].size;
		strncpy(init.tasks[i].name, bootinfo.taskmap.tasks[i].name,
			CONFIG_TASK_NAME_BUFLEN);
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
		if (bootinfo.screen.addr) {
			unsigned int visual;
			
			switch (bootinfo.screen.bpp) {
			case 8:
				visual = VISUAL_INDIRECT_8;
				break;
			case 16:
				visual = VISUAL_RGB_5_5_5;
				break;
			case 24:
				visual = VISUAL_RGB_8_8_8;
				break;
			case 32:
				visual = VISUAL_RGB_0_8_8_8;
				break;
			default:
				panic("Unsupported bits per pixel.");
			}
			fb_properties_t prop = {
				.addr = bootinfo.screen.addr,
				.offset = 0,
				.x = bootinfo.screen.width,
				.y = bootinfo.screen.height,
				.scan = bootinfo.screen.scanline,
				.visual = visual,
			};
			fb_init(&prop);
		}
		
		/* Initialize IRQ routing */
		irq_init(IRQ_COUNT, IRQ_COUNT);
		
		if (bootinfo.macio.addr) {
			/* Initialize PIC */
			pic_init(bootinfo.macio.addr, PAGE_SIZE);
			
			/* Initialize I/O controller */
			cuda_init(device_assign_devno(),
			    bootinfo.macio.addr + 0x16000, 2 * PAGE_SIZE);
		}
		
		/* Merge all zones to 1 big zone */
		zone_merge_all();
	}
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

void calibrate_delay_loop(void)
{
}

void userspace(uspace_arg_t *kernel_uarg)
{
	userspace_asm((uintptr_t) kernel_uarg->uspace_uarg,
	    (uintptr_t) kernel_uarg->uspace_stack +
	    THREAD_STACK_SIZE - SP_DELTA,
	    (uintptr_t) kernel_uarg->uspace_entry);
	
	/* Unreachable */
	while (true);
}

/** Acquire console back for kernel
 *
 */
void arch_grab_console(void)
{
	fb_redraw();
}

/** Return console to userspace
 *
 */
void arch_release_console(void)
{
}

/** Construct function pointer
 *
 * @param fptr   function pointer structure
 * @param addr   function address
 * @param caller calling function address
 *
 * @return address of the function pointer
 *
 */
void *arch_construct_function(fncptr_t *fptr, void *addr, void *caller)
{
	return addr;
}

/** @}
 */
