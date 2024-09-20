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

/** @addtogroup kernel_generic_proc
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
#include <synch/spinlock.h>
#include <adt/odict.h>
#include <mm/slab.h>
#include <arch/cpu.h>
#include <mm/tlb.h>
#include <udebug/udebug.h>
#include <abi/proc/thread.h>
#include <abi/sysinfo.h>
#include <arch.h>

#define THREAD              CURRENT->thread

#define THREAD_NAME_BUFLEN  20

extern const char *thread_states[];

/* Thread flags */
typedef enum {
	THREAD_FLAG_NONE = 0,
	/** Thread executes in user space. */
	THREAD_FLAG_USPACE = (1 << 0),
	/** Thread will be attached by the caller. */
	THREAD_FLAG_NOATTACH = (1 << 1),
	/** Thread accounting doesn't affect accumulated task accounting. */
	THREAD_FLAG_UNCOUNTED = (1 << 2)
} thread_flags_t;

/** Thread structure. There is one per thread. */
typedef struct thread {
	atomic_refcount_t refcount;

	link_t rq_link;  /**< Run queue link. */
	link_t wq_link;  /**< Wait queue link. */
	link_t th_link;  /**< Links to threads within containing task. */

	/** Link to @c threads ordered dictionary. */
	odlink_t lthreads;

	/** Tracking variable for thread_wait/thread_wakeup */
	atomic_int sleep_state;

	/**
	 * If true, the thread is terminating.
	 * It will not go to sleep in interruptible synchronization functions
	 * and will call thread_exit() before returning to userspace.
	 */
	volatile bool interrupted;

	/** Wait queue in which this thread sleeps. Used for debug printouts. */
	_Atomic(waitq_t *) sleep_queue;

	/** Waitq for thread_join_timeout(). */
	waitq_t join_wq;

	/** Thread accounting. */
	atomic_time_stat_t ucycles;
	atomic_time_stat_t kcycles;

	/** Architecture-specific data. */
	thread_arch_t arch;

#ifdef CONFIG_UDEBUG
	/**
	 * If true, the scheduler will print a stack trace
	 * to the kernel console upon scheduling this thread.
	 */
	atomic_int_fast8_t btrace;

	/** Debugging stuff */
	udebug_thread_t udebug;
#endif /* CONFIG_UDEBUG */

	/*
	 * Immutable fields.
	 *
	 * These fields are only modified during initialization, and are not
	 * changed at any time between initialization and destruction.
	 * Can be accessed without synchronization in most places.
	 */

	/** Thread ID. */
	thread_id_t tid;

	/** Function implementing the thread. */
	void (*thread_code)(void *);
	/** Argument passed to thread_code() function. */
	void *thread_arg;

	char name[THREAD_NAME_BUFLEN];

	/** Thread is executed in user space. */
	bool uspace;

	/** Thread doesn't affect accumulated accounting. */
	bool uncounted;

	/** Containing task. */
	task_t *task;

	/** Thread's kernel stack. */
	uint8_t *kstack;

	/*
	 * Local fields.
	 *
	 * These fields can be safely accessed from code that _controls execution_
	 * of this thread. Code controls execution of a thread if either:
	 *  - it runs in the context of said thread AND interrupts are disabled
	 *    (interrupts can and will access these fields)
	 *  - the thread is not running, and the code accessing it can legally
	 *    add/remove the thread to/from a runqueue, i.e., either:
	 *    - it is allowed to enqueue thread in a new runqueue
	 *    - it holds the lock to the runqueue containing the thread
	 *
	 */

	/**
	 * From here, the stored context is restored
	 * when the thread is scheduled.
	 */
	context_t saved_context;

	// TODO: we only need one of the two bools below

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

	/*
	 * FPU context is a special case. If lazy FPU switching is disabled,
	 * it acts as a regular local field. However, if lazy switching is enabled,
	 * the context is synchronized via CPU->fpu_lock
	 */
#ifdef CONFIG_FPU
	fpu_context_t fpu_context;
#endif
	bool fpu_context_exists;

	/* The thread will not be migrated if nomigrate is non-zero. */
	unsigned int nomigrate;

	/** Thread was migrated to another CPU and has not run yet. */
	bool stolen;

	/**
	 * Thread state (state_t).
	 * This is atomic because we read it via some commands for debug output,
	 * otherwise it could just be a regular local.
	 */
	atomic_int_fast32_t state;

	/** Thread CPU. */
	_Atomic(cpu_t *) cpu;

	/** Thread's priority. Implemented as index to CPU->rq */
	atomic_int_fast32_t priority;

	/** Last sampled cycle. */
	uint64_t last_cycle;
} thread_t;

IRQ_SPINLOCK_EXTERN(threads_lock);
extern odict_t threads;

extern void thread_init(void);
extern thread_t *thread_create(void (*)(void *), void *, task_t *,
    thread_flags_t, const char *);
extern void thread_wire(thread_t *, cpu_t *);
extern void thread_attach(thread_t *, task_t *);
extern void thread_start(thread_t *);
extern void thread_requeue_sleeping(thread_t *);
extern void thread_exit(void) __attribute__((noreturn));
extern void thread_interrupt(thread_t *);

enum sleep_state {
	SLEEP_INITIAL,
	SLEEP_ASLEEP,
	SLEEP_WOKE,
};

typedef enum {
	THREAD_OK,
	THREAD_TERMINATING,
} thread_termination_state_t;

typedef enum {
	THREAD_WAIT_SUCCESS,
	THREAD_WAIT_TIMEOUT,
} thread_wait_result_t;

extern thread_termination_state_t thread_wait_start(void);
extern thread_wait_result_t thread_wait_finish(deadline_t);
extern void thread_wakeup(thread_t *);

static inline thread_t *thread_ref(thread_t *thread)
{
	refcount_up(&thread->refcount);
	return thread;
}

static inline thread_t *thread_try_ref(thread_t *thread)
{
	if (refcount_try_up(&thread->refcount))
		return thread;
	else
		return NULL;
}

extern void thread_put(thread_t *);

#ifndef thread_create_arch
extern errno_t thread_create_arch(thread_t *, thread_flags_t);
#endif

#ifndef thr_constructor_arch
extern void thr_constructor_arch(thread_t *);
#endif

#ifndef thr_destructor_arch
extern void thr_destructor_arch(thread_t *);
#endif

extern void thread_sleep(uint32_t);
extern void thread_usleep(uint32_t);

extern errno_t thread_join(thread_t *);
extern errno_t thread_join_timeout(thread_t *, uint32_t, unsigned int);
extern void thread_detach(thread_t *);

extern void thread_yield(void);

extern void thread_print_list(bool);
extern thread_t *thread_find_by_id(thread_id_t);
extern size_t thread_count(void);
extern thread_t *thread_first(void);
extern thread_t *thread_next(thread_t *);
extern void thread_update_accounting(bool);
extern thread_t *thread_try_get(thread_t *);

extern void thread_migration_disable(void);
extern void thread_migration_enable(void);

#ifdef CONFIG_UDEBUG
extern void thread_stack_trace(thread_id_t);
#endif

/** Fpu context slab cache. */
extern slab_cache_t *fpu_context_cache;

/* Thread syscall prototypes. */
extern sys_errno_t sys_thread_create(uspace_ptr_uspace_arg_t, uspace_ptr_char, size_t,
    uspace_ptr_thread_id_t);
extern sys_errno_t sys_thread_exit(int);
extern sys_errno_t sys_thread_get_id(uspace_ptr_thread_id_t);
extern sys_errno_t sys_thread_usleep(uint32_t);
extern sys_errno_t sys_thread_udelay(uint32_t);

#endif

/** @}
 */
