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
/** @file
 */

#ifndef KERN_TASK_H_
#define KERN_TASK_H_

#include <cpu.h>
#include <ipc/ipc.h>
#include <synch/spinlock.h>
#include <synch/mutex.h>
#include <synch/rwlock.h>
#include <synch/futex.h>
#include <adt/avl.h>
#include <adt/btree.h>
#include <adt/list.h>
#include <security/cap.h>
#include <arch/proc/task.h>
#include <arch/proc/thread.h>
#include <arch/context.h>
#include <arch/fpu_context.h>
#include <arch/cpu.h>
#include <mm/tlb.h>
#include <proc/scheduler.h>
#include <udebug/udebug.h>

struct thread;

/** Task structure. */
typedef struct task {
	/** Task's linkage for the tasks_tree AVL tree. */
	avltree_node_t tasks_tree_node;
	
	/** Task lock.
	 *
	 * Must be acquired before threads_lock and thread lock of any of its
	 * threads.
	 */
	SPINLOCK_DECLARE(lock);
	
	char *name;
	/** List of threads contained in this task. */
	link_t th_head;
	/** Address space. */
	as_t *as;
	/** Unique identity of task. */
	task_id_t taskid;
	/** Task security context. */
	context_id_t context;	

	/** Number of references (i.e. threads). */
	atomic_t refcount;
	/** Number of threads that haven't exited yet. */
	atomic_t lifecount;

	/** Task capabilities. */
	cap_t capabilities;	

	/* IPC stuff */
	answerbox_t answerbox;  /**< Communication endpoint */
	phone_t phones[IPC_MAX_PHONES];
	/**
	 * Active asynchronous messages. It is used for limiting uspace to
	 * certain extent.
	 */
	atomic_t active_calls;

#ifdef CONFIG_UDEBUG
	/** Debugging stuff */
	udebug_task_t udebug;

	/** Kernel answerbox */
	answerbox_t kernel_box;
	/** Thread used to service kernel answerbox */
	struct thread *kb_thread;
	/** Kbox thread creation vs. begin of cleanup mutual exclusion */
	mutex_t kb_cleanup_lock;
	/** True if cleanup of kbox has already started */
	bool kb_finished;
#endif
	
	/** Architecture specific task data. */
	task_arch_t arch;
	
	/**
	 * Serializes access to the B+tree of task's futexes. This mutex is
	 * independent on the task spinlock.
	 */
	mutex_t futexes_lock;
	/** B+tree of futexes referenced by this task. */
	btree_t futexes;	
	
	/** Accumulated accounting. */
	uint64_t cycles;
} task_t;

SPINLOCK_EXTERN(tasks_lock);
extern avltree_t tasks_tree;

extern void task_init(void);
extern void task_done(void);
extern task_t *task_create(as_t *as, char *name);
extern void task_destroy(task_t *t);
extern task_t *task_find_by_id(task_id_t id);
extern int task_kill(task_id_t id);
extern uint64_t task_get_accounting(task_t *t);

extern void cap_set(task_t *t, cap_t caps);
extern cap_t cap_get(task_t *t);

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
