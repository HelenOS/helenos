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

#ifndef __THREAD_H__
#define __THREAD_H__

#include <arch/proc/thread.h>
#include <synch/spinlock.h>
#include <arch/context.h>
#include <fpu_context.h>
#include <arch/types.h>
#include <typedefs.h>
#include <time/timeout.h>
#include <synch/rwlock.h>
#include <config.h>
#include <adt/btree.h>
#include <adt/list.h>
#include <mm/slab.h>
#include <proc/uarg.h>

#define THREAD_STACK_SIZE	STACK_SIZE

enum state {
	Invalid,	/**< It is an error, if thread is found in this state. */
	Running,	/**< State of a thread that is currently executing on some CPU. */
	Sleeping,	/**< Thread in this state is waiting for an event. */
	Ready,		/**< State of threads in a run queue. */
	Entering,	/**< Threads are in this state before they are first readied. */
	Exiting		/**< After a thread calls thread_exit(), it is put into Exiting state. */
};

extern char *thread_states[];

#define X_WIRED		(1<<0)
#define X_STOLEN	(1<<1)

#define THREAD_NAME_BUFLEN	20

/** Thread structure. There is one per thread. */
struct thread {
	link_t rq_link;				/**< Run queue link. */
	link_t wq_link;				/**< Wait queue link. */
	link_t th_link;				/**< Links to threads within containing task. */
	
	/** Lock protecting thread structure.
	 *
	 * Protects the whole thread structure except list links above.
	 * Must be acquired before T.lock for each T of type task_t.
	 * 
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

	waitq_t *sleep_queue;			/**< Wait queue in which this thread sleeps. */
	timeout_t sleep_timeout;		/**< Timeout used for timeoutable sleeping.  */
	volatile int timeout_pending;		/**< Flag signalling sleep timeout in progress. */

	fpu_context_t *saved_fpu_context;
	int fpu_context_exists;

	/*
	 * Defined only if thread doesn't run.
	 * It means that fpu context is in CPU that last time executes this thread.
	 * This disables migration
	 */
	int fpu_context_engaged;

	rwlock_type_t rwlock_holder_type;

	void (* call_me)(void *);		/**< Funtion to be called in scheduler before the thread is put asleep. */
	void *call_me_with;			/**< Argument passed to call_me(). */

	state_t state;				/**< Thread's state. */
	int flags;				/**< Thread's flags. */
	
	cpu_t *cpu;				/**< Thread's CPU. */
	task_t *task;				/**< Containing task. */

	__u64 ticks;				/**< Ticks before preemption. */

	int priority;				/**< Thread's priority. Implemented as index to CPU->rq */
	__u32 tid;				/**< Thread ID. */
	
	thread_arch_t arch;			/**< Architecture-specific data. */

	__u8 *kstack;				/**< Thread's kernel stack. */
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
extern thread_t *thread_create(void (* func)(void *), void *arg, task_t *task, int flags, char *name);
extern void thread_ready(thread_t *t);
extern void thread_exit(void);

#ifndef thread_create_arch
extern void thread_create_arch(thread_t *t);
#endif

extern void thread_sleep(__u32 sec);
extern void thread_usleep(__u32 usec);

extern void thread_register_call_me(void (* call_me)(void *), void *call_me_with);
extern void thread_print_list(void);
extern void thread_destroy(thread_t *t);
extern bool thread_exists(thread_t *t);

/* Fpu context slab cache */
extern slab_cache_t *fpu_context_slab;

/** Thread syscall prototypes. */
__native sys_thread_create(uspace_arg_t *uspace_uarg, char *uspace_name);
__native sys_thread_exit(int uspace_status);

#endif
