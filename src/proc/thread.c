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

char *thread_states[] = {"Invalid", "Running", "Sleeping", "Ready", "Entering", "Exiting"};

spinlock_t threads_lock;
link_t threads_head;

static spinlock_t tidlock;
__u32 last_tid = 0;

/*
 * cushion() is provided to ensure that every thread
 * makes a call to thread_exit() when its implementing
 * function returns.
 *
 * cpu_priority_high()'d
 */
void cushion(void)
{
	void (*f)(void *) = THREAD->thread_code;
	void *arg = THREAD->thread_arg;

	/* this is where each thread wakes up after its creation */
	spinlock_unlock(&THREAD->lock);
	cpu_priority_low();

	f(arg);
	thread_exit();
	/* not reached */
}

void thread_init(void)
{
	THREAD = NULL;
	nrdy = 0;
	spinlock_initialize(&threads_lock);
	list_initialize(&threads_head);
}

void thread_ready(thread_t *t)
{
	cpu_t *cpu;
	runq_t *r;
	pri_t pri;
	int i, avg, send_ipi = 0;

	pri = cpu_priority_high();

	spinlock_lock(&t->lock);

	i = (t->pri < RQ_COUNT -1) ? ++t->pri : t->pri;
	
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

	spinlock_lock(&nrdylock);
	avg = ++nrdy / config.cpu_active;
	spinlock_unlock(&nrdylock);

	spinlock_lock(&cpu->lock);
	if ((++cpu->nrdy) > avg) {
		/*
		 * If there are idle halted CPU's, this will wake them up.
		 */
		ipi_broadcast(VECTOR_WAKEUP_IPI);
	}	
	spinlock_unlock(&cpu->lock);
    
	cpu_priority_restore(pri);
}

thread_t *thread_create(void (* func)(void *), void *arg, task_t *task, int flags)
{
	thread_t *t;
	__address frame_ks, frame_us = NULL;

	t = (thread_t *) malloc(sizeof(thread_t));
	if (t) {
		pri_t pri;
	
		spinlock_initialize(&t->lock);
	
		frame_ks = frame_alloc(FRAME_KA);
		if (THREAD_USER_STACK & flags) {
			frame_us = frame_alloc(0);
		}

		pri = cpu_priority_high();
		spinlock_lock(&tidlock);
		t->tid = ++last_tid;
		spinlock_unlock(&tidlock);
		cpu_priority_restore(pri);

		memsetb(frame_ks, THREAD_STACK_SIZE, 0);
		link_initialize(&t->rq_link);
		link_initialize(&t->wq_link);
		link_initialize(&t->th_link);
		link_initialize(&t->threads_link);
		t->kstack = (__u8 *) frame_ks;
		t->ustack = (__u8 *) frame_us;
		
		
		context_save(&t->saved_context);
		t->saved_context.pc = (__address) cushion;
		t->saved_context.sp = (__address) &t->kstack[THREAD_STACK_SIZE-8];

		pri = cpu_priority_high();
		t->saved_context.pri = cpu_priority_read();
		cpu_priority_restore(pri);
		
		t->thread_code = func;
		t->thread_arg = arg;
		t->ticks = -1;
		t->pri = -1;		/* start in rq[0] */
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
		
		/*
		 * Register this thread in the system-wide list.
		 */
		pri = cpu_priority_high();		
		spinlock_lock(&threads_lock);
		list_append(&t->threads_link, &threads_head);
		spinlock_unlock(&threads_lock);

		/*
		 * Attach to the containing task.
		 */
		spinlock_lock(&task->lock);
		list_append(&t->th_link, &task->th_head);
		spinlock_unlock(&task->lock);

		cpu_priority_restore(pri);
	}

	return t;
}

void thread_exit(void)
{
	pri_t pri;

restart:
	pri = cpu_priority_high();
	spinlock_lock(&THREAD->lock);
	if (THREAD->timeout_pending) { /* busy waiting for timeouts in progress */
		spinlock_unlock(&THREAD->lock);
		cpu_priority_restore(pri);
		goto restart;
	}
	THREAD->state = Exiting;
	spinlock_unlock(&THREAD->lock);
	scheduler();
}

void thread_sleep(__u32 sec)
{
        thread_usleep(sec*1000000);
}
	
/*
 * Suspend execution of current thread for usec microseconds.
 */
void thread_usleep(__u32 usec)
{
	waitq_t wq;
				  
	waitq_initialize(&wq);

	(void) waitq_sleep_timeout(&wq, usec, SYNCH_NON_BLOCKING);
}

void thread_register_call_me(void (* call_me)(void *), void *call_me_with)
{
	pri_t pri;
	
	pri = cpu_priority_high();
	spinlock_lock(&THREAD->lock);
	THREAD->call_me = call_me;
	THREAD->call_me_with = call_me_with;
	spinlock_unlock(&THREAD->lock);
	cpu_priority_restore(pri);
}
