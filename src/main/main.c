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
#include <config.h>
#include <time/clock.h>
#include <proc/scheduler.h>
#include <proc/thread.h>
#include <proc/task.h>
#include <mm/vm.h>
#include <main/kinit.h>
#include <cpu.h>
#include <mm/heap.h>

#ifdef __SMP__
#include <arch/smp/apic.h>
#include <arch/smp/mps.h>
#endif /* __SMP__ */

#include <smp/smp.h>

#include <arch/mm/memory_init.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <mm/tlb.h>
#include <synch/waitq.h>

#include <arch/arch.h>
#include <arch.h>
#include <arch/faddr.h>

#include <typedefs.h>

char *project = "SPARTAN kernel";
char *copyright = "Copyright (C) 2001-2005 Jakub Jermar\nCopyright (C) 2005 HelenOS project";

config_t config;
context_t ctx;

/*
 * These 'hardcoded' variables will be intialised by
 * the linker or the low level assembler code with
 * appropriate sizes and addresses.
 */
__address hardcoded_load_address = 0;
size_t hardcoded_ktext_size = 0;
size_t hardcoded_kdata_size = 0;

/*
 * Size of memory in bytes taken by kernel and heap.
 */
static size_t kernel_size;

/*
 * Extra space on heap to make the stack start on page boundary.
 */
static size_t heap_delta;

void main_bsp(void);
void main_ap(void);

/*
 * These two functions prevent stack from underflowing during the
 * kernel boot phase when SP is set to the very top of the reserved
 * space. The stack could get corrupted by a fooled compiler-generated
 * pop sequence otherwise.
 */
static void main_bsp_separated_stack(void);
static void main_ap_separated_stack(void);

/** Bootstrap CPU main kernel routine
 *
 * Initializes the kernel by bootstrap CPU.
 *
 * Assuming cpu_priority_high().
 *
 */
void main_bsp(void)
{
	config.cpu_count = 1;
	config.cpu_active = 1;
	
	kernel_size = hardcoded_ktext_size + hardcoded_kdata_size + CONFIG_HEAP_SIZE;	 
	heap_delta = PAGE_SIZE - ((hardcoded_load_address + kernel_size) % PAGE_SIZE);
	heap_delta = (heap_delta == PAGE_SIZE) ? 0 : heap_delta;
	kernel_size += heap_delta;
	
	config.base = hardcoded_load_address;
	config.memory_size = get_memory_size();
	config.kernel_size = kernel_size + CONFIG_STACK_SIZE;
	
	context_save(&ctx);
	early_mapping(config.base + hardcoded_ktext_size + hardcoded_kdata_size + heap_delta, CONFIG_STACK_SIZE + CONFIG_HEAP_SIZE);
	context_set(&ctx, FADDR(main_bsp_separated_stack), config.base + kernel_size, CONFIG_STACK_SIZE);
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

	int a;
	vm_t *m;
	task_t *k;
	thread_t *t;
	
	the_initialize(THE);
	
	arch_pre_mm_init();
	heap_init(config.base + hardcoded_ktext_size + hardcoded_kdata_size, CONFIG_HEAP_SIZE + heap_delta);
	frame_init();
	page_init();
	tlb_init();

	arch_post_mm_init();

	printf("%s\n%s\n", project, copyright);
	printf("%P: hardcoded_ktext_size=%dK, hardcoded_kdata_size=%dK\n",
		config.base, hardcoded_ktext_size/1024, hardcoded_kdata_size/1024);

	arch_late_init();
	
	smp_init();
	printf("config.memory_size=%dM\n", config.memory_size/(1024*1024));
	printf("config.cpu_count=%d\n", config.cpu_count);

	cpu_init();

	calibrate_delay_loop();
	
	timeout_init();
	scheduler_init();
	task_init();
	thread_init();

	/*
	 * Create kernel vm mapping.
	 */
	m = vm_create(GET_PTL0_ADDRESS());
	if (!m)
		panic("can't create kernel vm address space\n");

	/*
	 * Create kernel task.
	 */
	k = task_create(m);
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


#ifdef __SMP__
/** Application CPUs main kernel routine
 *
 * Executed by application processors, temporary stack
 * is at ctx.sp which was set during BP boot.
 *
 * Assuming  cpu_priority_high().
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
	context_set(&CPU->saved_context, FADDR(main_ap_separated_stack), CPU->stack, CPU_STACK_SIZE);
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
#endif /* __SMP__*/
