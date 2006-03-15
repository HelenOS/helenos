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

#include <main/uinit.h>
#include <proc/thread.h>
#include <proc/task.h>
#include <mm/as.h>
#include <mm/slab.h>

#include <synch/spinlock.h>
#include <arch.h>
#include <panic.h>
#include <adt/list.h>
#include <ipc/ipc.h>
#include <memstr.h>

#include <elf.h>

SPINLOCK_INITIALIZE(tasks_lock);
LIST_INITIALIZE(tasks_head);
static task_id_t task_counter = 0;

/** Initialize tasks
 *
 * Initialize kernel tasks support.
 *
 */
void task_init(void)
{
	TASK = NULL;
}


/** Create new task
 *
 * Create new task with no threads.
 *
 * @param as Task's address space.
 *
 * @return New task's structure
 *
 */
task_t *task_create(as_t *as)
{
	ipl_t ipl;
	task_t *ta;
	
	ta = (task_t *) malloc(sizeof(task_t), 0);

	spinlock_initialize(&ta->lock, "task_ta_lock");
	list_initialize(&ta->th_head);
	list_initialize(&ta->tasks_link);
	ta->as = as;

	ipc_answerbox_init(&ta->answerbox);
	memsetb((__address)&ta->phones, sizeof(ta->phones[0])*IPC_MAX_PHONES, 0);
	if (ipc_phone_0)
		ipc_phone_init(&ta->phones[0], ipc_phone_0);
	atomic_set(&ta->active_calls, 0);
	
	ipl = interrupts_disable();
	spinlock_lock(&tasks_lock);

	ta->taskid = ++task_counter;
	list_append(&ta->tasks_link, &tasks_head);

	spinlock_unlock(&tasks_lock);
	interrupts_restore(ipl);

	return ta;
}

/** Create new task with 1 thread and run it
 *
 * @return Task of the running program or NULL on error
 */
task_t * task_run_program(void *program_addr)
{
	as_t *as;
	as_area_t *a;
	int rc;
	thread_t *t;
	task_t *task;

	as = as_create(0);

	rc = elf_load((elf_header_t *) program_addr, as);
	if (rc != EE_OK) {
		as_free(as);
		return NULL;
	} 
	
	task = task_create(as);
	t = thread_create(uinit, (void *)((elf_header_t *)program_addr)->e_entry, 
			  task, THREAD_USER_STACK);
	
	/*
	 * Create the data as_area.
	 */
	a = as_area_create(as, AS_AREA_READ | AS_AREA_WRITE, 1, USTACK_ADDRESS);
	
	thread_ready(t);

	return task;
}
