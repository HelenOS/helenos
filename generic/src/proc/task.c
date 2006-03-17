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
#include <proc/uarg.h>
#include <mm/as.h>
#include <mm/slab.h>
#include <synch/spinlock.h>
#include <arch.h>
#include <panic.h>
#include <adt/list.h>
#include <ipc/ipc.h>
#include <memstr.h>
#include <print.h>
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
 * @param name Symbolic name.
 *
 * @return New task's structure
 *
 */
task_t *task_create(as_t *as, char *name)
{
	ipl_t ipl;
	task_t *ta;
	int i;
	
	ta = (task_t *) malloc(sizeof(task_t), 0);

	spinlock_initialize(&ta->lock, "task_ta_lock");
	list_initialize(&ta->th_head);
	list_initialize(&ta->tasks_link);
	ta->as = as;
	ta->name = name;

	
	ipc_answerbox_init(&ta->answerbox);
	for (i=0; i < IPC_MAX_PHONES;i++)
		ipc_phone_init(&ta->phones[i]);
	if (ipc_phone_0)
		ipc_phone_connect(&ta->phones[0], ipc_phone_0);
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
 * @param programe_addr Address of program executable image.
 * @param name Program name. 
 *
 * @return Task of the running program or NULL on error
 */
task_t * task_run_program(void *program_addr, char *name)
{
	as_t *as;
	as_area_t *a;
	int rc;
	thread_t *t;
	task_t *task;
	uspace_arg_t *kernel_uarg;

	as = as_create(0);

	rc = elf_load((elf_header_t *) program_addr, as);
	if (rc != EE_OK) {
		as_free(as);
		return NULL;
	} 
	
	kernel_uarg = (uspace_arg_t *) malloc(sizeof(uspace_arg_t), 0);
	kernel_uarg->uspace_entry = (void *) ((elf_header_t *) program_addr)->e_entry;
	kernel_uarg->uspace_stack = (void *) USTACK_ADDRESS;
	kernel_uarg->uspace_thread_function = NULL;
	kernel_uarg->uspace_thread_arg = NULL;
	kernel_uarg->uspace_uarg = NULL;
	
	task = task_create(as, name);
	t = thread_create(uinit, kernel_uarg, task, 0, "uinit");
	
	/*
	 * Create the data as_area.
	 */
	a = as_area_create(as, AS_AREA_READ | AS_AREA_WRITE, PAGE_SIZE, USTACK_ADDRESS);
	
	thread_ready(t);

	return task;
}

/** Print task list */
void task_print_list(void)
{
	link_t *cur;
	task_t *t;
	ipl_t ipl;
	int i;
	
	/* Messing with thread structures, avoid deadlock */
	ipl = interrupts_disable();
	spinlock_lock(&tasks_lock);

	for (cur=tasks_head.next; cur!=&tasks_head; cur=cur->next) {
		t = list_get_instance(cur, task_t, tasks_link);
		spinlock_lock(&t->lock);
		printf("%s: address=%P, taskid=%Q, as=%P, ActiveCalls: %d",
			t->name, t, t->taskid, t->as, atomic_get(&t->active_calls));
		for (i=0; i < IPC_MAX_PHONES; i++) {
			if (t->phones[i].callee)
				printf(" Ph(%d): %P ", i,t->phones[i].callee);
		}
		printf("\n");
		spinlock_unlock(&t->lock);
	}

	spinlock_unlock(&tasks_lock);
	interrupts_restore(ipl);
	
}
