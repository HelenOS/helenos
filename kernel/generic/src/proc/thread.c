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
#include <synch/workqueue.h>
#include <synch/rcu.h>
#include <cpu.h>
#include <str.h>
#include <context.h>
#include <adt/avl.h>
#include <adt/list.h>
#include <time/clock.h>
#include <time/timeout.h>
#include <time/delay.h>
#include <config.h>
#include <arch/interrupt.h>
#include <smp/ipi.h>
#include <arch/faddr.h>
#include <atomic.h>
#include <mem.h>
#include <print.h>
#include <mm/slab.h>
#include <main/uinit.h>
#include <syscall/copy.h>
#include <errno.h>

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

typedef struct {
	thread_id_t thread_id;
	thread_t *thread;
} thread_iterator_t;

/** Lock protecting the threads_tree AVL tree.
 *
 * For locking rules, see declaration thereof.
 *
 */
IRQ_SPINLOCK_INITIALIZE(threads_lock);

/** AVL tree of all threads.
 *
 * When a thread is found in the threads_tree AVL tree, it is guaranteed to
 * exist as long as the threads_lock is held.
 *
 */
avltree_t threads_tree;

IRQ_SPINLOCK_STATIC_INITIALIZE(tidlock);
static thread_id_t last_tid = 0;

static slab_cache_t *thread_cache;

#ifdef CONFIG_FPU
slab_cache_t *fpu_context_cache;
#endif

/** Thread wrapper.
 *
 * This wrapper is provided to ensure that every thread makes a call to
 * thread_exit() when its implementing function returns.
 *
 * interrupts_disable() is assumed.
 *
 */
static void cushion(void)
{
	void (*f)(void *) = THREAD->thread_code;
	void *arg = THREAD->thread_arg;
	THREAD->last_cycle = get_cycle();

	/* This is where each thread wakes up after its creation */
	irq_spinlock_unlock(&THREAD->lock, false);
	interrupts_enable();

	f(arg);

	/* Accumulate accounting to the task */
	irq_spinlock_lock(&THREAD->lock, true);
	if (!THREAD->uncounted) {
		thread_update_accounting(true);
		uint64_t ucycles = THREAD->ucycles;
		THREAD->ucycles = 0;
		uint64_t kcycles = THREAD->kcycles;
		THREAD->kcycles = 0;

		irq_spinlock_pass(&THREAD->lock, &TASK->lock);
		TASK->ucycles += ucycles;
		TASK->kcycles += kcycles;
		irq_spinlock_unlock(&TASK->lock, true);
	} else
		irq_spinlock_unlock(&THREAD->lock, true);

	thread_exit();

	/* Not reached */
}

/** Initialization and allocation for thread_t structure
 *
 */
static errno_t thr_constructor(void *obj, unsigned int kmflags)
{
	thread_t *thread = (thread_t *) obj;

	irq_spinlock_initialize(&thread->lock, "thread_t_lock");
	link_initialize(&thread->rq_link);
	link_initialize(&thread->wq_link);
	link_initialize(&thread->th_link);

	/* call the architecture-specific part of the constructor */
	thr_constructor_arch(thread);

#ifdef CONFIG_FPU
#ifdef CONFIG_FPU_LAZY
	thread->saved_fpu_context = NULL;
#else /* CONFIG_FPU_LAZY */
	thread->saved_fpu_context = slab_alloc(fpu_context_cache, kmflags);
	if (!thread->saved_fpu_context)
		return ENOMEM;
#endif /* CONFIG_FPU_LAZY */
#endif /* CONFIG_FPU */

	/*
	 * Allocate the kernel stack from the low-memory to prevent an infinite
	 * nesting of TLB-misses when accessing the stack from the part of the
	 * TLB-miss handler written in C.
	 *
	 * Note that low-memory is safe to be used for the stack as it will be
	 * covered by the kernel identity mapping, which guarantees not to
	 * nest TLB-misses infinitely (either via some hardware mechanism or
	 * by the construciton of the assembly-language part of the TLB-miss
	 * handler).
	 *
	 * This restriction can be lifted once each architecture provides
	 * a similar guarantee, for example by locking the kernel stack
	 * in the TLB whenever it is allocated from the high-memory and the
	 * thread is being scheduled to run.
	 */
	kmflags |= FRAME_LOWMEM;
	kmflags &= ~FRAME_HIGHMEM;

	uintptr_t stack_phys =
	    frame_alloc(STACK_FRAMES, kmflags, STACK_SIZE - 1);
	if (!stack_phys) {
#ifdef CONFIG_FPU
		if (thread->saved_fpu_context)
			slab_free(fpu_context_cache, thread->saved_fpu_context);
#endif
		return ENOMEM;
	}

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

#ifdef CONFIG_FPU
	if (thread->saved_fpu_context)
		slab_free(fpu_context_cache, thread->saved_fpu_context);
#endif

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

	atomic_set(&nrdy, 0);
	thread_cache = slab_cache_create("thread_t", sizeof(thread_t), 0,
	    thr_constructor, thr_destructor, 0);

#ifdef CONFIG_FPU
	fpu_context_cache = slab_cache_create("fpu_context_t",
	    sizeof(fpu_context_t), FPU_CONTEXT_ALIGN, NULL, NULL, 0);
#endif

	avltree_create(&threads_tree);
}

/** Wire thread to the given CPU
 *
 * @param cpu CPU to wire the thread to.
 *
 */
void thread_wire(thread_t *thread, cpu_t *cpu)
{
	irq_spinlock_lock(&thread->lock, true);
	thread->cpu = cpu;
	thread->wired = true;
	irq_spinlock_unlock(&thread->lock, true);
}

/** Invoked right before thread_ready() readies the thread. thread is locked. */
static void before_thread_is_ready(thread_t *thread)
{
	assert(irq_spinlock_locked(&thread->lock));
	workq_before_thread_is_ready(thread);
}

/** Make thread ready
 *
 * Switch thread to the ready state.
 *
 * @param thread Thread to make ready.
 *
 */
void thread_ready(thread_t *thread)
{
	irq_spinlock_lock(&thread->lock, true);

	assert(thread->state != Ready);

	before_thread_is_ready(thread);

	int i = (thread->priority < RQ_COUNT - 1) ?
	    ++thread->priority : thread->priority;

	cpu_t *cpu;
	if (thread->wired || thread->nomigrate || thread->fpu_context_engaged) {
		/* Cannot ready to another CPU */
		assert(thread->cpu != NULL);
		cpu = thread->cpu;
	} else if (thread->stolen) {
		/* Ready to the stealing CPU */
		cpu = CPU;
	} else if (thread->cpu) {
		/* Prefer the CPU on which the thread ran last */
		assert(thread->cpu != NULL);
		cpu = thread->cpu;
	} else {
		cpu = CPU;
	}

	thread->state = Ready;

	irq_spinlock_pass(&thread->lock, &(cpu->rq[i].lock));

	/*
	 * Append thread to respective ready queue
	 * on respective processor.
	 */

	list_append(&thread->rq_link, &cpu->rq[i].rq);
	cpu->rq[i].n++;
	irq_spinlock_unlock(&(cpu->rq[i].lock), true);

	atomic_inc(&nrdy);
	atomic_inc(&cpu->nrdy);
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
thread_t *thread_create(void (* func)(void *), void *arg, task_t *task,
    thread_flags_t flags, const char *name)
{
	thread_t *thread = (thread_t *) slab_alloc(thread_cache, 0);
	if (!thread)
		return NULL;

	/* Not needed, but good for debugging */
	memsetb(thread->kstack, STACK_SIZE, 0);

	irq_spinlock_lock(&tidlock, true);
	thread->tid = ++last_tid;
	irq_spinlock_unlock(&tidlock, true);

	context_save(&thread->saved_context);
	context_set(&thread->saved_context, FADDR(cushion),
	    (uintptr_t) thread->kstack, STACK_SIZE);

	the_initialize((the_t *) thread->kstack);

	ipl_t ipl = interrupts_disable();
	thread->saved_context.ipl = interrupts_read();
	interrupts_restore(ipl);

	str_cpy(thread->name, THREAD_NAME_BUFLEN, name);

	thread->thread_code = func;
	thread->thread_arg = arg;
	thread->ticks = -1;
	thread->ucycles = 0;
	thread->kcycles = 0;
	thread->uncounted =
	    ((flags & THREAD_FLAG_UNCOUNTED) == THREAD_FLAG_UNCOUNTED);
	thread->priority = -1;          /* Start in rq[0] */
	thread->cpu = NULL;
	thread->wired = false;
	thread->stolen = false;
	thread->uspace =
	    ((flags & THREAD_FLAG_USPACE) == THREAD_FLAG_USPACE);

	thread->nomigrate = 0;
	thread->state = Entering;

	timeout_initialize(&thread->sleep_timeout);
	thread->sleep_interruptible = false;
	thread->sleep_queue = NULL;
	thread->timeout_pending = false;

	thread->in_copy_from_uspace = false;
	thread->in_copy_to_uspace = false;

	thread->interrupted = false;
	thread->detached = false;
	waitq_initialize(&thread->join_wq);

	thread->task = task;

	thread->workq = NULL;

	thread->fpu_context_exists = false;
	thread->fpu_context_engaged = false;

	avltree_node_initialize(&thread->threads_tree_node);
	thread->threads_tree_node.key = (uintptr_t) thread;

#ifdef CONFIG_UDEBUG
	/* Initialize debugging stuff */
	thread->btrace = false;
	udebug_thread_initialize(&thread->udebug);
#endif

	/* Might depend on previous initialization */
	thread_create_arch(thread);

	rcu_thread_init(thread);

	if ((flags & THREAD_FLAG_NOATTACH) != THREAD_FLAG_NOATTACH)
		thread_attach(thread, task);

	return thread;
}

/** Destroy thread memory structure
 *
 * Detach thread from all queues, cpus etc. and destroy it.
 *
 * @param thread  Thread to be destroyed.
 * @param irq_res Indicate whether it should unlock thread->lock
 *                in interrupts-restore mode.
 *
 */
void thread_destroy(thread_t *thread, bool irq_res)
{
	assert(irq_spinlock_locked(&thread->lock));
	assert((thread->state == Exiting) || (thread->state == Lingering));
	assert(thread->task);
	assert(thread->cpu);

	irq_spinlock_lock(&thread->cpu->lock, false);
	if (thread->cpu->fpu_owner == thread)
		thread->cpu->fpu_owner = NULL;
	irq_spinlock_unlock(&thread->cpu->lock, false);

	irq_spinlock_pass(&thread->lock, &threads_lock);

	avltree_delete(&threads_tree, &thread->threads_tree_node);

	irq_spinlock_pass(&threads_lock, &thread->task->lock);

	/*
	 * Detach from the containing task.
	 */
	list_remove(&thread->th_link);
	irq_spinlock_unlock(&thread->task->lock, irq_res);

	/*
	 * Drop the reference to the containing task.
	 */
	task_release(thread->task);
	slab_free(thread_cache, thread);
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
	/*
	 * Attach to the specified task.
	 */
	irq_spinlock_lock(&task->lock, true);

	/* Hold a reference to the task. */
	task_hold(task);

	/* Must not count kbox thread into lifecount */
	if (thread->uspace)
		atomic_inc(&task->lifecount);

	list_append(&thread->th_link, &task->threads);

	irq_spinlock_pass(&task->lock, &threads_lock);

	/*
	 * Register this thread in the system-wide list.
	 */
	avltree_insert(&threads_tree, &thread->threads_tree_node);
	irq_spinlock_unlock(&threads_lock, true);
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
			futex_task_cleanup();
			LOG("Cleanup of task %" PRIu64" completed.", TASK->taskid);
		}
	}

restart:
	irq_spinlock_lock(&THREAD->lock, true);
	if (THREAD->timeout_pending) {
		/* Busy waiting for timeouts in progress */
		irq_spinlock_unlock(&THREAD->lock, true);
		goto restart;
	}

	THREAD->state = Exiting;
	irq_spinlock_unlock(&THREAD->lock, true);

	scheduler();

	/* Not reached */
	while (true);
}

/** Interrupts an existing thread so that it may exit as soon as possible.
 *
 * Threads that are blocked waiting for a synchronization primitive
 * are woken up with a return code of EINTR if the
 * blocking call was interruptable. See waitq_sleep_timeout().
 *
 * The caller must guarantee the thread object is valid during the entire
 * function, eg by holding the threads_lock lock.
 *
 * Interrupted threads automatically exit when returning back to user space.
 *
 * @param thread A valid thread object. The caller must guarantee it
 *               will remain valid until thread_interrupt() exits.
 */
void thread_interrupt(thread_t *thread)
{
	assert(thread != NULL);

	irq_spinlock_lock(&thread->lock, true);

	thread->interrupted = true;
	bool sleeping = (thread->state == Sleeping);

	irq_spinlock_unlock(&thread->lock, true);

	if (sleeping)
		waitq_interrupt_sleep(thread);
}

/** Returns true if the thread was interrupted.
 *
 * @param thread A valid thread object. User must guarantee it will
 *               be alive during the entire call.
 * @return true if the thread was already interrupted via thread_interrupt().
 */
bool thread_interrupted(thread_t *thread)
{
	assert(thread != NULL);

	bool interrupted;

	irq_spinlock_lock(&thread->lock, true);
	interrupted = thread->interrupted;
	irq_spinlock_unlock(&thread->lock, true);

	return interrupted;
}

/** Prevent the current thread from being migrated to another processor. */
void thread_migration_disable(void)
{
	assert(THREAD);

	THREAD->nomigrate++;
}

/** Allow the current thread to be migrated to another processor. */
void thread_migration_enable(void)
{
	assert(THREAD);
	assert(THREAD->nomigrate > 0);

	if (THREAD->nomigrate > 0)
		THREAD->nomigrate--;
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
	/* Sleep in 1000 second steps to support
	   full argument range */
	while (sec > 0) {
		uint32_t period = (sec > 1000) ? 1000 : sec;

		thread_usleep(period * 1000000);
		sec -= period;
	}
}

/** Wait for another thread to exit.
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
	if (thread == THREAD)
		return EINVAL;

	/*
	 * Since thread join can only be called once on an undetached thread,
	 * the thread pointer is guaranteed to be still valid.
	 */

	irq_spinlock_lock(&thread->lock, true);
	assert(!thread->detached);
	irq_spinlock_unlock(&thread->lock, true);

	return waitq_sleep_timeout(&thread->join_wq, usec, flags, NULL);
}

/** Detach thread.
 *
 * Mark the thread as detached. If the thread is already
 * in the Lingering state, deallocate its resources.
 *
 * @param thread Thread to be detached.
 *
 */
void thread_detach(thread_t *thread)
{
	/*
	 * Since the thread is expected not to be already detached,
	 * pointer to it must be still valid.
	 */
	irq_spinlock_lock(&thread->lock, true);
	assert(!thread->detached);

	if (thread->state == Lingering) {
		/*
		 * Unlock &thread->lock and restore
		 * interrupts in thread_destroy().
		 */
		thread_destroy(thread, true);
		return;
	} else {
		thread->detached = true;
	}

	irq_spinlock_unlock(&thread->lock, true);
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

	(void) waitq_sleep_timeout(&wq, usec, SYNCH_FLAGS_NON_BLOCKING, NULL);
}

static bool thread_walker(avltree_node_t *node, void *arg)
{
	bool *additional = (bool *) arg;
	thread_t *thread = avltree_get_instance(node, thread_t, threads_tree_node);

	uint64_t ucycles, kcycles;
	char usuffix, ksuffix;
	order_suffix(thread->ucycles, &ucycles, &usuffix);
	order_suffix(thread->kcycles, &kcycles, &ksuffix);

	char *name;
	if (str_cmp(thread->name, "uinit") == 0)
		name = thread->task->name;
	else
		name = thread->name;

#ifdef __32_BITS__
	if (*additional)
		printf("%-8" PRIu64 " %10p %10p %9" PRIu64 "%c %9" PRIu64 "%c ",
		    thread->tid, thread->thread_code, thread->kstack,
		    ucycles, usuffix, kcycles, ksuffix);
	else
		printf("%-8" PRIu64 " %-14s %10p %-8s %10p %-5" PRIu32 "\n",
		    thread->tid, name, thread, thread_states[thread->state],
		    thread->task, thread->task->container);
#endif

#ifdef __64_BITS__
	if (*additional)
		printf("%-8" PRIu64 " %18p %18p\n"
		    "         %9" PRIu64 "%c %9" PRIu64 "%c ",
		    thread->tid, thread->thread_code, thread->kstack,
		    ucycles, usuffix, kcycles, ksuffix);
	else
		printf("%-8" PRIu64 " %-14s %18p %-8s %18p %-5" PRIu32 "\n",
		    thread->tid, name, thread, thread_states[thread->state],
		    thread->task, thread->task->container);
#endif

	if (*additional) {
		if (thread->cpu)
			printf("%-5u", thread->cpu->id);
		else
			printf("none ");

		if (thread->state == Sleeping) {
#ifdef __32_BITS__
			printf(" %10p", thread->sleep_queue);
#endif

#ifdef __64_BITS__
			printf(" %18p", thread->sleep_queue);
#endif
		}

		printf("\n");
	}

	return true;
}

/** Print list of threads debug info
 *
 * @param additional Print additional information.
 *
 */
void thread_print_list(bool additional)
{
	/* Messing with thread structures, avoid deadlock */
	irq_spinlock_lock(&threads_lock, true);

#ifdef __32_BITS__
	if (additional)
		printf("[id    ] [code    ] [stack   ] [ucycles ] [kcycles ]"
		    " [cpu] [waitqueue]\n");
	else
		printf("[id    ] [name        ] [address ] [state ] [task    ]"
		    " [ctn]\n");
#endif

#ifdef __64_BITS__
	if (additional) {
		printf("[id    ] [code            ] [stack           ]\n"
		    "         [ucycles ] [kcycles ] [cpu] [waitqueue       ]\n");
	} else
		printf("[id    ] [name        ] [address         ] [state ]"
		    " [task            ] [ctn]\n");
#endif

	avltree_walk(&threads_tree, thread_walker, &additional);

	irq_spinlock_unlock(&threads_lock, true);
}

/** Check whether thread exists.
 *
 * Note that threads_lock must be already held and
 * interrupts must be already disabled.
 *
 * @param thread Pointer to thread.
 *
 * @return True if thread t is known to the system, false otherwise.
 *
 */
bool thread_exists(thread_t *thread)
{
	assert(interrupts_disabled());
	assert(irq_spinlock_locked(&threads_lock));

	avltree_node_t *node =
	    avltree_search(&threads_tree, (avltree_key_t) ((uintptr_t) thread));

	return node != NULL;
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
	uint64_t time = get_cycle();

	assert(interrupts_disabled());
	assert(irq_spinlock_locked(&THREAD->lock));

	if (user)
		THREAD->ucycles += time - THREAD->last_cycle;
	else
		THREAD->kcycles += time - THREAD->last_cycle;

	THREAD->last_cycle = time;
}

static bool thread_search_walker(avltree_node_t *node, void *arg)
{
	thread_t *thread =
	    (thread_t *) avltree_get_instance(node, thread_t, threads_tree_node);
	thread_iterator_t *iterator = (thread_iterator_t *) arg;

	if (thread->tid == iterator->thread_id) {
		iterator->thread = thread;
		return false;
	}

	return true;
}

/** Find thread structure corresponding to thread ID.
 *
 * The threads_lock must be already held by the caller of this function and
 * interrupts must be disabled.
 *
 * @param id Thread ID.
 *
 * @return Thread structure address or NULL if there is no such thread ID.
 *
 */
thread_t *thread_find_by_id(thread_id_t thread_id)
{
	assert(interrupts_disabled());
	assert(irq_spinlock_locked(&threads_lock));

	thread_iterator_t iterator;

	iterator.thread_id = thread_id;
	iterator.thread = NULL;

	avltree_walk(&threads_tree, thread_search_walker, (void *) &iterator);

	return iterator.thread;
}

#ifdef CONFIG_UDEBUG

void thread_stack_trace(thread_id_t thread_id)
{
	irq_spinlock_lock(&threads_lock, true);

	thread_t *thread = thread_find_by_id(thread_id);
	if (thread == NULL) {
		printf("No such thread.\n");
		irq_spinlock_unlock(&threads_lock, true);
		return;
	}

	irq_spinlock_lock(&thread->lock, false);

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

	bool sleeping = false;
	istate_t *istate = thread->udebug.uspace_state;
	if (istate != NULL) {
		printf("Scheduling thread stack trace.\n");
		thread->btrace = true;
		if (thread->state == Sleeping)
			sleeping = true;
	} else
		printf("Thread interrupt state not available.\n");

	irq_spinlock_unlock(&thread->lock, false);

	if (sleeping)
		waitq_interrupt_sleep(thread);

	irq_spinlock_unlock(&threads_lock, true);
}

#endif /* CONFIG_UDEBUG */

/** Process syscall to create new thread.
 *
 */
sys_errno_t sys_thread_create(uspace_arg_t *uspace_uarg, char *uspace_name,
    size_t name_len, thread_id_t *uspace_thread_id)
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
	uspace_arg_t *kernel_uarg =
	    (uspace_arg_t *) malloc(sizeof(uspace_arg_t), 0);

	rc = copy_from_uspace(kernel_uarg, uspace_uarg, sizeof(uspace_arg_t));
	if (rc != EOK) {
		free(kernel_uarg);
		return (sys_errno_t) rc;
	}

	thread_t *thread = thread_create(uinit, kernel_uarg, TASK,
	    THREAD_FLAG_USPACE | THREAD_FLAG_NOATTACH, namebuf);
	if (thread) {
		if (uspace_thread_id != NULL) {
			rc = copy_to_uspace(uspace_thread_id, &thread->tid,
			    sizeof(thread->tid));
			if (rc != EOK) {
				/*
				 * We have encountered a failure, but the thread
				 * has already been created. We need to undo its
				 * creation now.
				 */

				/*
				 * The new thread structure is initialized, but
				 * is still not visible to the system.
				 * We can safely deallocate it.
				 */
				slab_free(thread_cache, thread);
				free(kernel_uarg);

				return (sys_errno_t) rc;
			 }
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
		thread_ready(thread);

		return 0;
	} else
		free(kernel_uarg);

	return (sys_errno_t) ENOMEM;
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
sys_errno_t sys_thread_get_id(thread_id_t *uspace_thread_id)
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
