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
#include <arch/context.h>
#include <print.h>
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
#include <arch/apic.h>
#include <arch/mp.h>
#endif /* __SMP__ */

#include <mm/frame.h>
#include <mm/page.h>
#include <synch/waitq.h>

#include <arch.h>

char *project = "SPARTAN kernel";
char *copyright = "Copyright (C) 2001-2005 Jakub Jermar";

config_t config;
context_t ctx;

/*
 * These 'hardcoded' variables will be intialised by
 * the linker with appropriate sizes and addresses.
 */
__u32 hardcoded_load_address = 0;
__u32 hardcoded_ktext_size = 0;
__u32 hardcoded_kdata_size = 0;

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

/*
 * Executed by the bootstrap processor. cpu_priority_high()'d
 */
void main_bsp(void)
{
	config.cpu_count = 1;
	config.cpu_active = 1;

	config.base = hardcoded_load_address;
	config.memory_size = CONFIG_MEMORY_SIZE;
	config.kernel_size = hardcoded_ktext_size + hardcoded_kdata_size + CONFIG_HEAP_SIZE + CONFIG_STACK_SIZE;

	context_save(&ctx);
	ctx.sp = config.base + config.kernel_size - 8;
	ctx.pc = (__address) main_bsp_separated_stack;
	context_restore(&ctx);
	/* not reached */
}

void main_bsp_separated_stack(void) {
	vm_t *m;
	task_t *k;
	thread_t *t;

	arch_init();

	printf("%s, %s\n", project, copyright);

	printf("%L: hardcoded_ktext_size=%dK, hardcoded_kdata_size=%dK\n",
		config.base, hardcoded_ktext_size/1024, hardcoded_kdata_size/1024);

	heap_init(config.base + hardcoded_ktext_size + hardcoded_kdata_size, CONFIG_HEAP_SIZE);
	frame_init();
	page_init();

	#ifdef __SMP__
	mp_init();	/* Multiprocessor */
	#endif /* __SMP__ */

	cpu_init();
	calibrate_delay_loop();

	timeout_init();
	scheduler_init();
	task_init();
	thread_init();

	/*
	 * Create kernel vm mapping.
	 */
	m = vm_create();
	if (!m) panic("can't create kernel vm address space\n");

	/*
	 * Create kernel task.
	 */
	k = task_create(m);
	if (!k) panic("can't create kernel task\n");

	/*
	 * Create the first thread.
	 */
	t = thread_create(kinit, NULL, k, 0);
	if (!t) panic("can't create kinit thread\n");

	thread_ready(t);

	/*
	 * This call to scheduler() will return to kinit,
	 * starting the thread of kernel threads.
	 */
	scheduler();
	/* not reached */
}

#ifdef __SMP__
/*
 * Executed by application processors. cpu_priority_high()'d
 * Temporary stack is at ctx.sp which was set during BP boot.
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

	arch_init();
	frame_init();
	page_init();

	cpu_init();
	calibrate_delay_loop();

	l_apic_init();
	l_apic_debug();


	/*
	 * If we woke kmp up before we left the kernel stack, we could
	 * collide with another CPU coming up. To prevent this, we
	 * switch to this cpu's private stack prior to waking kmp up.
	 */
	the->cpu->saved_context.sp = (__address) &the->cpu->stack[CPU_STACK_SIZE-8];
	the->cpu->saved_context.pc = (__address) main_ap_separated_stack;
	context_restore(&the->cpu->saved_context);
	/* not reached */
}

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
