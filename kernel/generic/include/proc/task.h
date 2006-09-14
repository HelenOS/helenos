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

/** @addtogroup genericproc
 * @{
 */
/** @file
 */

#ifndef KERN_TASK_H_
#define KERN_TASK_H_

#include <typedefs.h>
#include <synch/spinlock.h>
#include <synch/mutex.h>
#include <synch/futex.h>
#include <adt/btree.h>
#include <adt/list.h>
#include <ipc/ipc.h>
#include <security/cap.h>
#include <arch/proc/task.h>

/** Task structure. */
struct task {
	/** Task lock.
	 *
	 * Must be acquired before threads_lock and thread lock of any of its threads.
	 */
	SPINLOCK_DECLARE(lock);
	
	char *name;
	thread_t *main_thread;	/**< Pointer to the main thread. */
	link_t th_head;		/**< List of threads contained in this task. */
	as_t *as;		/**< Address space. */
	task_id_t taskid;	/**< Unique identity of task */
	context_id_t context;	/**< Task security context */

	/** If this is true, new threads can become part of the task. */
	bool accept_new_threads;

	count_t refcount;	/**< Number of references (i.e. threads). */

	cap_t capabilities;	/**< Task capabilities. */

	/* IPC stuff */
	answerbox_t answerbox;  /**< Communication endpoint */
	phone_t phones[IPC_MAX_PHONES];
	atomic_t active_calls;  /**< Active asynchronous messages.
				 *   It is used for limiting uspace to
				 *   certain extent. */
	
	task_arch_t arch;	/**< Architecture specific task data. */
	
	/**
	 * Serializes access to the B+tree of task's futexes. This mutex is
	 * independent on the task spinlock.
	 */
	mutex_t futexes_lock;
	btree_t futexes;	/**< B+tree of futexes referenced by this task. */
};

extern spinlock_t tasks_lock;
extern btree_t tasks_btree;

extern void task_init(void);
extern task_t *task_create(as_t *as, char *name);
extern void task_destroy(task_t *t);
extern task_t *task_run_program(void *program_addr, char *name);
extern task_t *task_find_by_id(task_id_t id);
extern int task_kill(task_id_t id);


#ifndef task_create_arch
extern void task_create_arch(task_t *t);
#endif

#ifndef task_destroy_arch
extern void task_destroy_arch(task_t *t);
#endif

extern unative_t sys_task_get_id(task_id_t *uspace_task_id);

#endif

/** @}
 */
