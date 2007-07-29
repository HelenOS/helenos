/*
 * Copyright (c) 2001-2007 Jakub Jermar
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

#ifndef KERN_THREAD_H_
#define KERN_THREAD_H_

#include <synch/waitq.h>
#include <proc/task.h>
#include <time/timeout.h>
#include <cpu.h>
#include <synch/rwlock.h>
#include <synch/spinlock.h>
#include <adt/avl.h>
#include <mm/slab.h>
#include <arch/cpu.h>
#include <mm/tlb.h>
#include <proc/uarg.h>

#define THREAD_STACK_SIZE	STACK_SIZE
#define THREAD_NAME_BUFLEN	20

extern char *thread_states[];

/* Thread flags */

/** Thread cannot be migrated to another CPU.
 *
 * When using this flag, the caller must set cpu in the thread_t
 * structure manually before calling thread_ready (even on uniprocessor).
 */ 
#define THREAD_FLAG_WIRED	(1 << 0)
/** Thread was migrated to another CPU and has not run yet. */
#define THREAD_FLAG_STOLEN	(1 << 1)
/** Thread executes in userspace. */
#define THREAD_FLAG_USPACE	(1 << 2)
/** Thread will be attached by the caller. */
#define THREAD_FLAG_NOATTACH	(1 << 3)

/** Thread states. */
typedef enum {
	/** It is an error, if thread is found in this state. */
	Invalid,
	/** State of a thread that is currently executing on some CPU. */
	Running,
	/** Thread in this state is waiting for an event. */
	Sleeping,
	/** State of threads in a run queue. */
	Ready,
	/** Threads are in this state before they are first readied. */
	Entering,
	/** After a thread calls thread_exit(), it is put into Exiting state. */
	Exiting,
	/** Threads that were not detached but exited are Lingering. */
	Lingering
} state_t;

/** Thread structure. There is one per thread. */
typedef struct thread {
	link_t rq_link;		/**< Run queue link. */
	link_t wq_link;		/**< Wait queue link. */
	link_t th_link;		/**< Links to threads within containing task. */

	/** Threads linkage to the threads_tree. */
	avltree_node_t threads_tree_node;
	
	/** Lock protecting thread structure.
	 *
	 * Protects the whole thread structure except list links above.
	 */
	SPINLOCK_DECLARE(lock);

	char name[THREAD_NAME_BUFLEN];

	/** Function implementing the thread. */
	void (* thread_code)(void *);
	/** Argument passed to thread_code() function. */
	void *thread_arg;

	/**
	 * From here, the stored context is restored when the thread is
	 * scheduled.
	 */
	context_t saved_context;
	/**
	 * From here, the stored timeout context is restored when sleep times
	 * out.
	 */
	context_t sleep_timeout_context;
	/**
	 * From here, the stored interruption context is restored when sleep is
	 * interrupted.
	 */
	context_t sleep_interruption_context;

	/** If true, the thread can be interrupted from sleep. */
	bool sleep_interruptible;
	/** Wait queue in which this thread sleeps. */
	waitq_t *sleep_queue;
	/** Timeout used for timeoutable sleeping.  */
	timeout_t sleep_timeout;
	/** Flag signalling sleep timeout in progress. */
	volatile int timeout_pending;

	/**
	 * True if this thread is executing copy_from_uspace().
	 * False otherwise.
	 */
	bool in_copy_from_uspace;
	/**
	 * True if this thread is executing copy_to_uspace().
	 * False otherwise.
	 */
	bool in_copy_to_uspace;
	
	/**
	 * If true, the thread will not go to sleep at all and will call
	 * thread_exit() before returning to userspace.
	 */
	bool interrupted;			
	
	/** If true, thread_join_timeout() cannot be used on this thread. */
	bool detached;
	/** Waitq for thread_join_timeout(). */
	waitq_t join_wq;
	/** Link used in the joiner_head list. */
	link_t joiner_link;

	fpu_context_t *saved_fpu_context;
	int fpu_context_exists;

	/*
	 * Defined only if thread doesn't run.
	 * It means that fpu context is in CPU that last time executes this
	 * thread. This disables migration.
	 */
	int fpu_context_engaged;

	rwlock_type_t rwlock_holder_type;

	/** Callback fired in scheduler before the thread is put asleep. */
	void (* call_me)(void *);
	/** Argument passed to call_me(). */
	void *call_me_with;

	/** Thread's state. */
	state_t state;
	/** Thread's flags. */
	int flags;
	
	/** Thread's CPU. */
	cpu_t *cpu;
	/** Containing task. */
	task_t *task;

	/** Ticks before preemption. */
	uint64_t ticks;
	
	/** Thread accounting. */
	uint64_t cycles;
	/** Last sampled cycle. */
	uint64_t last_cycle;
	/** Thread doesn't affect accumulated accounting. */	
	bool uncounted;

	/** Thread's priority. Implemented as index to CPU->rq */
	int priority;
	/** Thread ID. */
	thread_id_t tid;
	
	/** Architecture-specific data. */
	thread_arch_t arch;

	/** Thread's kernel stack. */
	uint8_t *kstack;
} thread_t;

/** Thread list lock.
 *
 * This lock protects the threads_tree.
 * Must be acquired before T.lock for each T of type thread_t.
 *
 */
SPINLOCK_EXTERN(threads_lock);

/** AVL tree containing all threads. */
extern avltree_t threads_tree;

extern void thread_init(void);
extern thread_t *thread_create(void (* func)(void *), void *arg, task_t *task,
    int flags, char *name, bool uncounted);
extern void thread_attach(thread_t *t, task_t *task);
extern void thread_ready(thread_t *t);
extern void thread_exit(void) __attribute__((noreturn));

#ifndef thread_create_arch
extern void thread_create_arch(thread_t *t);
#endif
#ifndef thr_constructor_arch
extern void thr_constructor_arch(thread_t *t);
#endif
#ifndef thr_destructor_arch
extern void thr_destructor_arch(thread_t *t);
#endif

extern void thread_sleep(uint32_t sec);
extern void thread_usleep(uint32_t usec);

#define thread_join(t) \
	thread_join_timeout((t), SYNCH_NO_TIMEOUT, SYNCH_FLAGS_NONE)
extern int thread_join_timeout(thread_t *t, uint32_t usec, int flags);
extern void thread_detach(thread_t *t);

extern void thread_register_call_me(void (* call_me)(void *),
    void *call_me_with);
extern void thread_print_list(void);
extern void thread_destroy(thread_t *t);
extern void thread_update_accounting(void);
extern bool thread_exists(thread_t *t);

/** Fpu context slab cache. */
extern slab_cache_t *fpu_context_slab;

/* Thread syscall prototypes. */
extern unative_t sys_thread_create(uspace_arg_t *uspace_uarg, char *uspace_name, thread_id_t *uspace_thread_id);
extern unative_t sys_thread_exit(int uspace_status);
extern unative_t sys_thread_get_id(thread_id_t *uspace_thread_id);

#endif

/** @}
 */
