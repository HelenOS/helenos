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

/** @addtogroup genericproc
 * @{
 */

/**
 * @file
 * @brief	Task management.
 */

#include <proc/thread.h>
#include <proc/task.h>
#include <mm/as.h>
#include <mm/slab.h>
#include <atomic.h>
#include <synch/spinlock.h>
#include <synch/waitq.h>
#include <arch.h>
#include <arch/barrier.h>
#include <adt/avl.h>
#include <adt/btree.h>
#include <adt/list.h>
#include <ipc/ipc.h>
#include <ipc/ipcrsc.h>
#include <print.h>
#include <errno.h>
#include <func.h>
#include <string.h>
#include <memstr.h>
#include <syscall/copy.h>
#include <macros.h>
#include <ipc/event.h>

/** Spinlock protecting the tasks_tree AVL tree. */
SPINLOCK_INITIALIZE(tasks_lock);

/** AVL tree of active tasks.
 *
 * The task is guaranteed to exist after it was found in the tasks_tree as
 * long as:
 * @li the tasks_lock is held,
 * @li the task's lock is held when task's lock is acquired before releasing
 *     tasks_lock or
 * @li the task's refcount is greater than 0
 *
 */
avltree_t tasks_tree;

static task_id_t task_counter = 0;

static slab_cache_t *task_slab;

/* Forward declarations. */
static void task_kill_internal(task_t *);
static int tsk_constructor(void *, int);

/** Initialize kernel tasks support. */
void task_init(void)
{
	TASK = NULL;
	avltree_create(&tasks_tree);
	task_slab = slab_cache_create("task_slab", sizeof(task_t), 0,
	    tsk_constructor, NULL, 0);
}

/*
 * The idea behind this walker is to kill and count all tasks different from
 * TASK.
 */
static bool task_done_walker(avltree_node_t *node, void *arg)
{
	task_t *t = avltree_get_instance(node, task_t, tasks_tree_node);
	unsigned *cnt = (unsigned *) arg;

	if (t != TASK) {
		(*cnt)++;
#ifdef CONFIG_DEBUG
		printf("[%"PRIu64"] ", t->taskid);
#endif
		task_kill_internal(t);
	}

	return true;	/* continue the walk */
}

/** Kill all tasks except the current task. */
void task_done(void)
{
	unsigned tasks_left;

	do { /* Repeat until there are any tasks except TASK */
		/* Messing with task structures, avoid deadlock */
#ifdef CONFIG_DEBUG
		printf("Killing tasks... ");
#endif
		ipl_t ipl = interrupts_disable();
		spinlock_lock(&tasks_lock);
		tasks_left = 0;
		avltree_walk(&tasks_tree, task_done_walker, &tasks_left);
		spinlock_unlock(&tasks_lock);
		interrupts_restore(ipl);
		thread_sleep(1);
#ifdef CONFIG_DEBUG
		printf("\n");
#endif
	} while (tasks_left);
}

int tsk_constructor(void *obj, int kmflags)
{
	task_t *ta = obj;
	int i;

	atomic_set(&ta->refcount, 0);
	atomic_set(&ta->lifecount, 0);
	atomic_set(&ta->active_calls, 0);

	spinlock_initialize(&ta->lock, "task_ta_lock");
	mutex_initialize(&ta->futexes_lock, MUTEX_PASSIVE);

	list_initialize(&ta->th_head);
	list_initialize(&ta->sync_box_head);

	ipc_answerbox_init(&ta->answerbox, ta);
	for (i = 0; i < IPC_MAX_PHONES; i++)
		ipc_phone_init(&ta->phones[i]);

#ifdef CONFIG_UDEBUG
	/* Init kbox stuff */
	ta->kb.thread = NULL;
	ipc_answerbox_init(&ta->kb.box, ta);
	mutex_initialize(&ta->kb.cleanup_lock, MUTEX_PASSIVE);
#endif

	return 0;
}

/** Create new task with no threads.
 *
 * @param as		Task's address space.
 * @param name		Symbolic name (a copy is made).
 *
 * @return		New task's structure.
 *
 */
task_t *task_create(as_t *as, char *name)
{
	ipl_t ipl;
	task_t *ta;
	
	ta = (task_t *) slab_alloc(task_slab, 0);
	task_create_arch(ta);
	ta->as = as;
	memcpy(ta->name, name, TASK_NAME_BUFLEN);
	ta->name[TASK_NAME_BUFLEN - 1] = 0;

	ta->context = CONTEXT;
	ta->capabilities = 0;
	ta->cycles = 0;

#ifdef CONFIG_UDEBUG
	/* Init debugging stuff */
	udebug_task_init(&ta->udebug);

	/* Init kbox stuff */
	ta->kb.finished = false;
#endif

	if ((ipc_phone_0) &&
	    (context_check(ipc_phone_0->task->context, ta->context)))
		ipc_phone_connect(&ta->phones[0], ipc_phone_0);

	btree_create(&ta->futexes);
	
	ipl = interrupts_disable();
	atomic_inc(&as->refcount);
	spinlock_lock(&tasks_lock);
	ta->taskid = ++task_counter;
	avltree_node_initialize(&ta->tasks_tree_node);
	ta->tasks_tree_node.key = ta->taskid; 
	avltree_insert(&tasks_tree, &ta->tasks_tree_node);
	spinlock_unlock(&tasks_lock);
	interrupts_restore(ipl);
	
	return ta;
}

/** Destroy task.
 *
 * @param t		Task to be destroyed.
 */
void task_destroy(task_t *t)
{
	/*
	 * Remove the task from the task B+tree.
	 */
	spinlock_lock(&tasks_lock);
	avltree_delete(&tasks_tree, &t->tasks_tree_node);
	spinlock_unlock(&tasks_lock);

	/*
	 * Perform architecture specific task destruction.
	 */
	task_destroy_arch(t);

	/*
	 * Free up dynamically allocated state.
	 */
	btree_destroy(&t->futexes);

	/*
	 * Drop our reference to the address space.
	 */
	if (atomic_predec(&t->as->refcount) == 0) 
		as_destroy(t->as);
	
	slab_free(task_slab, t);
	TASK = NULL;
}

/** Syscall for reading task ID from userspace.
 *
 * @param		uspace_task_id userspace address of 8-byte buffer
 * 			where to store current task ID.
 *
 * @return		Zero on success or an error code from @ref errno.h.
 */
unative_t sys_task_get_id(task_id_t *uspace_task_id)
{
	/*
	 * No need to acquire lock on TASK because taskid remains constant for
	 * the lifespan of the task.
	 */
	return (unative_t) copy_to_uspace(uspace_task_id, &TASK->taskid,
	    sizeof(TASK->taskid));
}

/** Syscall for setting the task name.
 *
 * The name simplifies identifying the task in the task list.
 *
 * @param name	The new name for the task. (typically the same
 *		as the command used to execute it).
 *
 * @return 0 on success or an error code from @ref errno.h.
 */
unative_t sys_task_set_name(const char *uspace_name, size_t name_len)
{
	int rc;
	char namebuf[TASK_NAME_BUFLEN];

	/* Cap length of name and copy it from userspace. */

	if (name_len > TASK_NAME_BUFLEN - 1)
		name_len = TASK_NAME_BUFLEN - 1;

	rc = copy_from_uspace(namebuf, uspace_name, name_len);
	if (rc != 0)
		return (unative_t) rc;

	namebuf[name_len] = '\0';
	str_cpy(TASK->name, TASK_NAME_BUFLEN, namebuf);

	return EOK;
}

/** Find task structure corresponding to task ID.
 *
 * The tasks_lock must be already held by the caller of this function and
 * interrupts must be disabled.
 *
 * @param id		Task ID.
 *
 * @return		Task structure address or NULL if there is no such task
 * 			ID.
 */
task_t *task_find_by_id(task_id_t id) { avltree_node_t *node;
	
	node = avltree_search(&tasks_tree, (avltree_key_t) id);

	if (node)
		return avltree_get_instance(node, task_t, tasks_tree_node); 
	return NULL;
}

/** Get accounting data of given task.
 *
 * Note that task lock of 't' must be already held and interrupts must be
 * already disabled.
 *
 * @param t		Pointer to thread.
 *
 * @return		Number of cycles used by the task and all its threads
 * 			so far.
 */
uint64_t task_get_accounting(task_t *t)
{
	/* Accumulated value of task */
	uint64_t ret = t->cycles;
	
	/* Current values of threads */
	link_t *cur;
	for (cur = t->th_head.next; cur != &t->th_head; cur = cur->next) {
		thread_t *thr = list_get_instance(cur, thread_t, th_link);
		
		spinlock_lock(&thr->lock);
		/* Process only counted threads */
		if (!thr->uncounted) {
			if (thr == THREAD) {
				/* Update accounting of current thread */
				thread_update_accounting();
			} 
			ret += thr->cycles;
		}
		spinlock_unlock(&thr->lock);
	}
	
	return ret;
}

static void task_kill_internal(task_t *ta)
{
	link_t *cur;

	/*
	 * Interrupt all threads.
	 */
	spinlock_lock(&ta->lock);
	for (cur = ta->th_head.next; cur != &ta->th_head; cur = cur->next) {
		thread_t *thr;
		bool sleeping = false;
		
		thr = list_get_instance(cur, thread_t, th_link);
		
		spinlock_lock(&thr->lock);
		thr->interrupted = true;
		if (thr->state == Sleeping)
			sleeping = true;
		spinlock_unlock(&thr->lock);
		
		if (sleeping)
			waitq_interrupt_sleep(thr);
	}
	spinlock_unlock(&ta->lock);
}

/** Kill task.
 *
 * This function is idempotent.
 * It signals all the task's threads to bail it out.
 *
 * @param id		ID of the task to be killed.
 *
 * @return		Zero on success or an error code from errno.h.
 */
int task_kill(task_id_t id)
{
	ipl_t ipl;
	task_t *ta;

	if (id == 1)
		return EPERM;
	
	ipl = interrupts_disable();
	spinlock_lock(&tasks_lock);
	if (!(ta = task_find_by_id(id))) {
		spinlock_unlock(&tasks_lock);
		interrupts_restore(ipl);
		return ENOENT;
	}
	task_kill_internal(ta);
	spinlock_unlock(&tasks_lock);
	interrupts_restore(ipl);
	return 0;
}

static bool task_print_walker(avltree_node_t *node, void *arg)
{
	task_t *t = avltree_get_instance(node, task_t, tasks_tree_node);
	int j;
		
	spinlock_lock(&t->lock);
			
	uint64_t cycles;
	char suffix;
	order(task_get_accounting(t), &cycles, &suffix);

#ifdef __32_BITS__	
	printf("%-6" PRIu64 " %-12s %-3" PRIu32 " %10p %10p %9" PRIu64
	    "%c %7ld %6ld", t->taskid, t->name, t->context, t, t->as, cycles,
	    suffix, atomic_get(&t->refcount), atomic_get(&t->active_calls));
#endif

#ifdef __64_BITS__
	printf("%-6" PRIu64 " %-12s %-3" PRIu32 " %18p %18p %9" PRIu64
	    "%c %7ld %6ld", t->taskid, t->name, t->context, t, t->as, cycles,
	    suffix, atomic_get(&t->refcount), atomic_get(&t->active_calls));
#endif

	for (j = 0; j < IPC_MAX_PHONES; j++) {
		if (t->phones[j].callee)
			printf(" %d:%p", j, t->phones[j].callee);
	}
	printf("\n");
			
	spinlock_unlock(&t->lock);
	return true;
}

/** Print task list */
void task_print_list(void)
{
	ipl_t ipl;
	
	/* Messing with task structures, avoid deadlock */
	ipl = interrupts_disable();
	spinlock_lock(&tasks_lock);

#ifdef __32_BITS__	
	printf("taskid name         ctx address    as         "
	    "cycles     threads calls  callee\n");
	printf("------ ------------ --- ---------- ---------- "
	    "---------- ------- ------ ------>\n");
#endif

#ifdef __64_BITS__
	printf("taskid name         ctx address            as                 "
	    "cycles     threads calls  callee\n");
	printf("------ ------------ --- ------------------ ------------------ "
	    "---------- ------- ------ ------>\n");
#endif

	avltree_walk(&tasks_tree, task_print_walker, NULL);

	spinlock_unlock(&tasks_lock);
	interrupts_restore(ipl);
}

/** @}
 */
