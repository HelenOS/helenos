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
/** @file
 */

#ifndef KERN_TASK_H_
#define KERN_TASK_H_

#include <cpu.h>
#include <ipc/ipc.h>
#include <ipc/event.h>
#include <ipc/kbox.h>
#include <synch/spinlock.h>
#include <synch/mutex.h>
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
#include <mm/as.h>
#include <abi/sysinfo.h>

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
	IRQ_SPINLOCK_DECLARE(lock);
	
	char name[TASK_NAME_BUFLEN];
	/** List of threads contained in this task. */
	list_t threads;
	/** Address space. */
	as_t *as;
	/** Unique identity of task. */
	task_id_t taskid;
	/** Task security container. */
	container_id_t container;
	
	/** Number of references (i.e. threads). */
	atomic_t refcount;
	/** Number of threads that haven't exited yet. */
	atomic_t lifecount;
	
	/** Task capabilities. */
	cap_t capabilities;
	
	/* IPC stuff */

	/** Receiving communication endpoint */
	answerbox_t answerbox;

	/** Sending communication endpoints */
	phone_t phones[IPC_MAX_PHONES];

	/** Spinlock protecting the active_calls list. */
	SPINLOCK_DECLARE(active_calls_lock);

	/**
	 * List of all calls sent by this task that have not yet been
	 * answered.
	 */
	list_t active_calls;

	event_t events[EVENT_TASK_END - EVENT_END];

	/** IPC statistics */
	stats_ipc_t ipc_info;
	
#ifdef CONFIG_UDEBUG
	/** Debugging stuff. */
	udebug_task_t udebug;
	
	/** Kernel answerbox. */
	kbox_t kb;
#endif /* CONFIG_UDEBUG */
	
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
	uint64_t ucycles;
	uint64_t kcycles;
} task_t;

IRQ_SPINLOCK_EXTERN(tasks_lock);
extern avltree_t tasks_tree;

extern void task_init(void);
extern void task_done(void);
extern task_t *task_create(as_t *, const char *);
extern void task_destroy(task_t *);
extern void task_hold(task_t *);
extern void task_release(task_t *);
extern task_t *task_find_by_id(task_id_t);
extern int task_kill(task_id_t);
extern void task_kill_self(bool) __attribute__((noreturn));
extern void task_get_accounting(task_t *, uint64_t *, uint64_t *);
extern void task_print_list(bool);

extern void cap_set(task_t *, cap_t);
extern cap_t cap_get(task_t *);

#ifndef task_create_arch
extern void task_create_arch(task_t *);
#endif

#ifndef task_destroy_arch
extern void task_destroy_arch(task_t *);
#endif

#ifdef __32_BITS__
extern sysarg_t sys_task_get_id(sysarg64_t *);
#endif

#ifdef __64_BITS__
extern sysarg_t sys_task_get_id(void);
#endif

extern sysarg_t sys_task_set_name(const char *, size_t);
extern sysarg_t sys_task_kill(task_id_t *);
extern sysarg_t sys_task_exit(sysarg_t);

#endif

/** @}
 */
