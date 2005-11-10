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
#include <mm/heap.h>
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
#include <list.h>
#include <typedefs.h>
#include <time/clock.h>
#include <list.h>
#include <config.h>
#include <arch/interrupt.h>
#include <smp/ipi.h>
#include <arch/faddr.h>
#include <arch/atomic.h>
#include <memstr.h>

char *thread_states[] = {"Invalid", "Running", "Sleeping", "Ready", "Entering", "Exiting"}; /**< Thread states */

spinlock_t threads_lock;
link_t threads_head;

static spinlock_t tidlock;
__u32 last_tid = 0;


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
	before_thread_runs();

	spinlock_unlock(&THREAD->lock);
	interrupts_enable();

	f(arg);
	thread_exit();
	/* not reached */
}


/** Initialize threads
 *
 * Initialize kernel threads support.
 *
 */
void thread_init(void)
{
	THREAD = NULL;
	nrdy = 0;
	spinlock_initialize(&threads_lock);
	list_initialize(&threads_head);
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
	int i, avg, send_ipi = 0;

	ipl = interrupts_disable();

	spinlock_lock(&t->lock);

	i = (t->priority < RQ_COUNT -1) ? ++t->priority : t->priority;
	
	cpu = CPU;
	if (t->flags & X_WIRED) {
		cpu = t->cpu;
	}
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
	avg = nrdy / config.cpu_active;

	spinlock_lock(&cpu->lock);
	if ((++cpu->nrdy) > avg) {
		/*
		 * If there are idle halted CPU's, this will wake them up.
		 */
		ipi_broadcast(VECTOR_WAKEUP_IPI);
	}	
	spinlock_unlock(&cpu->lock);

	interrupts_restore(ipl);
}


/** Create new thread
 *
 * Create a new thread.
 *
 * @param func  Thread's implementing function.
 * @param arg   Thread's implementing function argument.
 * @param task  Task to which the thread belongs.
 * @param flags Thread flags.
 *
 * @return New thread's structure on success, NULL on failure.
 *
 */
thread_t *thread_create(void (* func)(void *), void *arg, task_t *task, int flags)
{
	thread_t *t;
	__address frame_ks, frame_us = NULL;

	t = (thread_t *) malloc(sizeof(thread_t));
	if (t) {
		ipl_t ipl;
	
		spinlock_initialize(&t->lock);
	
		frame_ks = frame_alloc(FRAME_KA);
		if (THREAD_USER_STACK & flags) {
			frame_us = frame_alloc(FRAME_KA);
		}

		ipl = interrupts_disable();
		spinlock_lock(&tidlock);
		t->tid = ++last_tid;
		spinlock_unlock(&tidlock);
		interrupts_restore(ipl);
		
		memsetb(frame_ks, THREAD_STACK_SIZE, 0);
		link_initialize(&t->rq_link);
		link_initialize(&t->wq_link);
		link_initialize(&t->th_link);
		link_initialize(&t->threads_link);
		t->kstack = (__u8 *) frame_ks;
		t->ustack = (__u8 *) frame_us;
		
		context_save(&t->saved_context);
		context_set(&t->saved_context, FADDR(cushion), (__address) t->kstack, THREAD_STACK_SIZE);
		
		the_initialize((the_t *) t->kstack);

		ipl = interrupts_disable();
		t->saved_context.ipl = interrupts_read();
		interrupts_restore(ipl);
		
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
		
		t->fpu_context_exists=0;
		t->fpu_context_engaged=0;
		
		/*
		 * Register this thread in the system-wide list.
		 */
		ipl = interrupts_disable();		
		spinlock_lock(&threads_lock);
		list_append(&t->threads_link, &threads_head);
		spinlock_unlock(&threads_lock);

		/*
		 * Attach to the containing task.
		 */
		spinlock_lock(&task->lock);
		list_append(&t->th_link, &task->th_head);
		spinlock_unlock(&task->lock);

		interrupts_restore(ipl);
	}

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
