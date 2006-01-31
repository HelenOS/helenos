/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

#include <arch/asm.h>
#include <context.h>
#include <print.h>
#include <panic.h>
#include <debug.h>
#include <config.h>
#include <time/clock.h>
#include <proc/scheduler.h>
#include <proc/thread.h>
#include <proc/task.h>
#include <main/kinit.h>
#include <main/version.h>
#include <console/kconsole.h>
#include <cpu.h>
#include <align.h>
#include <interrupt.h>
#include <arch/mm/memory_init.h>
#include <mm/heap.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <genarch/mm/page_pt.h>
#include <mm/tlb.h>
#include <mm/as.h>
#include <synch/waitq.h>
#include <arch/arch.h>
#include <arch.h>
#include <arch/faddr.h>
#include <typedefs.h>

#ifdef CONFIG_SMP
#include <arch/smp/apic.h>
#include <arch/smp/mps.h>
#endif /* CONFIG_SMP */
#include <smp/smp.h>

config_t config;	/**< Global configuration structure. */

context_t ctx;

/**
 * These 'hardcoded' variables will be intialized by
 * the linker or the low level assembler code with
 * appropriate sizes and addresses.
 */
__address hardcoded_load_address = 0;
size_t hardcoded_ktext_size = 0;
size_t hardcoded_kdata_size = 0;

__address init_addr = 0;
size_t init_size = 0;

/** Kernel address space. */
as_t *AS_KERNEL = NULL;

void main_bsp(void);
void main_ap(void);

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

/** Bootstrap CPU main kernel routine
 *
 * Initializes the kernel by bootstrap CPU.
 * This function passes control directly to
 * main_bsp_separated_stack().
 *
 * Assuming interrupts_disable().
 *
 */
void main_bsp(void)
{
	config.cpu_count = 1;
	config.cpu_active = 1;
	
	config.base = hardcoded_load_address;
	config.memory_size = get_memory_size();
	config.init_addr = init_addr;
	config.init_size = init_size;
	
	if (init_size > 0)
		config.heap_addr = init_addr + init_size;
	else
		config.heap_addr = hardcoded_load_address + hardcoded_ktext_size + hardcoded_kdata_size;
	
	config.heap_size = CONFIG_HEAP_SIZE + (config.memory_size / FRAME_SIZE) * sizeof(frame_t);
	
	config.kernel_size = ALIGN_UP(config.heap_addr - hardcoded_load_address + config.heap_size, PAGE_SIZE);
	config.heap_delta = config.kernel_size - (config.heap_addr - hardcoded_load_address + config.heap_size);
	config.kernel_size = config.kernel_size + CONFIG_STACK_SIZE;
	
	context_save(&ctx);
	context_set(&ctx, FADDR(main_bsp_separated_stack), config.base + config.kernel_size - CONFIG_STACK_SIZE, CONFIG_STACK_SIZE);
	context_restore(&ctx);
	/* not reached */
}


/** Bootstrap CPU main kernel routine stack wrapper
 *
 * Second part of main_bsp().
 *
 */
void main_bsp_separated_stack(void) 
{
	as_t *as;
	task_t *k;
	thread_t *t;
	
	the_initialize(THE);
	
	/*
	 * kconsole data structures must be initialized very early
	 * because other subsystems will register their respective
	 * commands.
	 */
	kconsole_init();

	/* Exception handler initialization, before architecture
	 * starts adding its own handlers
	 */
	exc_init();

	/*
	 * Memory management subsystems initialization.
	 */	
	arch_pre_mm_init();
	early_heap_init(config.heap_addr, config.heap_size + config.heap_delta);
	frame_init();
	page_init();
	tlb_init();
	arch_post_mm_init();

	version_print();

	printf("%P: hardcoded_ktext_size=%dK, hardcoded_kdata_size=%dK\n",
		config.base, hardcoded_ktext_size/1024, hardcoded_kdata_size/1024);

	arch_pre_smp_init();
	smp_init();
	printf("config.memory_size=%dM\n", config.memory_size/(1024*1024));
	printf("config.cpu_count=%d\n", config.cpu_count);

	cpu_init();

	calibrate_delay_loop();

	timeout_init();
	scheduler_init();
	task_init();
	thread_init();
	
	if (config.init_size > 0)
		printf("config.init_addr=%X, config.init_size=%d\n", config.init_addr, config.init_size);

	/*
	 * Create kernel address space.
	 */
	as = as_create(GET_PTL0_ADDRESS(), FLAG_AS_KERNEL);
	if (!as)
		panic("can't create kernel address space\n");

	/*
	 * Create kernel task.
	 */
	k = task_create(as);
	if (!k)
		panic("can't create kernel task\n");
		
	/*
	 * Create the first thread.
	 */
	t = thread_create(kinit, NULL, k, 0);
	if (!t)
		panic("can't create kinit thread\n");
	thread_ready(t);

	/*
	 * This call to scheduler() will return to kinit,
	 * starting the thread of kernel threads.
	 */
	scheduler();
	/* not reached */
}


#ifdef CONFIG_SMP
/** Application CPUs main kernel routine
 *
 * Executed by application processors, temporary stack
 * is at ctx.sp which was set during BP boot.
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
	 * pm_init() will not attempt to build GDT and IDT tables again.
	 * Neither frame_init() will do the complete thing. Neither cpu_init()
	 * will do.
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

	l_apic_init();
	l_apic_debug();

	the_copy(THE, (the_t *) CPU->stack);

	/*
	 * If we woke kmp up before we left the kernel stack, we could
	 * collide with another CPU coming up. To prevent this, we
	 * switch to this cpu's private stack prior to waking kmp up.
	 */
	context_set(&CPU->saved_context, FADDR(main_ap_separated_stack), (__address) CPU->stack, CPU_STACK_SIZE);
	context_restore(&CPU->saved_context);
	/* not reached */
}


/** Application CPUs main kernel routine stack wrapper
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
