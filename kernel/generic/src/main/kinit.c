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
 * @brief	Kernel initialization thread.
 *
 * This file contains kinit kernel thread which carries out
 * high level system initialization.
 *
 * This file is responsible for finishing SMP configuration
 * and creation of userspace init tasks.
 */

#include <main/kinit.h>
#include <config.h>
#include <arch.h>
#include <proc/scheduler.h>
#include <proc/task.h>
#include <proc/thread.h>
#include <panic.h>
#include <func.h>
#include <cpu.h>
#include <arch/asm.h>
#include <mm/page.h>
#include <arch/mm/page.h>
#include <mm/as.h>
#include <mm/frame.h>
#include <print.h>
#include <memstr.h>
#include <console/console.h>
#include <interrupt.h>
#include <console/kconsole.h>
#include <security/cap.h>
#include <lib/rd.h>

#ifdef CONFIG_SMP
#include <smp/smp.h>
#endif /* CONFIG_SMP */

#include <synch/waitq.h>
#include <synch/spinlock.h>

/** Kernel initialization thread.
 *
 * kinit takes care of higher level kernel
 * initialization (i.e. thread creation,
 * userspace initialization etc.).
 *
 * @param arg Not used.
 */
void kinit(void *arg)
{
	thread_t *t;

	/*
	 * Detach kinit as nobody will call thread_join_timeout() on it.
	 */
	thread_detach(THREAD);

	interrupts_disable();

#ifdef CONFIG_SMP		 	
	if (config.cpu_count > 1) {
		waitq_initialize(&ap_completion_wq);
		/*
		 * Create the kmp thread and wait for its completion.
		 * cpu1 through cpuN-1 will come up consecutively and
		 * not mess together with kcpulb threads.
		 * Just a beautification.
		 */
		if ((t = thread_create(kmp, NULL, TASK, THREAD_FLAG_WIRED,
		    "kmp", true))) {
			spinlock_lock(&t->lock);
			t->cpu = &cpus[0];
			spinlock_unlock(&t->lock);
			thread_ready(t);
		} else
			panic("thread_create/kmp\n");
		thread_join(t);
		thread_detach(t);
	}
#endif /* CONFIG_SMP */
	/*
	 * Now that all CPUs are up, we can report what we've found.
	 */
	cpu_list();

#ifdef CONFIG_SMP
	if (config.cpu_count > 1) {
		int i;
		
		/*
		 * For each CPU, create its load balancing thread.
		 */
		for (i = 0; i < config.cpu_count; i++) {

			if ((t = thread_create(kcpulb, NULL, TASK,
			    THREAD_FLAG_WIRED, "kcpulb", true))) {
				spinlock_lock(&t->lock);			
				t->cpu = &cpus[i];
				spinlock_unlock(&t->lock);
				thread_ready(t);
			} else
				panic("thread_create/kcpulb\n");

		}
	}
#endif /* CONFIG_SMP */

	/*
	 * At this point SMP, if present, is configured.
	 */
	arch_post_smp_init();

	/*
	 * Create kernel console.
	 */
	t = thread_create(kconsole, "kconsole", TASK, 0, "kconsole", false);
	if (t)
		thread_ready(t);
	else
		panic("thread_create/kconsole\n");

	interrupts_enable();

	count_t i;
	for (i = 0; i < init.cnt; i++) {
		/*
		 * Run user tasks, load RAM disk images.
		 */
		
		if (init.tasks[i].addr % FRAME_SIZE) {
			printf("init[%d].addr is not frame aligned", i);
			continue;
		}

		task_t *utask = task_run_program((void *) init.tasks[i].addr,
		    "uspace");
		if (utask) {
			/*
			 * Set capabilities to init userspace tasks.
			 */
			cap_set(utask, CAP_CAP | CAP_MEM_MANAGER |
			    CAP_IO_MANAGER | CAP_PREEMPT_CONTROL | CAP_IRQ_REG);
			
			if (!ipc_phone_0) 
				ipc_phone_0 = &utask->answerbox;
		} else {
			int rd = init_rd((rd_header *) init.tasks[i].addr,
			    init.tasks[i].size);
			
			if (rd != RE_OK)
				printf("Init binary %zd not used.\n", i);
		}
	}


	if (!stdin) {
		while (1) {
			thread_sleep(1);
			printf("kinit... ");
		}
	}
}

/** @}
 */
