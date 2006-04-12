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

#include <proc/scheduler.h>
#include <proc/thread.h>
#include <proc/task.h>
#include <proc/uarg.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <arch/asm.h>
#include <arch.h>
#include <synch/synch.h>
#include <synch/spinlock.h>
#include <synch/waitq.h>
#include <synch/rwlock.h>
#include <cpu.h>
#include <func.h>
#include <context.h>
#include <adt/btree.h>
#include <adt/list.h>
#include <typedefs.h>
#include <time/clock.h>
#include <adt/list.h>
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

char *thread_states[] = {"Invalid", "Running", "Sleeping", "Ready", "Entering", "Exiting"}; /**< Thread states */

/** Lock protecting threads_head list. For locking rules, see declaration thereof. */
SPINLOCK_INITIALIZE(threads_lock);
btree_t threads_btree;			/**< B+tree of all threads. */

SPINLOCK_INITIALIZE(tidlock);
__u32 last_tid = 0;

static slab_cache_t *thread_slab;
#ifdef ARCH_HAS_FPU
slab_cache_t *fpu_context_slab;
#endif

/** Thread wrapper
 *
 * This wrapper is provided to ensure that every thread
 * makes a call to thread_exit() when its implementing
 * function returns.
 *
 * interrupts_disable() is assumed.
 *
 */
static void cushion(void)
{
	void (*f)(void *) = THREAD->thread_code;
	void *arg = THREAD->thread_arg;

	/* this is where each thread wakes up after its creation */
	spinlock_unlock(&THREAD->lock);
	interrupts_enable();

	f(arg);
	thread_exit();
	/* not reached */
}

/** Initialization and allocation for thread_t structure */
static int thr_constructor(void *obj, int kmflags)
{
	thread_t *t = (thread_t *)obj;
	pfn_t pfn;
	int status;

	spinlock_initialize(&t->lock, "thread_t_lock");
	link_initialize(&t->rq_link);
	link_initialize(&t->wq_link);
	link_initialize(&t->th_link);
	
#ifdef ARCH_HAS_FPU
#  ifdef CONFIG_FPU_LAZY
	t->saved_fpu_context = NULL;
#  else
	t->saved_fpu_context = slab_alloc(fpu_context_slab,kmflags);
	if (!t->saved_fpu_context)
		return -1;
#  endif
#endif	

	pfn = frame_alloc_rc(STACK_FRAMES, FRAME_KA | kmflags,&status);
	if (status) {
#ifdef ARCH_HAS_FPU
		if (t->saved_fpu_context)
			slab_free(fpu_context_slab,t->saved_fpu_context);
#endif
		return -1;
	}
	t->kstack = (__u8 *)PA2KA(PFN2ADDR(pfn));

	return 0;
}

/** Destruction of thread_t object */
static int thr_destructor(void *obj)
{
	thread_t *t = (thread_t *)obj;

	frame_free(ADDR2PFN(KA2PA(t->kstack)));
#ifdef ARCH_HAS_FPU
	if (t->saved_fpu_context)
		slab_free(fpu_context_slab,t->saved_fpu_context);
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
	thread_slab = slab_cache_create("thread_slab", 
					sizeof(thread_t),0, 
					thr_constructor, thr_destructor, 0);
#ifdef ARCH_HAS_FPU
	fpu_context_slab = slab_cache_create("fpu_slab",
					     sizeof(fpu_context_t),
					     FPU_CONTEXT_ALIGN,
					     NULL, NULL, 0);
#endif

	btree_create(&threads_btree);
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

	ASSERT(! (t->state == Ready));

	i = (t->priority < RQ_COUNT -1) ? ++t->priority : t->priority;
	
	cpu = CPU;
	if (t->flags & X_WIRED) {
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

/** Destroy thread memory structure
 *
 * Detach thread from all queues, cpus etc. and destroy it.
 *
 * Assume thread->lock is held!!
 */
void thread_destroy(thread_t *t)
{
	ASSERT(t->state == Exiting);
	ASSERT(t->task);
	ASSERT(t->cpu);

	spinlock_lock(&t->cpu->lock);
	if(t->cpu->fpu_owner==t)
		t->cpu->fpu_owner=NULL;
	spinlock_unlock(&t->cpu->lock);

	/*
	 * Detach from the containing task.
	 */
	spinlock_lock(&t->task->lock);
	list_remove(&t->th_link);
	spinlock_unlock(&t->task->lock);
	
	spinlock_unlock(&t->lock);
	
	spinlock_lock(&threads_lock);
	btree_remove(&threads_btree, (__native) t, NULL);
	spinlock_unlock(&threads_lock);
	
	slab_free(thread_slab, t);
}

/** Create new thread
 *
 * Create a new thread.
 *
 * @param func  Thread's implementing function.
 * @param arg   Thread's implementing function argument.
 * @param task  Task to which the thread belongs.
 * @param flags Thread flags.
 * @param name  Symbolic name.
 *
 * @return New thread's structure on success, NULL on failure.
 *
 */
thread_t *thread_create(void (* func)(void *), void *arg, task_t *task, int flags, char *name)
{
	thread_t *t;
	ipl_t ipl;
	
	t = (thread_t *) slab_alloc(thread_slab, 0);
	if (!t)
		return NULL;

	thread_create_arch(t);
	
	/* Not needed, but good for debugging */
	memsetb((__address)t->kstack, THREAD_STACK_SIZE * 1<<STACK_FRAMES, 0);
	
	ipl = interrupts_disable();
	spinlock_lock(&tidlock);
	t->tid = ++last_tid;
	spinlock_unlock(&tidlock);
	interrupts_restore(ipl);
	
	context_save(&t->saved_context);
	context_set(&t->saved_context, FADDR(cushion), (__address) t->kstack, THREAD_STACK_SIZE);
	
	the_initialize((the_t *) t->kstack);
	
	ipl = interrupts_disable();
	t->saved_context.ipl = interrupts_read();
	interrupts_restore(ipl);
	
	memcpy(t->name, name, THREAD_NAME_BUFLEN);
	
	t->thread_code = func;
	t->thread_arg = arg;
	t->ticks = -1;
	t->priority = -1;		/* start in rq[0] */
	t->cpu = NULL;
	t->flags = 0;
	t->state = Entering;
	t->call_me = NULL;
	t->call_me_with = NULL;
	
	timeout_initialize(&t->sleep_timeout);
	t->sleep_queue = NULL;
	t->timeout_pending = 0;
	
	t->rwlock_holder_type = RWLOCK_NONE;
		
	t->task = task;
	
	t->fpu_context_exists = 0;
	t->fpu_context_engaged = 0;
	
	/*
	 * Register this thread in the system-wide list.
	 */
	ipl = interrupts_disable();
	spinlock_lock(&threads_lock);
	btree_insert(&threads_btree, (__native) t, (void *) t, NULL);
	spinlock_unlock(&threads_lock);
	
	/*
	 * Attach to the containing task.
	 */
	spinlock_lock(&task->lock);
	list_append(&t->th_link, &task->th_head);
	spinlock_unlock(&task->lock);
	
	interrupts_restore(ipl);
	
	return t;
}

/** Make thread exiting
 *
 * End current thread execution and switch it to the exiting
 * state. All pending timeouts are executed.
 *
 */
void thread_exit(void)
{
	ipl_t ipl;

restart:
	ipl = interrupts_disable();
	spinlock_lock(&THREAD->lock);
	if (THREAD->timeout_pending) { /* busy waiting for timeouts in progress */
		spinlock_unlock(&THREAD->lock);
		interrupts_restore(ipl);
		goto restart;
	}
	THREAD->state = Exiting;
	spinlock_unlock(&THREAD->lock);
	scheduler();
}


/** Thread sleep
 *
 * Suspend execution of the current thread.
 *
 * @param sec Number of seconds to sleep.
 *
 */
void thread_sleep(__u32 sec)
{
	thread_usleep(sec*1000000);
}

/** Thread usleep
 *
 * Suspend execution of the current thread.
 *
 * @param usec Number of microseconds to sleep.
 *
 */	
void thread_usleep(__u32 usec)
{
	waitq_t wq;
				  
	waitq_initialize(&wq);

	(void) waitq_sleep_timeout(&wq, usec, SYNCH_NON_BLOCKING);
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

/** Print list of threads debug info */
void thread_print_list(void)
{
	link_t *cur;
	ipl_t ipl;
	
	/* Messing with thread structures, avoid deadlock */
	ipl = interrupts_disable();
	spinlock_lock(&threads_lock);

	for (cur = threads_btree.leaf_head.next; cur != &threads_btree.leaf_head; cur = cur->next) {
		btree_node_t *node;
		int i;

		node = list_get_instance(cur, btree_node_t, leaf_link);
		for (i = 0; i < node->keys; i++) {
			thread_t *t;
		
			t = (thread_t *) node->value[i];
			printf("%s: address=%P, tid=%d, state=%s, task=%P, code=%P, stack=%P, cpu=",
				t->name, t, t->tid, thread_states[t->state], t->task, t->thread_code, t->kstack);
			if (t->cpu)
				printf("cpu%d ", t->cpu->id);
			else
				printf("none");
			printf("\n");
		}
	}

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
	btree_node_t *leaf;
	
	return btree_search(&threads_btree, (__native) t, &leaf) != NULL;
}

/** Process syscall to create new thread.
 *
 */
__native sys_thread_create(uspace_arg_t *uspace_uarg, char *uspace_name)
{
        thread_t *t;
        char namebuf[THREAD_NAME_BUFLEN];
	uspace_arg_t *kernel_uarg;
	__u32 tid;

        copy_from_uspace(namebuf, uspace_name, THREAD_NAME_BUFLEN);

	kernel_uarg = (uspace_arg_t *) malloc(sizeof(uspace_arg_t), 0);	
	copy_from_uspace(kernel_uarg, uspace_uarg, sizeof(uspace_arg_t));

        if ((t = thread_create(uinit, kernel_uarg, TASK, 0, namebuf))) {
		tid = t->tid;
                thread_ready(t);
		return (__native) tid; 
        } else {
		free(kernel_uarg);
        }

        return (__native) -1;
}

/** Process syscall to terminate thread.
 *
 */
__native sys_thread_exit(int uspace_status)
{
        thread_exit();
        /* Unreachable */
        return 0;
}
