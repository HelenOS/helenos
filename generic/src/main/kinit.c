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

#include <config.h>
#include <arch.h>
#include <main/kinit.h>
#include <main/uinit.h>
#include <proc/scheduler.h>
#include <proc/task.h>
#include <proc/thread.h>
#include <panic.h>
#include <func.h>
#include <cpu.h>
#include <arch/asm.h>
#include <mm/page.h>
#include <arch/mm/page.h>
#include <mm/vm.h>
#include <print.h>
#include <memstr.h>

#ifdef __SMP__
#include <arch/smp/mps.h>
#endif /* __SMP__ */

#include <synch/waitq.h>
#include <synch/spinlock.h>

#ifdef __TEST__
#include <test.h>
#endif /* __TEST__ */

#include <mm/frame.h>

void kinit(void *arg)
{
	vm_t *m;
	vm_area_t *a;
	task_t *u;
	thread_t *t;
	int i;

	interrupts_disable();

#ifdef __SMP__		 	
	if (config.cpu_count > 1) {
		/*
		 * Create the kmp thread and wait for its completion.
		 * cpu1 through cpuN-1 will come up consecutively and
		 * not mess together with kcpulb threads.
		 * Just a beautification.
		 */
		if (t = thread_create(kmp, NULL, TASK, 0)) {
			spinlock_lock(&t->lock);
			t->flags |= X_WIRED;
			t->cpu = &cpus[0];
			spinlock_unlock(&t->lock);
			thread_ready(t);
			waitq_sleep(&kmp_completion_wq);
		}
		else panic("thread_create/kmp");
	}
#endif /* __SMP__ */
	/*
	 * Now that all CPUs are up, we can report what we've found.
	 */
	for (i = 0; i < config.cpu_count; i++) {
		if (cpus[i].active)
			cpu_print_report(&cpus[i]);
		else
			printf("cpu%d: not active\n", i);
	}

#ifdef __SMP__
	if (config.cpu_count > 1) {
		/*
		 * For each CPU, create its load balancing thread.
		 */
		for (i = 0; i < config.cpu_count; i++) {

			if (t = thread_create(kcpulb, NULL, TASK, 0)) {
				spinlock_lock(&t->lock);			
				t->flags |= X_WIRED;
				t->cpu = &cpus[i];
				spinlock_unlock(&t->lock);
				thread_ready(t);
			}
			else panic("thread_create/kcpulb");

		}
	}
#endif /* __SMP__ */

	interrupts_enable();

#ifdef __USERSPACE__
	/*
	 * Create the first user task.
	 */
	m = vm_create(NULL);
	if (!m)	panic("vm_create");
	u = task_create(m);
	if (!u)	panic("task_create");
	t = thread_create(uinit, NULL, u, THREAD_USER_STACK);
	if (!t) panic("thread_create");

	/*
	 * Create the text vm_area and copy the userspace code there.
	 */	
	a = vm_area_create(m, VMA_TEXT, 1, UTEXT_ADDRESS);
	if (!a) panic("vm_area_create: vm_text");
	vm_area_map(a, m);
	memcpy((void *) PA2KA(a->mapping[0]), (void *) utext, utext_size < PAGE_SIZE ? utext_size : PAGE_SIZE);

	/*
	 * Create the data vm_area.
	 */
	a = vm_area_create(m, VMA_STACK, 1, USTACK_ADDRESS);
	if (!a) panic("vm_area_create: vm_stack");
	vm_area_map(a, m);	
	
	thread_ready(t);
#endif /* __USERSPACE__ */

#ifdef __TEST__
	test();
#endif /* __TEST__ */


	while (1) {
		thread_usleep(1000000);
		printf("kinit... ");
	}

}
