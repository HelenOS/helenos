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

/**
 * @file	task.c
 * @brief	Task management.
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
#include <errno.h>
#include <syscall/copy.h>

#ifndef LOADED_PROG_STACK_PAGES_NO
#define LOADED_PROG_STACK_PAGES_NO 1
#endif

SPINLOCK_INITIALIZE(tasks_lock);
btree_t tasks_btree;
static task_id_t task_counter = 0;

static void ktask_cleanup(void *);

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

	ta->refcount = 0;

	ta->capabilities = 0;
	ta->accept_new_threads = true;
	
	ipc_answerbox_init(&ta->answerbox);
	for (i=0; i < IPC_MAX_PHONES;i++)
		ipc_phone_init(&ta->phones[i]);
	if (ipc_phone_0)
		ipc_phone_connect(&ta->phones[0], ipc_phone_0);
	atomic_set(&ta->active_calls, 0);

	mutex_initialize(&ta->futexes_lock);
	btree_create(&ta->futexes);
	
	ipl = interrupts_disable();

	/*
	 * Increment address space reference count.
	 * TODO: Reconsider the locking scheme.
	 */
	mutex_lock(&as->lock);
	as->refcount++;
	mutex_unlock(&as->lock);

	spinlock_lock(&tasks_lock);

	ta->taskid = ++task_counter;
	btree_insert(&tasks_btree, (btree_key_t) ta->taskid, (void *) ta, NULL);

	spinlock_unlock(&tasks_lock);
	interrupts_restore(ipl);

	return ta;
}

/** Destroy task.
 *
 * @param t Task to be destroyed.
 */
void task_destroy(task_t *t)
{
}

/** Create new task with 1 thread and run it
 *
 * @param program_addr Address of program executable image.
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
		as_destroy(as);
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
	a = as_area_create(as, AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE, 
		LOADED_PROG_STACK_PAGES_NO*PAGE_SIZE,
		USTACK_ADDRESS, AS_AREA_ATTR_NONE, &anon_backend, NULL);

	t = thread_create(uinit, kernel_uarg, task, 0, "uinit");
	ASSERT(t);
	thread_ready(t);
	
	return task;
}

/** Syscall for reading task ID from userspace.
 *
 * @param uspace_task_id Userspace address of 8-byte buffer where to store current task ID.
 *
 * @return 0 on success or an error code from @ref errno.h.
 */
__native sys_task_get_id(task_id_t *uspace_task_id)
{
	/*
	 * No need to acquire lock on TASK because taskid
	 * remains constant for the lifespan of the task.
	 */
	return (__native) copy_to_uspace(uspace_task_id, &TASK->taskid, sizeof(TASK->taskid));
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

/** Kill task.
 *
 * @param id ID of the task to be killed.
 *
 * @return 0 on success or an error code from errno.h
 */
int task_kill(task_id_t id)
{
	ipl_t ipl;
	task_t *ta;
	thread_t *t;
	link_t *cur;
	
	ipl = interrupts_disable();
	spinlock_lock(&tasks_lock);

	if (!(ta = task_find_by_id(id))) {
		spinlock_unlock(&tasks_lock);
		interrupts_restore(ipl);
		return ENOENT;
	}
	
	spinlock_lock(&ta->lock);
	ta->refcount++;
	spinlock_unlock(&ta->lock);
	
	t = thread_create(ktask_cleanup, NULL, ta, 0, "ktask_cleanup");
	
	spinlock_lock(&ta->lock);
	ta->refcount--;
	
	for (cur = ta->th_head.next; cur != &ta->th_head; cur = cur->next) {
		thread_t *thr;
		bool  sleeping = false;
		
		thr = list_get_instance(cur, thread_t, th_link);
		if (thr == t)
			continue;
			
		spinlock_lock(&thr->lock);
		thr->interrupted = true;
		if (thr->state == Sleeping)
			sleeping = true;
		spinlock_unlock(&thr->lock);
		
		if (sleeping)
			waitq_interrupt_sleep(thr);
	}
	
	thread_ready(t);
	
	return 0;
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
			printf("%s(%lld): address=%#zX, as=%#zX, ActiveCalls: %zd",
				t->name, t->taskid, t, t->as, atomic_get(&t->active_calls));
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

/** Kernel thread used to cleanup the task. */
void ktask_cleanup(void *arg)
{
	/*
	 * TODO:
	 * Wait until it is save to cleanup the task (i.e. all other threads exit)
	 * and do the cleanup (e.g. close IPC communication and release used futexes).
	 * When this thread exits, the task refcount drops to zero and the task structure is
	 * cleaned.
	 */
}
