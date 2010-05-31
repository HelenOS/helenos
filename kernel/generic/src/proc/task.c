/*
 * Copyright (c) 2010 Jakub Jermar
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
 * @brief Task management.
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
#include <str.h>
#include <memstr.h>
#include <syscall/copy.h>
#include <macros.h>
#include <ipc/event.h>

/** Spinlock protecting the tasks_tree AVL tree. */
IRQ_SPINLOCK_INITIALIZE(tasks_lock);

/** AVL tree of active tasks.
 *
 * The task is guaranteed to exist after it was found in the tasks_tree as
 * long as:
 *
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
static int tsk_constructor(void *, unsigned int);

/** Initialize kernel tasks support.
 *
 */
void task_init(void)
{
	TASK = NULL;
	avltree_create(&tasks_tree);
	task_slab = slab_cache_create("task_slab", sizeof(task_t), 0,
	    tsk_constructor, NULL, 0);
}

/** Task finish walker.
 *
 * The idea behind this walker is to kill and count all tasks different from
 * TASK.
 *
 */
static bool task_done_walker(avltree_node_t *node, void *arg)
{
	task_t *task = avltree_get_instance(node, task_t, tasks_tree_node);
	size_t *cnt = (size_t *) arg;
	
	if (task != TASK) {
		(*cnt)++;
		
#ifdef CONFIG_DEBUG
		printf("[%"PRIu64"] ", task->taskid);
#endif
		
		task_kill_internal(task);
	}
	
	/* Continue the walk */
	return true;
}

/** Kill all tasks except the current task.
 *
 */
void task_done(void)
{
	size_t tasks_left;
	
	/* Repeat until there are any tasks except TASK */
	do {
#ifdef CONFIG_DEBUG
		printf("Killing tasks... ");
#endif
		
		irq_spinlock_lock(&tasks_lock, true);
		tasks_left = 0;
		avltree_walk(&tasks_tree, task_done_walker, &tasks_left);
		irq_spinlock_unlock(&tasks_lock, true);
		
		thread_sleep(1);
		
#ifdef CONFIG_DEBUG
		printf("\n");
#endif
	} while (tasks_left > 0);
}

int tsk_constructor(void *obj, unsigned int kmflags)
{
	task_t *task = (task_t *) obj;
	
	atomic_set(&task->refcount, 0);
	atomic_set(&task->lifecount, 0);
	atomic_set(&task->active_calls, 0);
	
	irq_spinlock_initialize(&task->lock, "task_t_lock");
	mutex_initialize(&task->futexes_lock, MUTEX_PASSIVE);
	
	list_initialize(&task->th_head);
	list_initialize(&task->sync_box_head);
	
	ipc_answerbox_init(&task->answerbox, task);
	
	size_t i;
	for (i = 0; i < IPC_MAX_PHONES; i++)
		ipc_phone_init(&task->phones[i]);
	
#ifdef CONFIG_UDEBUG
	/* Init kbox stuff */
	task->kb.thread = NULL;
	ipc_answerbox_init(&task->kb.box, task);
	mutex_initialize(&task->kb.cleanup_lock, MUTEX_PASSIVE);
#endif
	
	return 0;
}

/** Create new task with no threads.
 *
 * @param as   Task's address space.
 * @param name Symbolic name (a copy is made).
 *
 * @return New task's structure.
 *
 */
task_t *task_create(as_t *as, const char *name)
{
	task_t *task = (task_t *) slab_alloc(task_slab, 0);
	task_create_arch(task);
	
	task->as = as;
	str_cpy(task->name, TASK_NAME_BUFLEN, name);
	
	task->context = CONTEXT;
	task->capabilities = 0;
	task->ucycles = 0;
	task->kcycles = 0;
	
	task->ipc_info.call_sent = 0;
	task->ipc_info.call_recieved = 0;
	task->ipc_info.answer_sent = 0;
	task->ipc_info.answer_recieved = 0;
	task->ipc_info.irq_notif_recieved = 0;
	task->ipc_info.forwarded = 0;
	
#ifdef CONFIG_UDEBUG
	/* Init debugging stuff */
	udebug_task_init(&task->udebug);
	
	/* Init kbox stuff */
	task->kb.finished = false;
#endif
	
	if ((ipc_phone_0) &&
	    (context_check(ipc_phone_0->task->context, task->context)))
		ipc_phone_connect(&task->phones[0], ipc_phone_0);
	
	btree_create(&task->futexes);
	
	/*
	 * Get a reference to the address space.
	 */
	as_hold(task->as);
	
	irq_spinlock_lock(&tasks_lock, true);
	
	task->taskid = ++task_counter;
	avltree_node_initialize(&task->tasks_tree_node);
	task->tasks_tree_node.key = task->taskid;
	avltree_insert(&tasks_tree, &task->tasks_tree_node);
	
	irq_spinlock_unlock(&tasks_lock, true);
	
	return task;
}

/** Destroy task.
 *
 * @param task Task to be destroyed.
 *
 */
void task_destroy(task_t *task)
{
	/*
	 * Remove the task from the task B+tree.
	 */
	irq_spinlock_lock(&tasks_lock, true);
	avltree_delete(&tasks_tree, &task->tasks_tree_node);
	irq_spinlock_unlock(&tasks_lock, true);
	
	/*
	 * Perform architecture specific task destruction.
	 */
	task_destroy_arch(task);
	
	/*
	 * Free up dynamically allocated state.
	 */
	btree_destroy(&task->futexes);
	
	/*
	 * Drop our reference to the address space.
	 */
	as_release(task->as);
	
	slab_free(task_slab, task);
}

/** Hold a reference to a task.
 *
 * Holding a reference to a task prevents destruction of that task.
 *
 * @param task Task to be held.
 *
 */
void task_hold(task_t *task)
{
	atomic_inc(&task->refcount);
}

/** Release a reference to a task.
 *
 * The last one to release a reference to a task destroys the task.
 *
 * @param task Task to be released.
 *
 */
void task_release(task_t *task)
{
	if ((atomic_predec(&task->refcount)) == 0)
		task_destroy(task);
}

/** Syscall for reading task ID from userspace.
 *
 * @param uspace_task_id Userspace address of 8-byte buffer
 *                       where to store current task ID.
 *
 * @return Zero on success or an error code from @ref errno.h.
 *
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
 * @param name The new name for the task. (typically the same
 *             as the command used to execute it).
 *
 * @return 0 on success or an error code from @ref errno.h.
 *
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
 * @param id Task ID.
 *
 * @return Task structure address or NULL if there is no such task ID.
 *
 */
task_t *task_find_by_id(task_id_t id)
{
	ASSERT(interrupts_disabled());
	ASSERT(irq_spinlock_locked(&tasks_lock));

	avltree_node_t *node =
	    avltree_search(&tasks_tree, (avltree_key_t) id);
	
	if (node)
		return avltree_get_instance(node, task_t, tasks_tree_node);
	
	return NULL;
}

/** Get accounting data of given task.
 *
 * Note that task lock of 'task' must be already held and interrupts must be
 * already disabled.
 *
 * @param task    Pointer to the task.
 * @param ucycles Out pointer to sum of all user cycles.
 * @param kcycles Out pointer to sum of all kernel cycles.
 *
 */
void task_get_accounting(task_t *task, uint64_t *ucycles, uint64_t *kcycles)
{
	ASSERT(interrupts_disabled());
	ASSERT(irq_spinlock_locked(&task->lock));

	/* Accumulated values of task */
	uint64_t uret = task->ucycles;
	uint64_t kret = task->kcycles;
	
	/* Current values of threads */
	link_t *cur;
	for (cur = task->th_head.next; cur != &task->th_head; cur = cur->next) {
		thread_t *thread = list_get_instance(cur, thread_t, th_link);
		
		irq_spinlock_lock(&thread->lock, false);
		
		/* Process only counted threads */
		if (!thread->uncounted) {
			if (thread == THREAD) {
				/* Update accounting of current thread */
				thread_update_accounting(false);
			}
			
			uret += thread->ucycles;
			kret += thread->kcycles;
		}
		
		irq_spinlock_unlock(&thread->lock, false);
	}
	
	*ucycles = uret;
	*kcycles = kret;
}

static void task_kill_internal(task_t *task)
{
	link_t *cur;
	
	/*
	 * Interrupt all threads.
	 */
	irq_spinlock_lock(&task->lock, false);
	for (cur = task->th_head.next; cur != &task->th_head; cur = cur->next) {
		thread_t *thread = list_get_instance(cur, thread_t, th_link);
		bool sleeping = false;
		
		irq_spinlock_lock(&thread->lock, false);
		
		thread->interrupted = true;
		if (thread->state == Sleeping)
			sleeping = true;
		
		irq_spinlock_unlock(&thread->lock, false);
		
		if (sleeping)
			waitq_interrupt_sleep(thread);
	}
	
	irq_spinlock_unlock(&task->lock, false);
}

/** Kill task.
 *
 * This function is idempotent.
 * It signals all the task's threads to bail it out.
 *
 * @param id ID of the task to be killed.
 *
 * @return Zero on success or an error code from errno.h.
 *
 */
int task_kill(task_id_t id)
{
	if (id == 1)
		return EPERM;
	
	irq_spinlock_lock(&tasks_lock, true);
	
	task_t *task = task_find_by_id(id);
	if (!task) {
		irq_spinlock_unlock(&tasks_lock, true);
		return ENOENT;
	}
	
	task_kill_internal(task);
	irq_spinlock_unlock(&tasks_lock, true);
	
	return EOK;
}

static bool task_print_walker(avltree_node_t *node, void *arg)
{
	task_t *task = avltree_get_instance(node, task_t, tasks_tree_node);
	irq_spinlock_lock(&task->lock, false);
	
	uint64_t ucycles;
	uint64_t kcycles;
	char usuffix, ksuffix;
	task_get_accounting(task, &ucycles, &kcycles);
	order_suffix(ucycles, &ucycles, &usuffix);
	order_suffix(kcycles, &kcycles, &ksuffix);
	
#ifdef __32_BITS__
	printf("%-6" PRIu64 " %-12s %-3" PRIu32 " %10p %10p %9" PRIu64 "%c %9"
	    PRIu64 "%c %7ld %6ld", task->taskid, task->name, task->context,
	    task, task->as, ucycles, usuffix, kcycles, ksuffix,
	    atomic_get(&task->refcount), atomic_get(&task->active_calls));
#endif
	
#ifdef __64_BITS__
	printf("%-6" PRIu64 " %-12s %-3" PRIu32 " %18p %18p %9" PRIu64 "%c %9"
	    PRIu64 "%c %7ld %6ld", task->taskid, task->name, task->context,
	    task, task->as, ucycles, usuffix, kcycles, ksuffix,
	    atomic_get(&task->refcount), atomic_get(&task->active_calls));
#endif
	
	size_t i;
	for (i = 0; i < IPC_MAX_PHONES; i++) {
		if (task->phones[i].callee)
			printf(" %" PRIs ":%p", i, task->phones[i].callee);
	}
	printf("\n");
	
	irq_spinlock_unlock(&task->lock, false);
	return true;
}

/** Print task list */
void task_print_list(void)
{
	/* Messing with task structures, avoid deadlock */
	irq_spinlock_lock(&tasks_lock, true);
	
#ifdef __32_BITS__
	printf("taskid name         ctx address    as        "
	    " ucycles    kcycles    threads calls  callee\n");
	printf("------ ------------ --- ---------- ----------"
	    " ---------- ---------- ------- ------ ------>\n");
#endif
	
#ifdef __64_BITS__
	printf("taskid name         ctx address            as                "
	    " ucycles    kcycles    threads calls  callee\n");
	printf("------ ------------ --- ------------------ ------------------"
	    " ---------- ---------- ---------- ------- ------ ------>\n");
#endif
	
	avltree_walk(&tasks_tree, task_print_walker, NULL);
	
	irq_spinlock_unlock(&tasks_lock, true);
}

/** @}
 */
