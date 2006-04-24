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
#include <adt/btree.h>
#include <adt/list.h>
#include <ipc/ipc.h>
#include <security/cap.h>
#include <memstr.h>
#include <print.h>
#include <elf.h>


#ifndef LOADED_PROG_STACK_PAGES_NO
#define LOADED_PROG_STACK_PAGES_NO 1
#endif

SPINLOCK_INITIALIZE(tasks_lock);
btree_t tasks_btree;
static task_id_t task_counter = 0;

/** Initialize tasks
 *
 * Initialize kernel tasks support.
 *
 */
void task_init(void)
{
	TASK = NULL;
	btree_create(&tasks_btree);
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

	task_create_arch(ta);

	spinlock_initialize(&ta->lock, "task_ta_lock");
	list_initialize(&ta->th_head);
	ta->as = as;
	ta->name = name;

	ta->capabilities = 0;
	
	ipc_answerbox_init(&ta->answerbox);
	for (i=0; i < IPC_MAX_PHONES;i++)
		ipc_phone_init(&ta->phones[i]);
	if (ipc_phone_0)
		ipc_phone_connect(&ta->phones[0], ipc_phone_0);
	atomic_set(&ta->active_calls, 0);
	
	ipl = interrupts_disable();
	spinlock_lock(&tasks_lock);

	ta->taskid = ++task_counter;
	btree_insert(&tasks_btree, (btree_key_t) ta->taskid, (void *) ta, NULL);

	spinlock_unlock(&tasks_lock);
	interrupts_restore(ipl);

	return ta;
}

/** Create new task with 1 thread and run it
 *
 * @param programe_addr Address of program executable image.
 * @param name Program name. 
 *
 * @return Task of the running program or NULL on error.
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
	ASSERT(as);

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
	ASSERT(task);

	/*
	 * Create the data as_area.
	 */
	a = as_area_create(as, AS_AREA_READ | AS_AREA_WRITE, LOADED_PROG_STACK_PAGES_NO*PAGE_SIZE, USTACK_ADDRESS);

	t = thread_create(uinit, kernel_uarg, task, 0, "uinit");
	ASSERT(t);
	thread_ready(t);
	
	return task;
}

/** Syscall for reading task ID from userspace.
 *
 * @param uaddr Userspace address of 8-byte buffer where to store current task ID.
 *
 * @return Always returns 0.
 */
__native sys_task_get_id(task_id_t *uspace_task_id)
{
	/*
	 * No need to acquire lock on TASK because taskid
	 * remains constant for the lifespan of the task.
	 */
	copy_to_uspace(uspace_task_id, &TASK->taskid, sizeof(TASK->taskid));

	return 0;
}

/** Find task structure corresponding to task ID.
 *
 * The tasks_lock must be already held by the caller of this function
 * and interrupts must be disabled.
 *
 * @param id Task ID.
 *
 * @return Task structure address or NULL if there is no such task ID.
 */
task_t *task_find_by_id(task_id_t id)
{
	btree_node_t *leaf;
	
	return (task_t *) btree_search(&tasks_btree, (btree_key_t) id, &leaf);
}

/** Print task list */
void task_print_list(void)
{
	link_t *cur;
	ipl_t ipl;
	
	/* Messing with thread structures, avoid deadlock */
	ipl = interrupts_disable();
	spinlock_lock(&tasks_lock);

	for (cur = tasks_btree.leaf_head.next; cur != &tasks_btree.leaf_head; cur = cur->next) {
		btree_node_t *node;
		int i;
		
		node = list_get_instance(cur, btree_node_t, leaf_link);
		for (i = 0; i < node->keys; i++) {
			task_t *t;
			int j;

			t = (task_t *) node->value[i];
		
			spinlock_lock(&t->lock);
			printf("%s: address=%#zX, taskid=%#llX, as=%#zX, ActiveCalls: %zd",
				t->name, t, t->taskid, t->as, atomic_get(&t->active_calls));
			for (j=0; j < IPC_MAX_PHONES; j++) {
				if (t->phones[j].callee)
					printf(" Ph(%zd): %#zX ", j, t->phones[j].callee);
			}
			printf("\n");
			spinlock_unlock(&t->lock);
		}
	}

	spinlock_unlock(&tasks_lock);
	interrupts_restore(ipl);
}
