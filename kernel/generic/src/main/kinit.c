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
 * @brief Kernel initialization thread.
 *
 * This file contains kinit kernel thread which carries out
 * high level system initialization.
 *
 * This file is responsible for finishing SMP configuration
 * and creation of userspace init tasks.
 */

#include <assert.h>
#include <main/kinit.h>
#include <config.h>
#include <arch.h>
#include <proc/scheduler.h>
#include <proc/task.h>
#include <proc/thread.h>
#include <proc/program.h>
#include <panic.h>
#include <halt.h>
#include <cpu.h>
#include <arch/asm.h>
#include <mm/page.h>
#include <arch/mm/page.h>
#include <mm/as.h>
#include <mm/frame.h>
#include <mm/km.h>
#include <print.h>
#include <log.h>
#include <mem.h>
#include <console/console.h>
#include <interrupt.h>
#include <console/kconsole.h>
#include <security/perm.h>
#include <lib/rd.h>
#include <ipc/ipc.h>
#include <str.h>
#include <sysinfo/stats.h>
#include <sysinfo/sysinfo.h>
#include <align.h>

#ifdef CONFIG_SMP
#include <smp/smp.h>
#endif /* CONFIG_SMP */

#include <synch/waitq.h>
#include <synch/spinlock.h>
#include <synch/workqueue.h>
#include <synch/rcu.h>

#define ALIVE_CHARS  4

#ifdef CONFIG_KCONSOLE
static char alive[ALIVE_CHARS] = "-\\|/";
#endif

#define INIT_PREFIX      "init:"
#define INIT_PREFIX_LEN  5

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
	thread_t *thread;
	
	/*
	 * Detach kinit as nobody will call thread_join_timeout() on it.
	 */
	thread_detach(THREAD);

	interrupts_disable();
	
	/* Start processing RCU callbacks. RCU is fully functional afterwards. */
	rcu_kinit_init();
	
	/*
	 * Start processing work queue items. Some may have been queued during boot.
	 */
	workq_global_worker_init();
	
#ifdef CONFIG_SMP
	if (config.cpu_count > 1) {
		waitq_initialize(&ap_completion_wq);
		
		/*
		 * Create the kmp thread and wait for its completion.
		 * cpu1 through cpuN-1 will come up consecutively and
		 * not mess together with kcpulb threads.
		 * Just a beautification.
		 */
		thread = thread_create(kmp, NULL, TASK,
		    THREAD_FLAG_UNCOUNTED, "kmp");
		if (thread != NULL) {
			thread_wire(thread, &cpus[0]);
			thread_ready(thread);
		} else
			panic("Unable to create kmp thread.");
		
		thread_join(thread);
		thread_detach(thread);
		
		/*
		 * For each CPU, create its load balancing thread.
		 */
		unsigned int i;
		
		for (i = 0; i < config.cpu_count; i++) {
			thread = thread_create(kcpulb, NULL, TASK,
			    THREAD_FLAG_UNCOUNTED, "kcpulb");
			if (thread != NULL) {
				thread_wire(thread, &cpus[i]);
				thread_ready(thread);
			} else
				log(LF_OTHER, LVL_ERROR,
				    "Unable to create kcpulb thread for cpu%u", i);
		}
	}
#endif /* CONFIG_SMP */
	
	/*
	 * At this point SMP, if present, is configured.
	 */
	ARCH_OP(post_smp_init);
	
	/* Start thread computing system load */
	thread = thread_create(kload, NULL, TASK, THREAD_FLAG_NONE,
	    "kload");
	if (thread != NULL)
		thread_ready(thread);
	else
		log(LF_OTHER, LVL_ERROR, "Unable to create kload thread");
	
#ifdef CONFIG_KCONSOLE
	if (stdin) {
		/*
		 * Create kernel console.
		 */
		thread = thread_create(kconsole_thread, NULL, TASK,
		    THREAD_FLAG_NONE, "kconsole");
		if (thread != NULL)
			thread_ready(thread);
		else
			log(LF_OTHER, LVL_ERROR,
			    "Unable to create kconsole thread");
	}
#endif /* CONFIG_KCONSOLE */
	
	/*
	 * Store the default stack size in sysinfo so that uspace can create
	 * stack with this default size.
	 */
	sysinfo_set_item_val("default.stack_size", NULL, STACK_SIZE_USER);
	
	interrupts_enable();
	
	/*
	 * Create user tasks, load RAM disk images.
	 */
	size_t i;
	program_t programs[CONFIG_INIT_TASKS];
	
	// FIXME: do not propagate arguments through sysinfo
	// but pass them directly to the tasks
	for (i = 0; i < init.cnt; i++) {
		const char *arguments = init.tasks[i].arguments;
		if (str_length(arguments) == 0)
			continue;
		if (str_length(init.tasks[i].name) == 0)
			continue;
		size_t arguments_size = str_size(arguments);

		void *arguments_copy = malloc(arguments_size, 0);
		if (arguments_copy == NULL)
			continue;
		memcpy(arguments_copy, arguments, arguments_size);

		char item_name[CONFIG_TASK_NAME_BUFLEN + 15];
		snprintf(item_name, CONFIG_TASK_NAME_BUFLEN + 15,
		    "init_args.%s", init.tasks[i].name);

		sysinfo_set_item_data(item_name, NULL, arguments_copy, arguments_size);
	}

	for (i = 0; i < init.cnt; i++) {
		if (init.tasks[i].paddr % FRAME_SIZE) {
			log(LF_OTHER, LVL_ERROR,
			    "init[%zu]: Address is not frame aligned", i);
			programs[i].task = NULL;
			continue;
		}
		
		/*
		 * Construct task name from the 'init:' prefix and the
		 * name stored in the init structure (if any).
		 */
		
		char namebuf[TASK_NAME_BUFLEN];
		
		const char *name = init.tasks[i].name;
		if (name[0] == 0)
			name = "<unknown>";
		
		static_assert(TASK_NAME_BUFLEN >= INIT_PREFIX_LEN, "");
		str_cpy(namebuf, TASK_NAME_BUFLEN, INIT_PREFIX);
		str_cpy(namebuf + INIT_PREFIX_LEN,
		    TASK_NAME_BUFLEN - INIT_PREFIX_LEN, name);
		
		/*
		 * Create virtual memory mappings for init task images.
		 */
		uintptr_t page = km_map(init.tasks[i].paddr,
		    init.tasks[i].size,
		    PAGE_READ | PAGE_WRITE | PAGE_CACHEABLE);
		assert(page);
		
		errno_t rc = program_create_from_image((void *) page, namebuf,
		    &programs[i]);
		
		if (rc == 0) {
			if (programs[i].task != NULL) {
				/*
				 * Set permissions to init userspace tasks.
				 */
				perm_set(programs[i].task,
				    PERM_PERM | PERM_MEM_MANAGER |
				    PERM_IO_MANAGER | PERM_IRQ_REG);
				
				if (!ipc_phone_0) {
					ipc_phone_0 = &programs[i].task->answerbox;
					/*
					 * Hold the first task so that the
					 * ipc_phone_0 remains a valid pointer
					 * even if the first task exits for
					 * whatever reason.
					 */
					task_hold(programs[i].task);
				}
			}
			
			/*
			 * If programs[i].task == NULL then it is
			 * the program loader and it was registered
			 * successfully.
			 */
		} else if (i == init.cnt - 1) {
			/*
			 * Assume the last task is the RAM disk.
			 */
			init_rd((void *) init.tasks[i].paddr, init.tasks[i].size);
		} else
			log(LF_OTHER, LVL_ERROR,
			    "init[%zu]: Init binary load failed "
			    "(error %s, loader status %u)", i,
			    str_error_name(rc), programs[i].loader_status);
	}
	
	/*
	 * Run user tasks.
	 */
	for (i = 0; i < init.cnt; i++) {
		if (programs[i].task != NULL)
			program_ready(&programs[i]);
	}
	
#ifdef CONFIG_KCONSOLE
	if (!stdin) {
		thread_sleep(10);
		printf("kinit: No stdin\nKernel alive: .");
		
		unsigned int i = 0;
		while (true) {
			printf("\b%c", alive[i % ALIVE_CHARS]);
			thread_sleep(1);
			i++;
		}
	}
#endif /* CONFIG_KCONSOLE */
}

/** @}
 */
