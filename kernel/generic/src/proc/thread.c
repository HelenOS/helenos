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

/**
 * @file
 * @brief	Thread management functions.
 */

#include <proc/scheduler.h>
#include <proc/thread.h>
#include <proc/task.h>
#include <proc/uarg.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <arch/asm.h>
#include <arch/cycle.h>
#include <arch.h>
#include <synch/synch.h>
#include <synch/spinlock.h>
#include <synch/waitq.h>
#include <synch/rwlock.h>
#include <cpu.h>
#include <func.h>
#include <context.h>
#include <adt/avl.h>
#include <adt/list.h>
#include <time/clock.h>
#include <time/timeout.h>
#include <config.h>
#include <arch/interrupt.h>
#include <smp/ipi.h>
#include <arch/faddr.h>
#include <atomic.h>
#include <memstr.h>
#include <print.h>
#include <mm/slab.h>
#include <debug.h>
#include <main/uinit.h>
#include <syscall/copy.h>
#include <errno.h>
#include <console/klog.h>


/** Thread states */
char *thread_states[] = {
	"Invalid",
	"Running",
	"Sleeping",
	"Ready",
	"Entering",
	"Exiting",
	"Lingering"
}; 

/** Lock protecting the threads_tree AVL tree.
 *
 * For locking rules, see declaration thereof.
 */
SPINLOCK_INITIALIZE(threads_lock);

/** AVL tree of all threads.
 *
 * When a thread is found in the threads_tree AVL tree, it is guaranteed to
 * exist as long as the threads_lock is held.
 */
avltree_t threads_tree;		

SPINLOCK_INITIALIZE(tidlock);
thread_id_t last_tid = 0;

static slab_cache_t *thread_slab;
#ifdef ARCH_HAS_FPU
slab_cache_t *fpu_context_slab;
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
	spinlock_unlock(&THREAD->lock);
	interrupts_enable();

	f(arg);
	
	/* Accumulate accounting to the task */
	ipl_t ipl = interrupts_disable();
	
	spinlock_lock(&THREAD->lock);
	if (!THREAD->uncounted) {
		thread_update_accounting();
		uint64_t cycles = THREAD->cycles;
		THREAD->cycles = 0;
		spinlock_unlock(&THREAD->lock);
		
		spinlock_lock(&TASK->lock);
		TASK->cycles += cycles;
		spinlock_unlock(&TASK->lock);
	} else
		spinlock_unlock(&THREAD->lock);
	
	interrupts_restore(ipl);
	
	thread_exit();
	/* not reached */
}

/** Initialization and allocation for thread_t structure */
static int thr_constructor(void *obj, int kmflags)
{
	thread_t *t = (thread_t *) obj;

	spinlock_initialize(&t->lock, "thread_t_lock");
	link_initialize(&t->rq_link);
	link_initialize(&t->wq_link);
	link_initialize(&t->th_link);

	/* call the architecture-specific part of the constructor */
	thr_constructor_arch(t);
	
#ifdef ARCH_HAS_FPU
#ifdef CONFIG_FPU_LAZY
	t->saved_fpu_context = NULL;
#else
	t->saved_fpu_context = slab_alloc(fpu_context_slab, kmflags);
	if (!t->saved_fpu_context)
		return -1;
#endif
#endif	

	t->kstack = (uint8_t *) frame_alloc(STACK_FRAMES, FRAME_KA | kmflags);
	if (!t->kstack) {
#ifdef ARCH_HAS_FPU
		if (t->saved_fpu_context)
			slab_free(fpu_context_slab, t->saved_fpu_context);
#endif
		return -1;
	}

	return 0;
}

/** Destruction of thread_t object */
static int thr_destructor(void *obj)
{
	thread_t *t = (thread_t *) obj;

	/* call the architecture-specific part of the destructor */
	thr_destructor_arch(t);

	frame_free(KA2PA(t->kstack));
#ifdef ARCH_HAS_FPU
	if (t->saved_fpu_context)
		slab_free(fpu_context_slab, t->saved_fpu_context);
#endif
	return 1; /* One page freed */
}

/** Initialize threads
 *
 * Initialize kernel threads support.
 *
 */
void thread_init(void)
{
	THREAD = NULL;
	atomic_set(&nrdy,0);
	thread_slab = slab_cache_create("thread_slab", sizeof(thread_t), 0,
	    thr_constructor, thr_destructor, 0);

#ifdef ARCH_HAS_FPU
	fpu_context_slab = slab_cache_create("fpu_slab", sizeof(fpu_context_t),
	    FPU_CONTEXT_ALIGN, NULL, NULL, 0);
#endif

	avltree_create(&threads_tree);
}

/** Make thread ready
 *
 * Switch thread t to the ready state.
 *
 * @param t Thread to make ready.
 *
 */
void thread_ready(thread_t *t)
{
	cpu_t *cpu;
	runq_t *r;
	ipl_t ipl;
	int i, avg;

	ipl = interrupts_disable();

	spinlock_lock(&t->lock);

	ASSERT(!(t->state == Ready));

	i = (t->priority < RQ_COUNT - 1) ? ++t->priority : t->priority;
	
	cpu = CPU;
	if (t->flags & THREAD_FLAG_WIRED) {
		ASSERT(t->cpu != NULL);
		cpu = t->cpu;
	}
	t->state = Ready;
	spinlock_unlock(&t->lock);
	
	/*
	 * Append t to respective ready queue on respective processor.
	 */
	r = &cpu->rq[i];
	spinlock_lock(&r->lock);
	list_append(&t->rq_link, &r->rq_head);
	r->n++;
	spinlock_unlock(&r->lock);

	atomic_inc(&nrdy);
	avg = atomic_get(&nrdy) / config.cpu_active;
	atomic_inc(&cpu->nrdy);

	interrupts_restore(ipl);
}

/** Create new thread
 *
 * Create a new thread.
 *
 * @param func		Thread's implementing function.
 * @param arg		Thread's implementing function argument.
 * @param task		Task to which the thread belongs. The caller must
 * 			guarantee that the task won't cease to exist during the
 * 			call. The task's lock may not be held.
 * @param flags		Thread flags.
 * @param name		Symbolic name.
 * @param uncounted	Thread's accounting doesn't affect accumulated task
 * 			accounting.
 *
 * @return 		New thread's structure on success, NULL on failure.
 *
 */
thread_t *thread_create(void (* func)(void *), void *arg, task_t *task,
    int flags, char *name, bool uncounted)
{
	thread_t *t;
	ipl_t ipl;
	
	t = (thread_t *) slab_alloc(thread_slab, 0);
	if (!t)
		return NULL;
	
	/* Not needed, but good for debugging */
	memsetb((uintptr_t) t->kstack, THREAD_STACK_SIZE * 1 << STACK_FRAMES,
	    0);
	
	ipl = interrupts_disable();
	spinlock_lock(&tidlock);
	t->tid = ++last_tid;
	spinlock_unlock(&tidlock);
	interrupts_restore(ipl);
	
	context_save(&t->saved_context);
	context_set(&t->saved_context, FADDR(cushion), (uintptr_t) t->kstack,
	    THREAD_STACK_SIZE);
	
	the_initialize((the_t *) t->kstack);
	
	ipl = interrupts_disable();
	t->saved_context.ipl = interrupts_read();
	interrupts_restore(ipl);
	
	memcpy(t->name, name, THREAD_NAME_BUFLEN);
	
	t->thread_code = func;
	t->thread_arg = arg;
	t->ticks = -1;
	t->cycles = 0;
	t->uncounted = uncounted;
	t->priority = -1;		/* start in rq[0] */
	t->cpu = NULL;
	t->flags = flags;
	t->state = Entering;
	t->call_me = NULL;
	t->call_me_with = NULL;
	
	timeout_initialize(&t->sleep_timeout);
	t->sleep_interruptible = false;
	t->sleep_queue = NULL;
	t->timeout_pending = 0;

	t->in_copy_from_uspace = false;
	t->in_copy_to_uspace = false;

	t->interrupted = false;	
	t->detached = false;
	waitq_initialize(&t->join_wq);
	
	t->rwlock_holder_type = RWLOCK_NONE;
		
	t->task = task;
	
	t->fpu_context_exists = 0;
	t->fpu_context_engaged = 0;

	avltree_node_initialize(&t->threads_tree_node);
	t->threads_tree_node.key = (uintptr_t) t;

	/* might depend on previous initialization */
	thread_create_arch(t);	

	if (!(flags & THREAD_FLAG_NOATTACH))
		thread_attach(t, task);

	return t;
}

/** Destroy thread memory structure
 *
 * Detach thread from all queues, cpus etc. and destroy it.
 *
 * Assume thread->lock is held!!
 */
void thread_destroy(thread_t *t)
{
	ASSERT(t->state == Exiting || t->state == Lingering);
	ASSERT(t->task);
	ASSERT(t->cpu);

	spinlock_lock(&t->cpu->lock);
	if (t->cpu->fpu_owner == t)
		t->cpu->fpu_owner = NULL;
	spinlock_unlock(&t->cpu->lock);

	spinlock_unlock(&t->lock);

	spinlock_lock(&threads_lock);
	avltree_delete(&threads_tree, &t->threads_tree_node);
	spinlock_unlock(&threads_lock);

	/*
	 * Detach from the containing task.
	 */
	spinlock_lock(&t->task->lock);
	list_remove(&t->th_link);
	spinlock_unlock(&t->task->lock);	

	/*
	 * t is guaranteed to be the very last thread of its task.
	 * It is safe to destroy the task.
	 */
	if (atomic_predec(&t->task->refcount) == 0)
		task_destroy(t->task);
	
	slab_free(thread_slab, t);
}

/** Make the thread visible to the system.
 *
 * Attach the thread structure to the current task and make it visible in the
 * threads_tree.
 *
 * @param t	Thread to be attached to the task.
 * @param task	Task to which the thread is to be attached.
 */
void thread_attach(thread_t *t, task_t *task)
{
	ipl_t ipl;

	/*
	 * Attach to the current task.
	 */
	ipl = interrupts_disable();
	spinlock_lock(&task->lock);
	atomic_inc(&task->refcount);
	atomic_inc(&task->lifecount);
	list_append(&t->th_link, &task->th_head);
	spinlock_unlock(&task->lock);

	/*
	 * Register this thread in the system-wide list.
	 */
	spinlock_lock(&threads_lock);
	avltree_insert(&threads_tree, &t->threads_tree_node);
	spinlock_unlock(&threads_lock);
	
	interrupts_restore(ipl);
}

/** Terminate thread.
 *
 * End current thread execution and switch it to the exiting state. All pending
 * timeouts are executed.
 */
void thread_exit(void)
{
	ipl_t ipl;

	if (atomic_predec(&TASK->lifecount) == 0) {
		/*
		 * We are the last thread in the task that still has not exited.
		 * With the exception of the moment the task was created, new
		 * threads can only be created by threads of the same task.
		 * We are safe to perform cleanup.
		 */
		if (THREAD->flags & THREAD_FLAG_USPACE) {
			ipc_cleanup();
		        futex_cleanup();
			klog_printf("Cleanup of task %llu completed.",
			    TASK->taskid);
		}
	}

restart:
	ipl = interrupts_disable();
	spinlock_lock(&THREAD->lock);
	if (THREAD->timeout_pending) { 
		/* busy waiting for timeouts in progress */
		spinlock_unlock(&THREAD->lock);
		interrupts_restore(ipl);
		goto restart;
	}
	
	THREAD->state = Exiting;
	spinlock_unlock(&THREAD->lock);
	scheduler();

	/* Not reached */
	while (1)
		;
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
	thread_usleep(sec * 1000000);
}

/** Wait for another thread to exit.
 *
 * @param t Thread to join on exit.
 * @param usec Timeout in microseconds.
 * @param flags Mode of operation.
 *
 * @return An error code from errno.h or an error code from synch.h.
 */
int thread_join_timeout(thread_t *t, uint32_t usec, int flags)
{
	ipl_t ipl;
	int rc;

	if (t == THREAD)
		return EINVAL;

	/*
	 * Since thread join can only be called once on an undetached thread,
	 * the thread pointer is guaranteed to be still valid.
	 */
	
	ipl = interrupts_disable();
	spinlock_lock(&t->lock);
	ASSERT(!t->detached);
	spinlock_unlock(&t->lock);
	interrupts_restore(ipl);
	
	rc = waitq_sleep_timeout(&t->join_wq, usec, flags);
	
	return rc;	
}

/** Detach thread.
 *
 * Mark the thread as detached, if the thread is already in the Lingering
 * state, deallocate its resources.
 *
 * @param t Thread to be detached.
 */
void thread_detach(thread_t *t)
{
	ipl_t ipl;

	/*
	 * Since the thread is expected not to be already detached,
	 * pointer to it must be still valid.
	 */
	ipl = interrupts_disable();
	spinlock_lock(&t->lock);
	ASSERT(!t->detached);
	if (t->state == Lingering) {
		thread_destroy(t);	/* unlocks &t->lock */
		interrupts_restore(ipl);
		return;
	} else {
		t->detached = true;
	}
	spinlock_unlock(&t->lock);
	interrupts_restore(ipl);
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

	(void) waitq_sleep_timeout(&wq, usec, SYNCH_FLAGS_NON_BLOCKING);
}

/** Register thread out-of-context invocation
 *
 * Register a function and its argument to be executed
 * on next context switch to the current thread.
 *
 * @param call_me      Out-of-context function.
 * @param call_me_with Out-of-context function argument.
 *
 */
void thread_register_call_me(void (* call_me)(void *), void *call_me_with)
{
	ipl_t ipl;
	
	ipl = interrupts_disable();
	spinlock_lock(&THREAD->lock);
	THREAD->call_me = call_me;
	THREAD->call_me_with = call_me_with;
	spinlock_unlock(&THREAD->lock);
	interrupts_restore(ipl);
}

static bool thread_walker(avltree_node_t *node, void *arg)
{
	thread_t *t;
		
	t = avltree_get_instance(node, thread_t, threads_tree_node);

	uint64_t cycles;
	char suffix;
	order(t->cycles, &cycles, &suffix);
	
	if (sizeof(void *) == 4)
		printf("%-6llu %-10s %#10zx %-8s %#10zx %-3ld %#10zx %#10zx %9llu%c ",
		    t->tid, t->name, t, thread_states[t->state], t->task,
	    	t->task->context, t->thread_code, t->kstack, cycles, suffix);
	else
		printf("%-6llu %-10s %#18zx %-8s %#18zx %-3ld %#18zx %#18zx %9llu%c ",
		    t->tid, t->name, t, thread_states[t->state], t->task,
	    	t->task->context, t->thread_code, t->kstack, cycles, suffix);
			
	if (t->cpu)
		printf("%-4zd", t->cpu->id);
	else
		printf("none");
			
	if (t->state == Sleeping) {
		if (sizeof(uintptr_t) == 4)
			printf(" %#10zx", t->sleep_queue);
		else
			printf(" %#18zx", t->sleep_queue);
	}
			
	printf("\n");

	return true;
}

/** Print list of threads debug info */
void thread_print_list(void)
{
	ipl_t ipl;
	
	/* Messing with thread structures, avoid deadlock */
	ipl = interrupts_disable();
	spinlock_lock(&threads_lock);
	
	if (sizeof(uintptr_t) == 4) {
		printf("tid    name       address    state    task       "
			"ctx code       stack      cycles     cpu  "
			"waitqueue\n");
		printf("------ ---------- ---------- -------- ---------- "
			"--- ---------- ---------- ---------- ---- "
			"----------\n");
	} else {
		printf("tid    name       address            state    task               "
			"ctx code               stack              cycles     cpu  "
			"waitqueue\n");
		printf("------ ---------- ------------------ -------- ------------------ "
			"--- ------------------ ------------------ ---------- ---- "
			"------------------\n");
	}

	avltree_walk(&threads_tree, thread_walker, NULL);

	spinlock_unlock(&threads_lock);
	interrupts_restore(ipl);
}

/** Check whether thread exists.
 *
 * Note that threads_lock must be already held and
 * interrupts must be already disabled.
 *
 * @param t Pointer to thread.
 *
 * @return True if thread t is known to the system, false otherwise.
 */
bool thread_exists(thread_t *t)
{
	avltree_node_t *node;

	node = avltree_search(&threads_tree, (avltree_key_t) ((uintptr_t) t));
	
	return node != NULL;
}


/** Update accounting of current thread.
 *
 * Note that thread_lock on THREAD must be already held and
 * interrupts must be already disabled.
 *
 */
void thread_update_accounting(void)
{
	uint64_t time = get_cycle();
	THREAD->cycles += time - THREAD->last_cycle;
	THREAD->last_cycle = time;
}

/** Process syscall to create new thread.
 *
 */
unative_t sys_thread_create(uspace_arg_t *uspace_uarg, char *uspace_name,
    thread_id_t *uspace_thread_id)
{
	thread_t *t;
	char namebuf[THREAD_NAME_BUFLEN];
	uspace_arg_t *kernel_uarg;
	int rc;

	rc = copy_from_uspace(namebuf, uspace_name, THREAD_NAME_BUFLEN);
	if (rc != 0)
		return (unative_t) rc;

	/*
	 * In case of failure, kernel_uarg will be deallocated in this function.
	 * In case of success, kernel_uarg will be freed in uinit().
	 */
	kernel_uarg = (uspace_arg_t *) malloc(sizeof(uspace_arg_t), 0);
	
	rc = copy_from_uspace(kernel_uarg, uspace_uarg, sizeof(uspace_arg_t));
	if (rc != 0) {
		free(kernel_uarg);
		return (unative_t) rc;
	}

	t = thread_create(uinit, kernel_uarg, TASK,
	    THREAD_FLAG_USPACE | THREAD_FLAG_NOATTACH, namebuf, false);
	if (t) {
		if (uspace_thread_id != NULL) {
			int rc;

			rc = copy_to_uspace(uspace_thread_id, &t->tid,
			    sizeof(t->tid));
			if (rc != 0) {
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
				slab_free(thread_slab, t);
			 	free(kernel_uarg);

				return (unative_t) rc;
			 }
		}
		thread_attach(t, TASK);
		thread_ready(t);

		return 0;
	} else
		free(kernel_uarg);

	return (unative_t) ENOMEM;
}

/** Process syscall to terminate thread.
 *
 */
unative_t sys_thread_exit(int uspace_status)
{
	thread_exit();
	/* Unreachable */
	return 0;
}

/** Syscall for getting TID.
 *
 * @param uspace_thread_id Userspace address of 8-byte buffer where to store
 * current thread ID.
 *
 * @return 0 on success or an error code from @ref errno.h.
 */
unative_t sys_thread_get_id(thread_id_t *uspace_thread_id)
{
	/*
	 * No need to acquire lock on THREAD because tid
	 * remains constant for the lifespan of the thread.
	 */
	return (unative_t) copy_to_uspace(uspace_thread_id, &THREAD->tid,
	    sizeof(THREAD->tid));
}

/** @}
 */
