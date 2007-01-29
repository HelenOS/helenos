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
#include <synch/spinlock.h>
#include <synch/mutex.h>
#include <synch/rwlock.h>
#include <synch/futex.h>
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

#define IPC_MAX_PHONES  16

struct answerbox;
struct task;
struct thread;

typedef enum {
	IPC_PHONE_FREE = 0,     /**< Phone is free and can be allocated */
	IPC_PHONE_CONNECTING,   /**< Phone is connecting somewhere */
	IPC_PHONE_CONNECTED,    /**< Phone is connected */
	IPC_PHONE_HUNGUP,  	/**< Phone is hung up, waiting for answers to come */
	IPC_PHONE_SLAMMED       /**< Phone was hungup from server */
} ipc_phone_state_t;

/** Structure identifying phone (in TASK structure) */
typedef struct {
	SPINLOCK_DECLARE(lock);
	link_t link;
	struct answerbox *callee;
	ipc_phone_state_t state;
	atomic_t active_calls;
} phone_t;

typedef struct answerbox {
	SPINLOCK_DECLARE(lock);

	struct task *task;

	waitq_t wq;

	link_t connected_phones;	/**< Phones connected to this answerbox */
	link_t calls;			/**< Received calls */
	link_t dispatched_calls;	/* Should be hash table in the future */

	link_t answers;			/**< Answered calls */

	SPINLOCK_DECLARE(irq_lock);
	link_t irq_notifs;       	/**< Notifications from IRQ handlers */
	link_t irq_head;		/**< IRQs with notifications to this answerbox. */
} answerbox_t;

/** Task structure. */
typedef struct task {
	/** Task lock.
	 *
	 * Must be acquired before threads_lock and thread lock of any of its threads.
	 */
	SPINLOCK_DECLARE(lock);
	
	char *name;
	struct thread *main_thread;	/**< Pointer to the main thread. */
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
	
	uint64_t cycles;	/**< Accumulated accounting. */
} task_t;

typedef void (* timeout_handler_t)(void *arg);

typedef struct {
	SPINLOCK_DECLARE(lock);

	link_t link;			/**< Link to the list of active timeouts on THE->cpu */
	
	uint64_t ticks;			/**< Timeout will be activated in this amount of clock() ticks. */

	timeout_handler_t handler;	/**< Function that will be called on timeout activation. */
	void *arg;			/**< Argument to be passed to handler() function. */
	
	cpu_t *cpu;			/**< On which processor is this timeout registered. */
} timeout_t;

/** Thread states. */
typedef enum {
	Invalid,	/**< It is an error, if thread is found in this state. */
	Running,	/**< State of a thread that is currently executing on some CPU. */
	Sleeping,	/**< Thread in this state is waiting for an event. */
	Ready,		/**< State of threads in a run queue. */
	Entering,	/**< Threads are in this state before they are first readied. */
	Exiting,	/**< After a thread calls thread_exit(), it is put into Exiting state. */
	Undead		/**< Threads that were not detached but exited are in the Undead state. */
} state_t;

/** Join types. */
typedef enum {
	None,
	TaskClnp,	/**< The thread will be joined by ktaskclnp thread. */
	TaskGC		/**< The thread will be joined by ktaskgc thread. */
} thread_join_type_t;

SPINLOCK_EXTERN(tasks_lock);
extern btree_t tasks_btree;

extern void task_init(void);
extern task_t *task_create(as_t *as, char *name);
extern void task_destroy(task_t *t);
extern task_t *task_run_program(void *program_addr, char *name);
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
