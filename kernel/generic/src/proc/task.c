/*
 * Copyright (c) 2010 Jakub Jermar
 * Copyright (c) 2018 Jiri Svoboda
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

/** @addtogroup kernel_generic_proc
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
#include <synch/spinlock.h>
#include <synch/waitq.h>
#include <arch.h>
#include <barrier.h>
#include <adt/list.h>
#include <adt/odict.h>
#include <cap/cap.h>
#include <ipc/ipc.h>
#include <ipc/ipcrsc.h>
#include <ipc/event.h>
#include <stdio.h>
#include <errno.h>
#include <halt.h>
#include <str.h>
#include <syscall/copy.h>
#include <macros.h>

/** Spinlock protecting the @c tasks ordered dictionary. */
IRQ_SPINLOCK_INITIALIZE(tasks_lock);

/** Ordered dictionary of active tasks by task ID.
 *
 * Members are task_t structures.
 *
 * The task is guaranteed to exist after it was found in the @c tasks
 * dictionary as long as:
 *
 * @li the tasks_lock is held,
 * @li the task's lock is held when task's lock is acquired before releasing
 *     tasks_lock or
 * @li the task's refcount is greater than 0
 *
 */
odict_t tasks;

static task_id_t task_counter = 0;

static slab_cache_t *task_cache;

/* Forward declarations. */
static void task_kill_internal(task_t *);
static errno_t tsk_constructor(void *, unsigned int);
static size_t tsk_destructor(void *);

static void *tasks_getkey(odlink_t *);
static int tasks_cmp(void *, void *);

/** Initialize kernel tasks support.
 *
 */
void task_init(void)
{
	TASK = NULL;
	odict_initialize(&tasks, tasks_getkey, tasks_cmp);
	task_cache = slab_cache_create("task_t", sizeof(task_t), 0,
	    tsk_constructor, tsk_destructor, 0);
}

/** Kill all tasks except the current task.
 *
 */
void task_done(void)
{
	size_t tasks_left;
	task_t *task;

	if (ipc_box_0) {
		task_t *task_0 = ipc_box_0->task;
		ipc_box_0 = NULL;
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

		task = task_first();
		while (task != NULL) {
			if (task != TASK) {
				tasks_left++;
#ifdef CONFIG_DEBUG
				printf("[%" PRIu64 "] ", task->taskid);
#endif
				task_kill_internal(task);
			}

			task = task_next(task);
		}

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

	atomic_store(&task->lifecount, 0);

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
	task_t *task = (task_t *) slab_alloc(task_cache, FRAME_ATOMIC);
	if (!task)
		return NULL;

	refcount_init(&task->refcount);

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

	task->debug_sections = NULL;

#ifdef CONFIG_UDEBUG
	/* Init debugging stuff */
	udebug_task_init(&task->udebug);

	/* Init kbox stuff */
	task->kb.box.active = true;
	task->kb.finished = false;
#endif

	if ((ipc_box_0) &&
	    (container_check(ipc_box_0->task->container, task->container))) {
		cap_phone_handle_t phone_handle;
		errno_t rc = phone_alloc(task, true, &phone_handle, NULL);
		if (rc != EOK) {
			task->as = NULL;
			task_destroy_arch(task);
			slab_free(task_cache, task);
			return NULL;
		}

		kobject_t *phone_obj = kobject_get(task, phone_handle,
		    KOBJECT_TYPE_PHONE);
		(void) ipc_phone_connect(phone_obj->phone, ipc_box_0);
	}

	irq_spinlock_lock(&tasks_lock, true);

	task->taskid = ++task_counter;
	odlink_initialize(&task->ltasks);
	odict_insert(&task->ltasks, &tasks, NULL);

	irq_spinlock_unlock(&tasks_lock, true);

	return task;
}

/** Destroy task.
 *
 * @param task Task to be destroyed.
 *
 */
static void task_destroy(task_t *task)
{
	/*
	 * Remove the task from the task odict.
	 */
	irq_spinlock_lock(&tasks_lock, true);
	odict_remove(&task->ltasks);
	irq_spinlock_unlock(&tasks_lock, true);

	/*
	 * Perform architecture specific task destruction.
	 */
	task_destroy_arch(task);

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
	refcount_up(&task->refcount);
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
	if (refcount_down(&task->refcount))
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
sys_errno_t sys_task_get_id(uspace_ptr_sysarg64_t uspace_taskid)
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
sys_errno_t sys_task_set_name(const uspace_ptr_char uspace_name, size_t name_len)
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

	/* Set task name */
	str_cpy(TASK->name, TASK_NAME_BUFLEN, namebuf);

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
sys_errno_t sys_task_kill(uspace_ptr_task_id_t uspace_taskid)
{
	task_id_t taskid;
	errno_t rc = copy_from_uspace(&taskid, uspace_taskid, sizeof(taskid));
	if (rc != EOK)
		return (sys_errno_t) rc;

	return (sys_errno_t) task_kill(taskid);
}

/** Find task structure corresponding to task ID.
 *
 * @param id Task ID.
 *
 * @return Task reference or NULL if there is no such task ID.
 *
 */
task_t *task_find_by_id(task_id_t id)
{
	task_t *task = NULL;

	irq_spinlock_lock(&tasks_lock, true);

	odlink_t *odlink = odict_find_eq(&tasks, &id, NULL);
	if (odlink != NULL) {
		task = odict_get_instance(odlink, task_t, ltasks);

		/*
		 * The directory of tasks can't hold a reference, since that would
		 * prevent task from ever being destroyed. That means we have to
		 * check for the case where the task is already being destroyed, but
		 * not yet removed from the directory.
		 */
		if (!refcount_try_up(&task->refcount))
			task = NULL;
	}

	irq_spinlock_unlock(&tasks_lock, true);

	return task;
}

/** Get count of tasks.
 *
 * @return Number of tasks in the system
 */
size_t task_count(void)
{
	assert(interrupts_disabled());
	assert(irq_spinlock_locked(&tasks_lock));

	return odict_count(&tasks);
}

/** Get first task (task with lowest ID).
 *
 * @return Pointer to first task or @c NULL if there are none.
 */
task_t *task_first(void)
{
	odlink_t *odlink;

	assert(interrupts_disabled());
	assert(irq_spinlock_locked(&tasks_lock));

	odlink = odict_first(&tasks);
	if (odlink == NULL)
		return NULL;

	return odict_get_instance(odlink, task_t, ltasks);
}

/** Get next task (with higher task ID).
 *
 * @param cur Current task
 * @return Pointer to next task or @c NULL if there are no more tasks.
 */
task_t *task_next(task_t *cur)
{
	odlink_t *odlink;

	assert(interrupts_disabled());
	assert(irq_spinlock_locked(&tasks_lock));

	odlink = odict_next(&cur->ltasks, &tasks);
	if (odlink == NULL)
		return NULL;

	return odict_get_instance(odlink, task_t, ltasks);
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
		/* Process only counted threads */
		if (!thread->uncounted) {
			if (thread == THREAD) {
				/* Update accounting of current thread */
				thread_update_accounting(false);
			}

			uret += atomic_time_read(&thread->ucycles);
			kret += atomic_time_read(&thread->kcycles);
		}
	}

	*ucycles = uret;
	*kcycles = kret;
}

static void task_kill_internal(task_t *task)
{
	irq_spinlock_lock(&task->lock, true);

	/*
	 * Interrupt all threads.
	 */

	list_foreach(task->threads, th_link, thread_t, thread) {
		thread_interrupt(thread);
	}

	irq_spinlock_unlock(&task->lock, true);
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

	task_t *task = task_find_by_id(id);
	if (!task)
		return ENOENT;

	task_kill_internal(task);
	task_release(task);
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

	task_kill_internal(TASK);
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
	unreachable();
}

static void task_print(task_t *task, bool additional)
{
	irq_spinlock_lock(&task->lock, false);

	uint64_t ucycles;
	uint64_t kcycles;
	char usuffix, ksuffix;
	task_get_accounting(task, &ucycles, &kcycles);
	order_suffix(ucycles, &ucycles, &usuffix);
	order_suffix(kcycles, &kcycles, &ksuffix);

#ifdef __32_BITS__
	if (additional)
		printf("%-8" PRIu64 " %9zu", task->taskid,
		    atomic_load(&task->lifecount));
	else
		printf("%-8" PRIu64 " %-14s %-5" PRIu32 " %10p %10p"
		    " %9" PRIu64 "%c %9" PRIu64 "%c\n", task->taskid,
		    task->name, task->container, task, task->as,
		    ucycles, usuffix, kcycles, ksuffix);
#endif

#ifdef __64_BITS__
	if (additional)
		printf("%-8" PRIu64 " %9" PRIu64 "%c %9" PRIu64 "%c "
		    "%9zu\n", task->taskid, ucycles, usuffix, kcycles,
		    ksuffix, atomic_load(&task->lifecount));
	else
		printf("%-8" PRIu64 " %-14s %-5" PRIu32 " %18p %18p\n",
		    task->taskid, task->name, task->container, task, task->as);
#endif

	irq_spinlock_unlock(&task->lock, false);
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

	task_t *task;

	task = task_first();
	while (task != NULL) {
		task_print(task, additional);
		task = task_next(task);
	}

	irq_spinlock_unlock(&tasks_lock, true);
}

/** Get key function for the @c tasks ordered dictionary.
 *
 * @param odlink Link
 * @return Pointer to task ID cast as 'void *'
 */
static void *tasks_getkey(odlink_t *odlink)
{
	task_t *task = odict_get_instance(odlink, task_t, ltasks);
	return (void *) &task->taskid;
}

/** Key comparison function for the @c tasks ordered dictionary.
 *
 * @param a Pointer to thread A ID
 * @param b Pointer to thread B ID
 * @return -1, 0, 1 iff ID A is less than, equal to, greater than B
 */
static int tasks_cmp(void *a, void *b)
{
	task_id_t ida = *(task_id_t *)a;
	task_id_t idb = *(task_id_t *)b;

	if (ida < idb)
		return -1;
	else if (ida == idb)
		return 0;
	else
		return +1;
}

/** @}
 */
