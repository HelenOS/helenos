/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup main
 * @{
 */

/**
 * @file
 * @brief 	Main initialization kernel function for all processors.
 *
 * During kernel boot, all processors, after architecture dependent
 * initialization, start executing code found in this file. After
 * bringing up all subsystems, control is passed to scheduler().
 *
 * The bootstrap processor starts executing main_bsp() while
 * the application processors start executing main_ap().
 *
 * @see scheduler()
 * @see main_bsp()
 * @see main_ap()
 */

#include <arch/asm.h>
#include <context.h>
#include <print.h>
#include <panic.h>
#include <debug.h>
#include <config.h>
#include <time/clock.h>
#include <time/timeout.h>
#include <proc/scheduler.h>
#include <proc/thread.h>
#include <proc/task.h>
#include <proc/tasklet.h>
#include <main/kinit.h>
#include <main/version.h>
#include <console/kconsole.h>
#include <console/console.h>
#include <cpu.h>
#include <align.h>
#include <interrupt.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <genarch/mm/page_pt.h>
#include <mm/tlb.h>
#include <mm/as.h>
#include <mm/slab.h>
#include <synch/waitq.h>
#include <synch/futex.h>
#include <arch/arch.h>
#include <arch.h>
#include <arch/faddr.h>
#include <ipc/ipc.h>
#include <macros.h>
#include <adt/btree.h>
#include <smp/smp.h>
#include <ddi/ddi.h>
#include <main/main.h>

/** Global configuration structure. */
config_t config;

/** Initial user-space tasks */
init_t init = {
	.cnt = 0
};

/** Boot allocations. */
ballocs_t ballocs = {
	.base = NULL,
	.size = 0
};

context_t ctx;

/*
 * These 'hardcoded' variables will be intialized by
 * the linker or the low level assembler code with
 * appropriate sizes and addresses.
 */

/** Virtual address of where the kernel is loaded. */
uintptr_t hardcoded_load_address = 0;
/** Size of the kernel code in bytes. */
size_t hardcoded_ktext_size = 0;
/** Size of the kernel data in bytes. */
size_t hardcoded_kdata_size = 0;
/** Lowest safe stack virtual address. */
uintptr_t stack_safe = 0;		

/*
 * These two functions prevent stack from underflowing during the
 * kernel boot phase when SP is set to the very top of the reserved
 * space. The stack could get corrupted by a fooled compiler-generated
 * pop sequence otherwise.
 */
static void main_bsp_separated_stack(void);
#ifdef CONFIG_SMP
static void main_ap_separated_stack(void);
#endif

#define CONFIG_STACK_SIZE	((1 << STACK_FRAMES) * STACK_SIZE)

/** Main kernel routine for bootstrap CPU.
 *
 * The code here still runs on the boot stack, which knows nothing about
 * preemption counts.  Because of that, this function cannot directly call
 * functions that disable or enable preemption (e.g. spinlock_lock()). The
 * primary task of this function is to calculate address of a new stack and
 * switch to it.
 *
 * Assuming interrupts_disable().
 *
 */
void main_bsp(void)
{
	config.cpu_count = 1;
	config.cpu_active = 1;
	
	config.base = hardcoded_load_address;
	config.kernel_size = ALIGN_UP(hardcoded_ktext_size +
	    hardcoded_kdata_size, PAGE_SIZE);
	config.stack_size = CONFIG_STACK_SIZE;
	
	/* Initialy the stack is placed just after the kernel */
	config.stack_base = config.base + config.kernel_size;
	
	/* Avoid placing stack on top of init */
	count_t i;
	for (i = 0; i < init.cnt; i++) {
		if (PA_overlaps(config.stack_base, config.stack_size,
		    init.tasks[i].addr, init.tasks[i].size))
			config.stack_base = ALIGN_UP(init.tasks[i].addr +
			    init.tasks[i].size, config.stack_size);
	}

	/* Avoid placing stack on top of boot allocations. */
	if (ballocs.size) {
		if (PA_overlaps(config.stack_base, config.stack_size,
		    ballocs.base, ballocs.size))
			config.stack_base = ALIGN_UP(ballocs.base +
			    ballocs.size, PAGE_SIZE);
	}
	
	if (config.stack_base < stack_safe)
		config.stack_base = ALIGN_UP(stack_safe, PAGE_SIZE);
	
	context_save(&ctx);
	context_set(&ctx, FADDR(main_bsp_separated_stack), config.stack_base,
	    THREAD_STACK_SIZE);
	context_restore(&ctx);
	/* not reached */
}


/** Main kernel routine for bootstrap CPU using new stack.
 *
 * Second part of main_bsp().
 *
 */
void main_bsp_separated_stack(void) 
{
	/* Keep this the first thing. */
	the_initialize(THE);
	
	version_print();
	
	LOG("\nconfig.base=%#" PRIp " config.kernel_size=%" PRIs
	    "\nconfig.stack_base=%#" PRIp " config.stack_size=%" PRIs,
	    config.base, config.kernel_size, config.stack_base,
	    config.stack_size);
	
#ifdef CONFIG_KCONSOLE
	/*
	 * kconsole data structures must be initialized very early
	 * because other subsystems will register their respective
	 * commands.
	 */
	LOG_EXEC(kconsole_init());
#endif
	
	/*
	 * Exception handler initialization, before architecture
	 * starts adding its own handlers
	 */
	LOG_EXEC(exc_init());
	
	/*
	 * Memory management subsystems initialization.
	 */
	LOG_EXEC(arch_pre_mm_init());
	LOG_EXEC(frame_init());
	
	/* Initialize at least 1 memory segment big enough for slab to work. */
	LOG_EXEC(slab_cache_init());
	LOG_EXEC(btree_init());
	LOG_EXEC(as_init());
	LOG_EXEC(page_init());
	LOG_EXEC(tlb_init());
	LOG_EXEC(ddi_init());
	LOG_EXEC(tasklet_init());
	LOG_EXEC(arch_post_mm_init());
	LOG_EXEC(arch_pre_smp_init());
	LOG_EXEC(smp_init());
	
	/* Slab must be initialized after we know the number of processors. */
	LOG_EXEC(slab_enable_cpucache());
	
	printf("Detected %" PRIc " CPU(s), %" PRIu64" MiB free memory\n",
	    config.cpu_count, SIZE2MB(zone_total_size()));
	
	LOG_EXEC(cpu_init());
	
	LOG_EXEC(calibrate_delay_loop());
	LOG_EXEC(clock_counter_init());
	LOG_EXEC(timeout_init());
	LOG_EXEC(scheduler_init());
	LOG_EXEC(task_init());
	LOG_EXEC(thread_init());
	LOG_EXEC(futex_init());
	
	if (init.cnt > 0) {
		count_t i;
		for (i = 0; i < init.cnt; i++)
			LOG("init[%" PRIc "].addr=%#" PRIp ", init[%" PRIc
			    "].size=%#" PRIs "\n", i, init.tasks[i].addr, i,
			    init.tasks[i].size);
	} else
		printf("No init binaries found\n");
	
	LOG_EXEC(ipc_init());
	LOG_EXEC(klog_init());
	LOG_EXEC(console_init());
	
#ifdef CONFIG_KCONSOLE
	LOG_EXEC(kconsole_notify_init());
#endif
	
	/*
	 * Create kernel task.
	 */
	task_t *kernel = task_create(AS_KERNEL, "kernel");
	if (!kernel)
		panic("Cannot create kernel task.");
	
	/*
	 * Create the first thread.
	 */
	thread_t *kinit_thread
		= thread_create(kinit, NULL, kernel, 0, "kinit", true);
	if (!kinit_thread)
		panic("Cannot create kinit thread.");
	LOG_EXEC(thread_ready(kinit_thread));
	
	/*
	 * This call to scheduler() will return to kinit,
	 * starting the thread of kernel threads.
	 */
	scheduler();
	/* not reached */
}


#ifdef CONFIG_SMP
/** Main kernel routine for application CPUs.
 *
 * Executed by application processors, temporary stack
 * is at ctx.sp which was set during BSP boot.
 * This function passes control directly to
 * main_ap_separated_stack().
 *
 * Assuming interrupts_disable()'d.
 *
 */
void main_ap(void)
{
	/*
	 * Incrementing the active CPU counter will guarantee that the
	 * *_init() functions can find out that they need to
	 * do initialization for AP only.
	 */
	config.cpu_active++;

	/*
	 * The THE structure is well defined because ctx.sp is used as stack.
	 */
	the_initialize(THE);
	
	arch_pre_mm_init();
	frame_init();
	page_init();
	tlb_init();
	arch_post_mm_init();
	
	cpu_init();
	calibrate_delay_loop();
	arch_post_cpu_init();

	the_copy(THE, (the_t *) CPU->stack);

	/*
	 * If we woke kmp up before we left the kernel stack, we could
	 * collide with another CPU coming up. To prevent this, we
	 * switch to this cpu's private stack prior to waking kmp up.
	 */
	context_save(&CPU->saved_context);
	context_set(&CPU->saved_context, FADDR(main_ap_separated_stack),
	    (uintptr_t) CPU->stack, CPU_STACK_SIZE);
	context_restore(&CPU->saved_context);
	/* not reached */
}


/** Main kernel routine for application CPUs using new stack.
 *
 * Second part of main_ap().
 *
 */
void main_ap_separated_stack(void)
{
	/*
	 * Configure timeouts for this cpu.
	 */
	timeout_init();

	waitq_wakeup(&ap_completion_wq, WAKEUP_FIRST);
	scheduler();
	/* not reached */
}
#endif /* CONFIG_SMP */

/** @}
 */
