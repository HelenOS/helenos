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

#include <assert.h>
#include <proc/thread.h>
#include <proc/task.h>
#include <mm/as.h>
#include <mm/slab.h>
#include <atomic.h>
#include <synch/futex.h>
#include <synch/spinlock.h>
#include <synch/waitq.h>
#include <arch.h>
#include <arch/barrier.h>
#include <adt/avl.h>
#include <adt/btree.h>
#include <adt/list.h>
#include <cap/cap.h>
#include <ipc/ipc.h>
#include <ipc/ipcrsc.h>
#include <ipc/event.h>
#include <print.h>
#include <errno.h>
#include <halt.h>
#include <str.h>
#include <syscall/copy.h>
#include <macros.h>

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

static slab_cache_t *task_cache;

/* Forward declarations. */
static void task_kill_internal(task_t *);
static errno_t tsk_constructor(void *, unsigned int);
static size_t tsk_destructor(void *obj);

/** Initialize kernel tasks support.
 *
 */
void task_init(void)
{
	TASK = NULL;
	avltree_create(&tasks_tree);
	task_cache = slab_cache_create("task_t", sizeof(task_t), 0,
	    tsk_constructor, tsk_destructor, 0);
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

	if (ipc_phone_0) {
		task_t *task_0 = ipc_phone_0->task;
		ipc_phone_0 = NULL;
		/*
		 * The first task is held by kinit(), we need to release it or
		 * it will never finish cleanup.
		 */
		task_release(task_0);
	}
	
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

errno_t tsk_constructor(void *obj, unsigned int kmflags)
{
	task_t *task = (task_t *) obj;

	errno_t rc = caps_task_alloc(task);
	if (rc != EOK)
		return rc;
	
	atomic_set(&task->refcount, 0);
	atomic_set(&task->lifecount, 0);
	
	irq_spinlock_initialize(&task->lock, "task_t_lock");
	
	list_initialize(&task->threads);
	
	ipc_answerbox_init(&task->answerbox, task);
	
	spinlock_initialize(&task->active_calls_lock, "active_calls_lock");
	list_initialize(&task->active_calls);
		
#ifdef CONFIG_UDEBUG
	/* Init kbox stuff */
	task->kb.thread = NULL;
	ipc_answerbox_init(&task->kb.box, task);
	mutex_initialize(&task->kb.cleanup_lock, MUTEX_PASSIVE);
#endif
	
	return EOK;
}

size_t tsk_destructor(void *obj)
{
	task_t *task = (task_t *) obj;
	
	caps_task_free(task);
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
	task_t *task = (task_t *) slab_alloc(task_cache, 0);
	if (task == NULL) {
		return NULL;
	}
	
	task_create_arch(task);
	
	task->as = as;
	str_cpy(task->name, TASK_NAME_BUFLEN, name);
	
	task->container = CONTAINER;
	task->perms = 0;
	task->ucycles = 0;
	task->kcycles = 0;

	caps_task_init(task);

	task->ipc_info.call_sent = 0;
	task->ipc_info.call_received = 0;
	task->ipc_info.answer_sent = 0;
	task->ipc_info.answer_received = 0;
	task->ipc_info.irq_notif_received = 0;
	task->ipc_info.forwarded = 0;

	event_task_init(task);
	
	task->answerbox.active = true;

#ifdef CONFIG_UDEBUG
	/* Init debugging stuff */
	udebug_task_init(&task->udebug);
	
	/* Init kbox stuff */
	task->kb.box.active = true;
	task->kb.finished = false;
#endif
	
	if ((ipc_phone_0) &&
	    (container_check(ipc_phone_0->task->container, task->container))) {
		cap_handle_t phone_handle;
		errno_t rc = phone_alloc(task, &phone_handle);
		if (rc != EOK) {
			task->as = NULL;
			task_destroy_arch(task);
			slab_free(task_cache, task);
			return NULL;
		}
		
		kobject_t *phone_obj = kobject_get(task, phone_handle,
		    KOBJECT_TYPE_PHONE);
		(void) ipc_phone_connect(phone_obj->phone, ipc_phone_0);
	}
	
	futex_task_init(task);
	
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
	futex_task_deinit(task);
	
	/*
	 * Drop our reference to the address space.
	 */
	as_release(task->as);
	
	slab_free(task_cache, task);
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

#ifdef __32_BITS__

/** Syscall for reading task ID from userspace (32 bits)
 *
 * @param uspace_taskid Pointer to user-space buffer
 *                      where to store current task ID.
 *
 * @return Zero on success or an error code from @ref errno.h.
 *
 */
sys_errno_t sys_task_get_id(sysarg64_t *uspace_taskid)
{
	/*
	 * No need to acquire lock on TASK because taskid remains constant for
	 * the lifespan of the task.
	 */
	return (sys_errno_t) copy_to_uspace(uspace_taskid, &TASK->taskid,
	    sizeof(TASK->taskid));
}

#endif  /* __32_BITS__ */

#ifdef __64_BITS__

/** Syscall for reading task ID from userspace (64 bits)
 *
 * @return Current task ID.
 *
 */
sysarg_t sys_task_get_id(void)
{
	/*
	 * No need to acquire lock on TASK because taskid remains constant for
	 * the lifespan of the task.
	 */
	return TASK->taskid;
}

#endif  /* __64_BITS__ */

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
sys_errno_t sys_task_set_name(const char *uspace_name, size_t name_len)
{
	char namebuf[TASK_NAME_BUFLEN];
	
	/* Cap length of name and copy it from userspace. */
	if (name_len > TASK_NAME_BUFLEN - 1)
		name_len = TASK_NAME_BUFLEN - 1;
	
	errno_t rc = copy_from_uspace(namebuf, uspace_name, name_len);
	if (rc != EOK)
		return (sys_errno_t) rc;
	
	namebuf[name_len] = '\0';
	
	/*
	 * As the task name is referenced also from the
	 * threads, lock the threads' lock for the course
	 * of the update.
	 */
	
	irq_spinlock_lock(&tasks_lock, true);
	irq_spinlock_lock(&TASK->lock, false);
	irq_spinlock_lock(&threads_lock, false);
	
	/* Set task name */
	str_cpy(TASK->name, TASK_NAME_BUFLEN, namebuf);
	
	irq_spinlock_unlock(&threads_lock, false);
	irq_spinlock_unlock(&TASK->lock, false);
	irq_spinlock_unlock(&tasks_lock, true);
	
	return EOK;
}

/** Syscall to forcefully terminate a task
 *
 * @param uspace_taskid Pointer to task ID in user space.
 *
 * @return 0 on success or an error code from @ref errno.h.
 *
 */
sys_errno_t sys_task_kill(task_id_t *uspace_taskid)
{
	task_id_t taskid;
	errno_t rc = copy_from_uspace(&taskid, uspace_taskid, sizeof(taskid));
	if (rc != EOK)
		return (sys_errno_t) rc;
	
	return (sys_errno_t) task_kill(taskid);
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
	assert(interrupts_disabled());
	assert(irq_spinlock_locked(&tasks_lock));

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
	assert(interrupts_disabled());
	assert(irq_spinlock_locked(&task->lock));

	/* Accumulated values of task */
	uint64_t uret = task->ucycles;
	uint64_t kret = task->kcycles;
	
	/* Current values of threads */
	list_foreach(task->threads, th_link, thread_t, thread) {
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
	irq_spinlock_lock(&task->lock, false);
	irq_spinlock_lock(&threads_lock, false);
	
	/*
	 * Interrupt all threads.
	 */
	
	list_foreach(task->threads, th_link, thread_t, thread) {
		bool sleeping = false;
		
		irq_spinlock_lock(&thread->lock, false);
		
		thread->interrupted = true;
		if (thread->state == Sleeping)
			sleeping = true;
		
		irq_spinlock_unlock(&thread->lock, false);
		
		if (sleeping)
			waitq_interrupt_sleep(thread);
	}
	
	irq_spinlock_unlock(&threads_lock, false);
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
errno_t task_kill(task_id_t id)
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

/** Kill the currently running task.
 *
 * @param notify Send out fault notifications.
 *
 * @return Zero on success or an error code from errno.h.
 *
 */
void task_kill_self(bool notify)
{
	/*
	 * User space can subscribe for FAULT events to take action
	 * whenever a task faults (to take a dump, run a debugger, etc.).
	 * The notification is always available, but unless udebug is enabled,
	 * that's all you get.
	*/
	if (notify) {
		/* Notify the subscriber that a fault occurred. */
		if (event_notify_3(EVENT_FAULT, false, LOWER32(TASK->taskid),
		    UPPER32(TASK->taskid), (sysarg_t) THREAD) == EOK) {
#ifdef CONFIG_UDEBUG
			/* Wait for a debugging session. */
			udebug_thread_fault();
#endif
		}
	}
	
	irq_spinlock_lock(&tasks_lock, true);
	task_kill_internal(TASK);
	irq_spinlock_unlock(&tasks_lock, true);
	
	thread_exit();
}

/** Process syscall to terminate the current task.
 *
 * @param notify Send out fault notifications.
 *
 */
sys_errno_t sys_task_exit(sysarg_t notify)
{
	task_kill_self(notify);
	
	/* Unreachable */
	return EOK;
}

static bool task_print_walker(avltree_node_t *node, void *arg)
{
	bool *additional = (bool *) arg;
	task_t *task = avltree_get_instance(node, task_t, tasks_tree_node);
	irq_spinlock_lock(&task->lock, false);
	
	uint64_t ucycles;
	uint64_t kcycles;
	char usuffix, ksuffix;
	task_get_accounting(task, &ucycles, &kcycles);
	order_suffix(ucycles, &ucycles, &usuffix);
	order_suffix(kcycles, &kcycles, &ksuffix);
	
#ifdef __32_BITS__
	if (*additional)
		printf("%-8" PRIu64 " %9" PRIua, task->taskid,
		    atomic_get(&task->refcount));
	else
		printf("%-8" PRIu64 " %-14s %-5" PRIu32 " %10p %10p"
		    " %9" PRIu64 "%c %9" PRIu64 "%c\n", task->taskid,
		    task->name, task->container, task, task->as,
		    ucycles, usuffix, kcycles, ksuffix);
#endif
	
#ifdef __64_BITS__
	if (*additional)
		printf("%-8" PRIu64 " %9" PRIu64 "%c %9" PRIu64 "%c "
		    "%9" PRIua "\n", task->taskid, ucycles, usuffix, kcycles,
		    ksuffix, atomic_get(&task->refcount));
	else
		printf("%-8" PRIu64 " %-14s %-5" PRIu32 " %18p %18p\n",
		    task->taskid, task->name, task->container, task, task->as);
#endif
	
	irq_spinlock_unlock(&task->lock, false);
	return true;
}

/** Print task list
 *
 * @param additional Print additional information.
 *
 */
void task_print_list(bool additional)
{
	/* Messing with task structures, avoid deadlock */
	irq_spinlock_lock(&tasks_lock, true);
	
#ifdef __32_BITS__
	if (additional)
		printf("[id    ] [threads] [calls] [callee\n");
	else
		printf("[id    ] [name        ] [ctn] [address ] [as      ]"
		    " [ucycles ] [kcycles ]\n");
#endif
	
#ifdef __64_BITS__
	if (additional)
		printf("[id    ] [ucycles ] [kcycles ] [threads] [calls]"
		    " [callee\n");
	else
		printf("[id    ] [name        ] [ctn] [address         ]"
		    " [as              ]\n");
#endif
	
	avltree_walk(&tasks_tree, task_print_walker, &additional);
	
	irq_spinlock_unlock(&tasks_lock, true);
}

/** @}
 */
