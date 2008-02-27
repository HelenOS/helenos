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
#include <arch/boot.h>
#include <config.h>
#include <arch/console.h>
#include <ddi/device.h>
#include <genarch/fb/fb.h>
#include <genarch/fb/visuals.h>
#include <ddi/irq.h>
#include <arch/debug/print.h>
#include <print.h>
#include <config.h>
#include <interrupt.h>
#include <arch/regutils.h>
#include <arch/machine.h>
#include <userspace.h>

/** Information about loaded tasks. */
bootinfo_t bootinfo;

/** Performs arm32 specific initialization before main_bsp() is called. */
void arch_pre_main(void)
{
	unsigned int i;

	init.cnt = bootinfo.cnt;

	for (i = 0; i < bootinfo.cnt; ++i) {
		init.tasks[i].addr = bootinfo.tasks[i].addr;
		init.tasks[i].size = bootinfo.tasks[i].size;
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
	machine_hw_map_init();

	/* Initialize exception dispatch table */
	exception_init();

	interrupt_init();
	
	console_init(device_assign_devno());

#ifdef CONFIG_FB
	fb_init(machine_get_fb_address(), 640, 480, 1920, VISUAL_RGB_8_8_8);
#endif
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
	machine_cpu_halt();
}

/** Reboot. */
void arch_reboot()
{
	/* not implemented */
	for (;;)
		;
}

/** @}
 */
