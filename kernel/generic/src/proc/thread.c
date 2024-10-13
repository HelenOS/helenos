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
 * @brief Thread management functions.
 */

#include <assert.h>
#include <proc/scheduler.h>
#include <proc/thread.h>
#include <proc/task.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <arch/asm.h>
#include <arch/cycle.h>
#include <arch.h>
#include <synch/spinlock.h>
#include <synch/waitq.h>
#include <synch/syswaitq.h>
#include <cpu.h>
#include <str.h>
#include <context.h>
#include <adt/list.h>
#include <adt/odict.h>
#include <time/clock.h>
#include <time/timeout.h>
#include <time/delay.h>
#include <config.h>
#include <arch/interrupt.h>
#include <smp/ipi.h>
#include <atomic.h>
#include <memw.h>
#include <stdio.h>
#include <stdlib.h>
#include <main/uinit.h>
#include <syscall/copy.h>
#include <errno.h>
#include <debug.h>
#include <halt.h>

/** Thread states */
const char *thread_states[] = {
	"Invalid",
	"Running",
	"Sleeping",
	"Ready",
	"Entering",
	"Exiting",
	"Lingering"
};

/** Lock protecting the @c threads ordered dictionary .
 *
 * For locking rules, see declaration thereof.
 */
IRQ_SPINLOCK_INITIALIZE(threads_lock);

/** Ordered dictionary of all threads by their address (i.e. pointer to
 * the thread_t structure).
 *
 * When a thread is found in the @c threads ordered dictionary, it is
 * guaranteed to exist as long as the @c threads_lock is held.
 *
 * Members are of type thread_t.
 *
 * This structure contains weak references. Any reference from it must not leave
 * threads_lock critical section unless strengthened via thread_try_ref().
 */
odict_t threads;

IRQ_SPINLOCK_STATIC_INITIALIZE(tidlock);
static thread_id_t last_tid = 0;

static slab_cache_t *thread_cache;

static void *threads_getkey(odlink_t *);
static int threads_cmp(void *, void *);

/** Initialization and allocation for thread_t structure
 *
 */
static errno_t thr_constructor(void *obj, unsigned int kmflags)
{
	thread_t *thread = (thread_t *) obj;

	link_initialize(&thread->rq_link);
	link_initialize(&thread->wq_link);
	link_initialize(&thread->th_link);

	/* call the architecture-specific part of the constructor */
	thr_constructor_arch(thread);

	/*
	 * Allocate the kernel stack from the low-memory to prevent an infinite
	 * nesting of TLB-misses when accessing the stack from the part of the
	 * TLB-miss handler written in C.
	 *
	 * Note that low-memory is safe to be used for the stack as it will be
	 * covered by the kernel identity mapping, which guarantees not to
	 * nest TLB-misses infinitely (either via some hardware mechanism or
	 * by the construction of the assembly-language part of the TLB-miss
	 * handler).
	 *
	 * This restriction can be lifted once each architecture provides
	 * a similar guarantee, for example, by locking the kernel stack
	 * in the TLB whenever it is allocated from the high-memory and the
	 * thread is being scheduled to run.
	 */
	kmflags |= FRAME_LOWMEM;
	kmflags &= ~FRAME_HIGHMEM;

	/*
	 * NOTE: All kernel stacks must be aligned to STACK_SIZE,
	 *       see CURRENT.
	 */

	uintptr_t stack_phys =
	    frame_alloc(STACK_FRAMES, kmflags, STACK_SIZE - 1);
	if (!stack_phys)
		return ENOMEM;

	thread->kstack = (uint8_t *) PA2KA(stack_phys);

#ifdef CONFIG_UDEBUG
	mutex_initialize(&thread->udebug.lock, MUTEX_PASSIVE);
#endif

	return EOK;
}

/** Destruction of thread_t object */
static size_t thr_destructor(void *obj)
{
	thread_t *thread = (thread_t *) obj;

	/* call the architecture-specific part of the destructor */
	thr_destructor_arch(thread);

	frame_free(KA2PA(thread->kstack), STACK_FRAMES);

	return STACK_FRAMES;  /* number of frames freed */
}

/** Initialize threads
 *
 * Initialize kernel threads support.
 *
 */
void thread_init(void)
{
	THREAD = NULL;

	atomic_store(&nrdy, 0);
	thread_cache = slab_cache_create("thread_t", sizeof(thread_t), _Alignof(thread_t),
	    thr_constructor, thr_destructor, 0);

	odict_initialize(&threads, threads_getkey, threads_cmp);
}

/** Wire thread to the given CPU
 *
 * @param cpu CPU to wire the thread to.
 *
 */
void thread_wire(thread_t *thread, cpu_t *cpu)
{
	ipl_t ipl = interrupts_disable();
	atomic_set_unordered(&thread->cpu, cpu);
	thread->nomigrate++;
	interrupts_restore(ipl);
}

/** Start a thread that wasn't started yet since it was created.
 *
 * @param thread A reference to the newly created thread.
 */
void thread_start(thread_t *thread)
{
	assert(atomic_get_unordered(&thread->state) == Entering);
	thread_requeue_sleeping(thread_ref(thread));
}

/** Create new thread
 *
 * Create a new thread.
 *
 * @param func      Thread's implementing function.
 * @param arg       Thread's implementing function argument.
 * @param task      Task to which the thread belongs. The caller must
 *                  guarantee that the task won't cease to exist during the
 *                  call. The task's lock may not be held.
 * @param flags     Thread flags.
 * @param name      Symbolic name (a copy is made).
 *
 * @return New thread's structure on success, NULL on failure.
 *
 */
thread_t *thread_create(void (*func)(void *), void *arg, task_t *task,
    thread_flags_t flags, const char *name)
{
	thread_t *thread = (thread_t *) slab_alloc(thread_cache, FRAME_ATOMIC);
	if (!thread)
		return NULL;

	refcount_init(&thread->refcount);

	if (thread_create_arch(thread, flags) != EOK) {
		slab_free(thread_cache, thread);
		return NULL;
	}

	/* Not needed, but good for debugging */
	memsetb(thread->kstack, STACK_SIZE, 0);

	irq_spinlock_lock(&tidlock, true);
	thread->tid = ++last_tid;
	irq_spinlock_unlock(&tidlock, true);

	context_create(&thread->saved_context, thread_main_func,
	    thread->kstack, STACK_SIZE);

	current_initialize((current_t *) thread->kstack);

	str_cpy(thread->name, THREAD_NAME_BUFLEN, name);

	thread->thread_code = func;
	thread->thread_arg = arg;
	thread->ucycles = ATOMIC_TIME_INITIALIZER();
	thread->kcycles = ATOMIC_TIME_INITIALIZER();
	thread->uncounted =
	    ((flags & THREAD_FLAG_UNCOUNTED) == THREAD_FLAG_UNCOUNTED);
	atomic_init(&thread->priority, 0);
	atomic_init(&thread->cpu, NULL);
	thread->stolen = false;
	thread->uspace =
	    ((flags & THREAD_FLAG_USPACE) == THREAD_FLAG_USPACE);

	thread->nomigrate = 0;
	atomic_init(&thread->state, Entering);

	atomic_init(&thread->sleep_queue, NULL);

	thread->in_copy_from_uspace = false;
	thread->in_copy_to_uspace = false;

	thread->interrupted = false;
	atomic_init(&thread->sleep_state, SLEEP_INITIAL);

	waitq_initialize(&thread->join_wq);

	thread->task = task;

	thread->fpu_context_exists = false;

	odlink_initialize(&thread->lthreads);

#ifdef CONFIG_UDEBUG
	/* Initialize debugging stuff */
	atomic_init(&thread->btrace, false);
	udebug_thread_initialize(&thread->udebug);
#endif

	if ((flags & THREAD_FLAG_NOATTACH) != THREAD_FLAG_NOATTACH)
		thread_attach(thread, task);

	return thread;
}

/** Destroy thread memory structure
 *
 * Detach thread from all queues, cpus etc. and destroy it.
 *
 * @param obj  Thread to be destroyed.
 *
 */
static void thread_destroy(void *obj)
{
	thread_t *thread = (thread_t *) obj;

	assert_link_not_used(&thread->rq_link);
	assert_link_not_used(&thread->wq_link);

	assert(thread->task);

	ipl_t ipl = interrupts_disable();

	/* Remove thread from global list. */
	irq_spinlock_lock(&threads_lock, false);
	odict_remove(&thread->lthreads);
	irq_spinlock_unlock(&threads_lock, false);

	/* Remove thread from task's list and accumulate accounting. */
	irq_spinlock_lock(&thread->task->lock, false);

	list_remove(&thread->th_link);

	/*
	 * No other CPU has access to this thread anymore, so we don't need
	 * thread->lock for accessing thread's fields after this point.
	 */

	if (!thread->uncounted) {
		thread->task->ucycles += atomic_time_read(&thread->ucycles);
		thread->task->kcycles += atomic_time_read(&thread->kcycles);
	}

	irq_spinlock_unlock(&thread->task->lock, false);

	assert((atomic_get_unordered(&thread->state) == Exiting) || (atomic_get_unordered(&thread->state) == Lingering));

	/* Clear cpu->fpu_owner if set to this thread. */
#ifdef CONFIG_FPU_LAZY
	cpu_t *cpu = atomic_get_unordered(&thread->cpu);
	if (cpu) {
		/*
		 * We need to lock for this because the old CPU can concurrently try
		 * to dump this thread's FPU state, in which case we need to wait for
		 * it to finish. An atomic compare-and-swap wouldn't be enough.
		 */
		irq_spinlock_lock(&cpu->fpu_lock, false);

		if (atomic_get_unordered(&cpu->fpu_owner) == thread)
			atomic_set_unordered(&cpu->fpu_owner, NULL);

		irq_spinlock_unlock(&cpu->fpu_lock, false);
	}
#endif

	interrupts_restore(ipl);

	/*
	 * Drop the reference to the containing task.
	 */
	task_release(thread->task);
	thread->task = NULL;

	slab_free(thread_cache, thread);
}

void thread_put(thread_t *thread)
{
	if (refcount_down(&thread->refcount)) {
		thread_destroy(thread);
	}
}

/** Make the thread visible to the system.
 *
 * Attach the thread structure to the current task and make it visible in the
 * threads_tree.
 *
 * @param t    Thread to be attached to the task.
 * @param task Task to which the thread is to be attached.
 *
 */
void thread_attach(thread_t *thread, task_t *task)
{
	ipl_t ipl = interrupts_disable();

	/*
	 * Attach to the specified task.
	 */
	irq_spinlock_lock(&task->lock, false);

	/* Hold a reference to the task. */
	task_hold(task);

	/* Must not count kbox thread into lifecount */
	if (thread->uspace)
		atomic_inc(&task->lifecount);

	list_append(&thread->th_link, &task->threads);

	irq_spinlock_unlock(&task->lock, false);

	/*
	 * Register this thread in the system-wide dictionary.
	 */
	irq_spinlock_lock(&threads_lock, false);
	odict_insert(&thread->lthreads, &threads, NULL);
	irq_spinlock_unlock(&threads_lock, false);

	interrupts_restore(ipl);
}

/** Terminate thread.
 *
 * End current thread execution and switch it to the exiting state.
 * All pending timeouts are executed.
 *
 */
void thread_exit(void)
{
	if (THREAD->uspace) {
#ifdef CONFIG_UDEBUG
		/* Generate udebug THREAD_E event */
		udebug_thread_e_event();

		/*
		 * This thread will not execute any code or system calls from
		 * now on.
		 */
		udebug_stoppable_begin();
#endif
		if (atomic_predec(&TASK->lifecount) == 0) {
			/*
			 * We are the last userspace thread in the task that
			 * still has not exited. With the exception of the
			 * moment the task was created, new userspace threads
			 * can only be created by threads of the same task.
			 * We are safe to perform cleanup.
			 *
			 */
			ipc_cleanup();
			sys_waitq_task_cleanup();
			LOG("Cleanup of task %" PRIu64 " completed.", TASK->taskid);
		}
	}

	scheduler_enter(Exiting);
	unreachable();
}

/** Interrupts an existing thread so that it may exit as soon as possible.
 *
 * Threads that are blocked waiting for a synchronization primitive
 * are woken up with a return code of EINTR if the
 * blocking call was interruptable. See waitq_sleep_timeout().
 *
 * Interrupted threads automatically exit when returning back to user space.
 *
 * @param thread A valid thread object.
 */
void thread_interrupt(thread_t *thread)
{
	assert(thread != NULL);
	thread->interrupted = true;
	thread_wakeup(thread);
}

/** Prepare for putting the thread to sleep.
 *
 * @returns whether the thread is currently terminating. If THREAD_OK
 * is returned, the thread is guaranteed to be woken up instantly if the thread
 * is terminated at any time between this function's return and
 * thread_wait_finish(). If THREAD_TERMINATING is returned, the thread can still
 * go to sleep, but doing so will delay termination.
 */
thread_termination_state_t thread_wait_start(void)
{
	assert(THREAD != NULL);

	/*
	 * This is an exchange rather than a store so that we can use the acquire
	 * semantics, which is needed to ensure that code after this operation sees
	 * memory ops made before thread_wakeup() in other thread, if that wakeup
	 * was reset by this operation.
	 *
	 * In particular, we need this to ensure we can't miss the thread being
	 * terminated concurrently with a synchronization primitive preparing to
	 * sleep.
	 */
	(void) atomic_exchange_explicit(&THREAD->sleep_state, SLEEP_INITIAL,
	    memory_order_acquire);

	return THREAD->interrupted ? THREAD_TERMINATING : THREAD_OK;
}

static void thread_wait_timeout_callback(void *arg)
{
	thread_wakeup(arg);
}

/**
 * Suspends this thread's execution until thread_wakeup() is called on it,
 * or deadline is reached.
 *
 * The way this would normally be used is that the current thread call
 * thread_wait_start(), and if interruption has not been signaled, stores
 * a reference to itself in a synchronized structure (such as waitq).
 * After that, it releases any spinlocks it might hold and calls this function.
 *
 * The thread doing the wakeup will acquire the thread's reference from said
 * synchronized structure and calls thread_wakeup() on it.
 *
 * Notably, there can be more than one thread performing wakeup.
 * The number of performed calls to thread_wakeup(), or their relative
 * ordering with thread_wait_finish(), does not matter. However, calls to
 * thread_wakeup() are expected to be synchronized with thread_wait_start()
 * with which they are associated, otherwise wakeups may be missed.
 * However, the operation of thread_wakeup() is defined at any time,
 * synchronization notwithstanding (in the sense of C un/defined behavior),
 * and is in fact used to interrupt waiting threads by external events.
 * The waiting thread must operate correctly in face of spurious wakeups,
 * and clean up its reference in the synchronization structure if necessary.
 *
 * Returns THREAD_WAIT_TIMEOUT if timeout fired, which is a necessary condition
 * for it to have been waken up by the timeout, but the caller must assume
 * that proper wakeups, timeouts and interrupts may occur concurrently, so
 * the fact timeout has been registered does not necessarily mean the thread
 * has not been woken up or interrupted.
 */
thread_wait_result_t thread_wait_finish(deadline_t deadline)
{
	assert(THREAD != NULL);

	timeout_t timeout;

	/* Extra check to avoid going to scheduler if we don't need to. */
	if (atomic_load_explicit(&THREAD->sleep_state, memory_order_acquire) !=
	    SLEEP_INITIAL)
		return THREAD_WAIT_SUCCESS;

	if (deadline != DEADLINE_NEVER) {
		timeout_initialize(&timeout);
		timeout_register_deadline(&timeout, deadline,
		    thread_wait_timeout_callback, THREAD);
	}

	scheduler_enter(Sleeping);

	if (deadline != DEADLINE_NEVER && !timeout_unregister(&timeout)) {
		return THREAD_WAIT_TIMEOUT;
	} else {
		return THREAD_WAIT_SUCCESS;
	}
}

void thread_wakeup(thread_t *thread)
{
	assert(thread != NULL);

	int state = atomic_exchange_explicit(&thread->sleep_state, SLEEP_WOKE,
	    memory_order_acq_rel);

	if (state == SLEEP_ASLEEP) {
		/*
		 * Only one thread gets to do this.
		 * The reference consumed here is the reference implicitly passed to
		 * the waking thread by the sleeper in thread_wait_finish().
		 */
		thread_requeue_sleeping(thread);
	}
}

/** Prevent the current thread from being migrated to another processor. */
void thread_migration_disable(void)
{
	ipl_t ipl = interrupts_disable();

	assert(THREAD);
	THREAD->nomigrate++;

	interrupts_restore(ipl);
}

/** Allow the current thread to be migrated to another processor. */
void thread_migration_enable(void)
{
	ipl_t ipl = interrupts_disable();

	assert(THREAD);
	assert(THREAD->nomigrate > 0);

	if (THREAD->nomigrate > 0)
		THREAD->nomigrate--;

	interrupts_restore(ipl);
}

/** Thread sleep
 *
 * Suspend execution of the current thread.
 *
 * @param sec Number of seconds to sleep.
 *
 */
void thread_sleep(uint32_t sec)
{
	/*
	 * Sleep in 1000 second steps to support
	 * full argument range
	 */
	while (sec > 0) {
		uint32_t period = (sec > 1000) ? 1000 : sec;

		thread_usleep(period * 1000000);
		sec -= period;
	}
}

errno_t thread_join(thread_t *thread)
{
	return thread_join_timeout(thread, SYNCH_NO_TIMEOUT, SYNCH_FLAGS_NONE);
}

/** Wait for another thread to exit.
 * After successful wait, the thread reference is destroyed.
 *
 * @param thread Thread to join on exit.
 * @param usec   Timeout in microseconds.
 * @param flags  Mode of operation.
 *
 * @return An error code from errno.h or an error code from synch.h.
 *
 */
errno_t thread_join_timeout(thread_t *thread, uint32_t usec, unsigned int flags)
{
	assert(thread != NULL);

	if (thread == THREAD)
		return EINVAL;

	errno_t rc = _waitq_sleep_timeout(&thread->join_wq, usec, flags);

	if (rc == EOK)
		thread_put(thread);

	return rc;
}

void thread_detach(thread_t *thread)
{
	thread_put(thread);
}

/** Thread usleep
 *
 * Suspend execution of the current thread.
 *
 * @param usec Number of microseconds to sleep.
 *
 */
void thread_usleep(uint32_t usec)
{
	waitq_t wq;

	waitq_initialize(&wq);

	(void) waitq_sleep_timeout(&wq, usec);
}

/** Allow other threads to run. */
void thread_yield(void)
{
	assert(THREAD != NULL);
	scheduler_enter(Running);
}

static void thread_print(thread_t *thread, bool additional)
{
	uint64_t ucycles, kcycles;
	char usuffix, ksuffix;
	order_suffix(atomic_time_read(&thread->ucycles), &ucycles, &usuffix);
	order_suffix(atomic_time_read(&thread->kcycles), &kcycles, &ksuffix);

	state_t state = atomic_get_unordered(&thread->state);

	char *name;
	if (str_cmp(thread->name, "uinit") == 0)
		name = thread->task->name;
	else
		name = thread->name;

	if (additional)
		printf("%-8" PRIu64 " %p %p %9" PRIu64 "%c %9" PRIu64 "%c ",
		    thread->tid, thread->thread_code, thread->kstack,
		    ucycles, usuffix, kcycles, ksuffix);
	else
		printf("%-8" PRIu64 " %-14s %p %-8s %p %-5" PRIu32 "\n",
		    thread->tid, name, thread, thread_states[state],
		    thread->task, thread->task->container);

	if (additional) {
		cpu_t *cpu = atomic_get_unordered(&thread->cpu);
		if (cpu)
			printf("%-5u", cpu->id);
		else
			printf("none ");

		if (state == Sleeping) {
			printf(" %p", thread->sleep_queue);
		}

		printf("\n");
	}
}

/** Print list of threads debug info
 *
 * @param additional Print additional information.
 *
 */
void thread_print_list(bool additional)
{
	thread_t *thread;

	/* Accessing system-wide threads list through thread_first()/thread_next(). */
	irq_spinlock_lock(&threads_lock, true);

	if (sizeof(void *) <= 4) {
		if (additional)
			printf("[id    ] [code    ] [stack   ] [ucycles ] [kcycles ]"
			    " [cpu] [waitqueue]\n");
		else
			printf("[id    ] [name        ] [address ] [state ] [task    ]"
			    " [ctn]\n");
	} else {
		if (additional) {
			printf("[id    ] [code            ] [stack           ] [ucycles ] [kcycles ]"
			    " [cpu] [waitqueue       ]\n");
		} else
			printf("[id    ] [name        ] [address         ] [state ]"
			    " [task            ] [ctn]\n");
	}

	thread = thread_first();
	while (thread != NULL) {
		thread_print(thread, additional);
		thread = thread_next(thread);
	}

	irq_spinlock_unlock(&threads_lock, true);
}

static bool thread_exists(thread_t *thread)
{
	odlink_t *odlink = odict_find_eq(&threads, thread, NULL);
	return odlink != NULL;
}

/** Check whether the thread exists, and if so, return a reference to it.
 */
thread_t *thread_try_get(thread_t *thread)
{
	irq_spinlock_lock(&threads_lock, true);

	if (thread_exists(thread)) {
		/* Try to strengthen the reference. */
		thread = thread_try_ref(thread);
	} else {
		thread = NULL;
	}

	irq_spinlock_unlock(&threads_lock, true);

	return thread;
}

/** Update accounting of current thread.
 *
 * Note that thread_lock on THREAD must be already held and
 * interrupts must be already disabled.
 *
 * @param user True to update user accounting, false for kernel.
 *
 */
void thread_update_accounting(bool user)
{
	assert(interrupts_disabled());

	uint64_t time = get_cycle();

	if (user)
		atomic_time_increment(&THREAD->ucycles, time - THREAD->last_cycle);
	else
		atomic_time_increment(&THREAD->kcycles, time - THREAD->last_cycle);

	THREAD->last_cycle = time;
}

/** Find thread structure corresponding to thread ID.
 *
 * The threads_lock must be already held by the caller of this function and
 * interrupts must be disabled.
 *
 * The returned reference is weak.
 * If the caller needs to keep it, thread_try_ref() must be used to upgrade
 * to a strong reference _before_ threads_lock is released.
 *
 * @param id Thread ID.
 *
 * @return Thread structure address or NULL if there is no such thread ID.
 *
 */
thread_t *thread_find_by_id(thread_id_t thread_id)
{
	thread_t *thread;

	assert(interrupts_disabled());
	assert(irq_spinlock_locked(&threads_lock));

	thread = thread_first();
	while (thread != NULL) {
		if (thread->tid == thread_id)
			return thread;

		thread = thread_next(thread);
	}

	return NULL;
}

/** Get count of threads.
 *
 * @return Number of threads in the system
 */
size_t thread_count(void)
{
	assert(interrupts_disabled());
	assert(irq_spinlock_locked(&threads_lock));

	return odict_count(&threads);
}

/** Get first thread.
 *
 * @return Pointer to first thread or @c NULL if there are none.
 */
thread_t *thread_first(void)
{
	odlink_t *odlink;

	assert(interrupts_disabled());
	assert(irq_spinlock_locked(&threads_lock));

	odlink = odict_first(&threads);
	if (odlink == NULL)
		return NULL;

	return odict_get_instance(odlink, thread_t, lthreads);
}

/** Get next thread.
 *
 * @param cur Current thread
 * @return Pointer to next thread or @c NULL if there are no more threads.
 */
thread_t *thread_next(thread_t *cur)
{
	odlink_t *odlink;

	assert(interrupts_disabled());
	assert(irq_spinlock_locked(&threads_lock));

	odlink = odict_next(&cur->lthreads, &threads);
	if (odlink == NULL)
		return NULL;

	return odict_get_instance(odlink, thread_t, lthreads);
}

#ifdef CONFIG_UDEBUG

void thread_stack_trace(thread_id_t thread_id)
{
	irq_spinlock_lock(&threads_lock, true);
	thread_t *thread = thread_try_ref(thread_find_by_id(thread_id));
	irq_spinlock_unlock(&threads_lock, true);

	if (thread == NULL) {
		printf("No such thread.\n");
		return;
	}

	/*
	 * Schedule a stack trace to be printed
	 * just before the thread is scheduled next.
	 *
	 * If the thread is sleeping then try to interrupt
	 * the sleep. Any request for printing an uspace stack
	 * trace from within the kernel should be always
	 * considered a last resort debugging means, therefore
	 * forcing the thread's sleep to be interrupted
	 * is probably justifiable.
	 */

	printf("Scheduling thread stack trace.\n");
	atomic_set_unordered(&thread->btrace, true);

	thread_wakeup(thread);
	thread_put(thread);
}

#endif /* CONFIG_UDEBUG */

/** Get key function for the @c threads ordered dictionary.
 *
 * @param odlink Link
 * @return Pointer to thread structure cast as 'void *'
 */
static void *threads_getkey(odlink_t *odlink)
{
	thread_t *thread = odict_get_instance(odlink, thread_t, lthreads);
	return (void *) thread;
}

/** Key comparison function for the @c threads ordered dictionary.
 *
 * @param a Pointer to thread A
 * @param b Pointer to thread B
 * @return -1, 0, 1 iff pointer to A is less than, equal to, greater than B
 */
static int threads_cmp(void *a, void *b)
{
	if (a > b)
		return -1;
	else if (a == b)
		return 0;
	else
		return +1;
}

/** Process syscall to create new thread.
 * The started thread will have initial pc and sp set to the exact values passed
 * to the syscall. The kernel will not touch any stack data below the stack
 * pointer, but some architectures may require some space to be available
 * for use above it. See userspace() in kernel, and <libarch/thread.h> in libc.
 *
 */
sys_errno_t sys_thread_create(sysarg_t pc, sysarg_t sp,
    uspace_ptr_char uspace_name, size_t name_len)
{
	if (name_len > THREAD_NAME_BUFLEN - 1)
		name_len = THREAD_NAME_BUFLEN - 1;

	char namebuf[THREAD_NAME_BUFLEN];
	errno_t rc = copy_from_uspace(namebuf, uspace_name, name_len);
	if (rc != EOK)
		return (sys_errno_t) rc;

	namebuf[name_len] = 0;

	/*
	 * In case of failure, kernel_uarg will be deallocated in this function.
	 * In case of success, kernel_uarg will be freed in uinit().
	 */
	uinit_arg_t *kernel_uarg = malloc(sizeof(uinit_arg_t));
	if (!kernel_uarg)
		return (sys_errno_t) ENOMEM;

	kernel_uarg->pc = pc;
	kernel_uarg->sp = sp;

	// TODO: fix some unnecessary inconsistencies between architectures

	thread_t *thread = thread_create(uinit, kernel_uarg, TASK,
	    THREAD_FLAG_USPACE | THREAD_FLAG_NOATTACH, namebuf);
	if (!thread) {
		free(kernel_uarg);
		return (sys_errno_t) ENOMEM;
	}

#ifdef CONFIG_UDEBUG
	/*
	 * Generate udebug THREAD_B event and attach the thread.
	 * This must be done atomically (with the debug locks held),
	 * otherwise we would either miss some thread or receive
	 * THREAD_B events for threads that already existed
	 * and could be detected with THREAD_READ before.
	 */
	udebug_thread_b_event_attach(thread, TASK);
#else
	thread_attach(thread, TASK);
#endif
	thread_start(thread);
	thread_put(thread);

	return (sys_errno_t) EOK;
}

/** Process syscall to terminate thread.
 *
 */
sys_errno_t sys_thread_exit(int uspace_status)
{
	thread_exit();
}

/** Syscall for getting TID.
 *
 * @param uspace_thread_id Userspace address of 8-byte buffer where to store
 * current thread ID.
 *
 * @return 0 on success or an error code from @ref errno.h.
 *
 */
sys_errno_t sys_thread_get_id(uspace_ptr_thread_id_t uspace_thread_id)
{
	/*
	 * No need to acquire lock on THREAD because tid
	 * remains constant for the lifespan of the thread.
	 *
	 */
	return (sys_errno_t) copy_to_uspace(uspace_thread_id, &THREAD->tid,
	    sizeof(THREAD->tid));
}

/** Syscall wrapper for sleeping. */
sys_errno_t sys_thread_usleep(uint32_t usec)
{
	thread_usleep(usec);
	return 0;
}

sys_errno_t sys_thread_udelay(uint32_t usec)
{
	delay(usec);
	return 0;
}

/** @}
 */
