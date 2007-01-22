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

#ifndef KERN_THREAD_H_
#define KERN_THREAD_H_

#include <arch/proc/thread.h>
#include <synch/spinlock.h>
#include <arch/context.h>
#include <fpu_context.h>
#include <arch/types.h>
#include <typedefs.h>
#include <time/timeout.h>
#include <synch/rwlock.h>
#include <synch/synch.h>
#include <config.h>
#include <adt/btree.h>
#include <adt/list.h>
#include <mm/slab.h>
#include <proc/uarg.h>

#define THREAD_STACK_SIZE	STACK_SIZE

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

extern char *thread_states[];

/** Join types. */
typedef enum {
	None,
	TaskClnp,	/**< The thread will be joined by ktaskclnp thread. */
	TaskGC		/**< The thread will be joined by ktaskgc thread. */
} thread_join_type_t;

/* Thread flags */
#define THREAD_FLAG_WIRED	(1<<0)	/**< Thread cannot be migrated to another CPU. */
#define THREAD_FLAG_STOLEN	(1<<1)	/**< Thread was migrated to another CPU and has not run yet. */
#define THREAD_FLAG_USPACE	(1<<2)	/**< Thread executes in userspace. */

#define THREAD_NAME_BUFLEN	20

/** Thread structure. There is one per thread. */
struct thread {
	link_t rq_link;				/**< Run queue link. */
	link_t wq_link;				/**< Wait queue link. */
	link_t th_link;				/**< Links to threads within containing task. */
	
	/** Lock protecting thread structure.
	 *
	 * Protects the whole thread structure except list links above.
	 */
	SPINLOCK_DECLARE(lock);

	char name[THREAD_NAME_BUFLEN];

	void (* thread_code)(void *);		/**< Function implementing the thread. */
	void *thread_arg;			/**< Argument passed to thread_code() function. */

	/** From here, the stored context is restored when the thread is scheduled. */
	context_t saved_context;
	/** From here, the stored timeout context is restored when sleep times out. */
	context_t sleep_timeout_context;
	/** From here, the stored interruption context is restored when sleep is interrupted. */
	context_t sleep_interruption_context;

	bool sleep_interruptible;		/**< If true, the thread can be interrupted from sleep. */
	waitq_t *sleep_queue;			/**< Wait queue in which this thread sleeps. */
	timeout_t sleep_timeout;		/**< Timeout used for timeoutable sleeping.  */
	volatile int timeout_pending;		/**< Flag signalling sleep timeout in progress. */

	/** True if this thread is executing copy_from_uspace(). False otherwise. */
	bool in_copy_from_uspace;
	/** True if this thread is executing copy_to_uspace(). False otherwise. */
	bool in_copy_to_uspace;
	
	/**
	 * If true, the thread will not go to sleep at all and will
	 * call thread_exit() before returning to userspace.
	 */
	bool interrupted;			
	
	thread_join_type_t	join_type;	/**< Who joinins the thread. */
	bool detached;				/**< If true, thread_join_timeout() cannot be used on this thread. */
	waitq_t join_wq;			/**< Waitq for thread_join_timeout(). */

	fpu_context_t *saved_fpu_context;
	int fpu_context_exists;

	/*
	 * Defined only if thread doesn't run.
	 * It means that fpu context is in CPU that last time executes this thread.
	 * This disables migration.
	 */
	int fpu_context_engaged;

	rwlock_type_t rwlock_holder_type;

	void (* call_me)(void *);		/**< Funtion to be called in scheduler before the thread is put asleep. */
	void *call_me_with;			/**< Argument passed to call_me(). */

	state_t state;				/**< Thread's state. */
	int flags;				/**< Thread's flags. */
	
	cpu_t *cpu;				/**< Thread's CPU. */
	task_t *task;				/**< Containing task. */

	uint64_t ticks;				/**< Ticks before preemption. */
	
	uint64_t cycles;			/**< Thread accounting. */
	uint64_t last_cycle;		/**< Last sampled cycle. */
	bool uncounted;				/**< Thread doesn't affect accumulated accounting. */

	int priority;				/**< Thread's priority. Implemented as index to CPU->rq */
	uint32_t tid;				/**< Thread ID. */
	
	thread_arch_t arch;			/**< Architecture-specific data. */

	uint8_t *kstack;			/**< Thread's kernel stack. */
};

/** Thread list lock.
 *
 * This lock protects all link_t structures chained in threads_head.
 * Must be acquired before T.lock for each T of type thread_t.
 *
 */
extern spinlock_t threads_lock;

extern btree_t threads_btree;			/**< B+tree containing all threads. */

extern void thread_init(void);
extern thread_t *thread_create(void (* func)(void *), void *arg, task_t *task, int flags, char *name, bool uncounted);
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

#define thread_join(t)	thread_join_timeout((t), SYNCH_NO_TIMEOUT, SYNCH_FLAGS_NONE)
extern int thread_join_timeout(thread_t *t, uint32_t usec, int flags);
extern void thread_detach(thread_t *t);

extern void thread_register_call_me(void (* call_me)(void *), void *call_me_with);
extern void thread_print_list(void);
extern void thread_destroy(thread_t *t);
extern void thread_update_accounting(void);
extern bool thread_exists(thread_t *t);

/* Fpu context slab cache */
extern slab_cache_t *fpu_context_slab;

/** Thread syscall prototypes. */
unative_t sys_thread_create(uspace_arg_t *uspace_uarg, char *uspace_name);
unative_t sys_thread_exit(int uspace_status);

#endif

/** @}
 */
