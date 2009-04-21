/*
 * Copyright (c) 2007 Michal Kebrt
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

/** @addtogroup arm32
 * @{
 */
/** @file
 *  @brief ARM32 architecture specific functions.
 */

#include <arch.h>
#include <config.h>
#include <genarch/fb/fb.h>
#include <genarch/fb/visuals.h>
#include <genarch/drivers/dsrln/dsrlnin.h>
#include <genarch/drivers/dsrln/dsrlnout.h>
#include <genarch/srln/srln.h>
#include <sysinfo/sysinfo.h>
#include <console/console.h>
#include <ddi/irq.h>
#include <arch/drivers/gxemul.h>
#include <print.h>
#include <config.h>
#include <interrupt.h>
#include <arch/regutils.h>
#include <userspace.h>
#include <macros.h>
#include <string.h>

/** Performs arm32-specific initialization before main_bsp() is called. */
void arch_pre_main(void *entry __attribute__((unused)), bootinfo_t *bootinfo)
{
	unsigned int i;
	
	init.cnt = bootinfo->cnt;
	
	for (i = 0; i < min3(bootinfo->cnt, TASKMAP_MAX_RECORDS, CONFIG_INIT_TASKS); ++i) {
		init.tasks[i].addr = bootinfo->tasks[i].addr;
		init.tasks[i].size = bootinfo->tasks[i].size;
		str_cpy(init.tasks[i].name, CONFIG_TASK_NAME_BUFLEN,
		    bootinfo->tasks[i].name);
	}
}

/** Performs arm32 specific initialization before mm is initialized. */
void arch_pre_mm_init(void)
{
	/* It is not assumed by default */
	interrupts_disable();
}

/** Performs arm32 specific initialization afterr mm is initialized. */
void arch_post_mm_init(void)
{
	gxemul_init();
	
	/* Initialize exception dispatch table */
	exception_init();
	interrupt_init();
	
#ifdef CONFIG_FB
	fb_properties_t prop = {
		.addr = GXEMUL_FB_ADDRESS,
		.offset = 0,
		.x = 640,
		.y = 480,
		.scan = 1920,
		.visual = VISUAL_BGR_8_8_8,
	};
	fb_init(&prop);
#else
#ifdef CONFIG_ARM_PRN
	dsrlnout_init((ioport8_t *) gxemul_kbd);
#endif /* CONFIG_ARM_PRN */
#endif /* CONFIG_FB */
}

/** Performs arm32 specific tasks needed after cpu is initialized.
 *
 * Currently the function is empty.
 */
void arch_post_cpu_init(void)
{
}


/** Performs arm32 specific tasks needed before the multiprocessing is
 * initialized.
 *
 * Currently the function is empty because SMP is not supported.
 */
void arch_pre_smp_init(void)
{
}


/** Performs arm32 specific tasks needed after the multiprocessing is
 * initialized.
 *
 * Currently the function is empty because SMP is not supported.
 */
void arch_post_smp_init(void)
{
#ifdef CONFIG_ARM_KBD
	/*
	 * Initialize the GXemul keyboard port. Then initialize the serial line
	 * module and connect it to the GXemul keyboard.
	 */
	dsrlnin_instance_t *dsrlnin_instance
	    = dsrlnin_init((dsrlnin_t *) gxemul_kbd, GXEMUL_KBD_IRQ);
	if (dsrlnin_instance) {
		srln_instance_t *srln_instance = srln_init();
		if (srln_instance) {
			indev_t *sink = stdin_wire();
			indev_t *srln = srln_wire(srln_instance, sink);
			dsrlnin_wire(dsrlnin_instance, srln);
		}
	}
	
	/*
	 * This is the necessary evil until the userspace driver is entirely
	 * self-sufficient.
	 */
	sysinfo_set_item_val("kbd", NULL, true);
	sysinfo_set_item_val("kbd.inr", NULL, GXEMUL_KBD_IRQ);
	sysinfo_set_item_val("kbd.address.virtual", NULL, (unative_t) gxemul_kbd);
#endif
}


/** Performs arm32 specific tasks needed before the new task is run. */
void before_task_runs_arch(void)
{
	tlb_invalidate_all();
}


/** Performs arm32 specific tasks needed before the new thread is scheduled.
 *
 * It sets supervisor_sp.
 */
void before_thread_runs_arch(void)
{
	uint8_t *stck;
	
	stck = &THREAD->kstack[THREAD_STACK_SIZE - SP_DELTA];
	supervisor_sp = (uintptr_t) stck;
}

/** Performs arm32 specific tasks before a thread stops running.
 *
 * Currently the function is empty.
 */
void after_thread_ran_arch(void)
{
}

/** Halts CPU. */
void cpu_halt(void)
{
	*((char *) (gxemul_kbd + GXEMUL_HALT_OFFSET))
		= 0;
}

/** Reboot. */
void arch_reboot()
{
	/* not implemented */
	while (1);
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

/** Acquire console back for kernel. */
void arch_grab_console(void)
{
#ifdef CONFIG_FB
	fb_redraw();
#endif
}

/** Return console to userspace. */
void arch_release_console(void)
{
}

/** @}
 */
