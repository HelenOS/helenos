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

#include <context.h>
#include <proc/thread.h>

#include <synch/synch.h>
#include <synch/waitq.h>
#include <synch/spinlock.h>

#include <arch/asm.h>
#include <arch/types.h>
#include <arch.h>

#include <list.h>

#include <time/timeout.h>

void waitq_initialize(waitq_t *wq)
{
	spinlock_initialize(&wq->lock);
	list_initialize(&wq->head);
	wq->missed_wakeups = 0;
}

/*
 * Called with interrupts disabled from clock() when sleep_timeout
 * timeouts. This function is not allowed to enable interrupts.
 *
 * It is supposed to try to remove 'its' thread from the waitqueue; it
 * can eventually fail to achieve this goal when these two events
 * overlap; in that case it behaves just as though there was no
 * timeout at all
 */
void waitq_interrupted_sleep(void *data)
{
	thread_t *t = (thread_t *) data;
	waitq_t *wq;
	int do_wakeup = 0;

	spinlock_lock(&threads_lock);
	if (!list_member(&t->threads_link, &threads_head))
		goto out;

grab_locks:
	spinlock_lock(&t->lock);
	if (wq = t->sleep_queue) {
		if (!spinlock_trylock(&wq->lock)) {
			spinlock_unlock(&t->lock);
			goto grab_locks; /* avoid deadlock */
		}

		list_remove(&t->wq_link);
		t->saved_context = t->sleep_timeout_context;
		do_wakeup = 1;
		
		spinlock_unlock(&wq->lock);
		t->sleep_queue = NULL;
	}
	
	t->timeout_pending = 0;
	spinlock_unlock(&t->lock);
	
	if (do_wakeup) thread_ready(t);

out:
	spinlock_unlock(&threads_lock);
}

/*
 * This is a sleep implementation which allows itself to be
 * interrupted from the sleep, restoring a failover context.
 *
 * This function is really basic in that other functions as waitq_sleep()
 * and all the *_timeout() functions use it.
 *
 * The third argument controls whether only a conditional sleep
 * (non-blocking sleep) is called for when the second argument is 0.
 *
 * usec | nonblocking |	what happens if there is no missed_wakeup
 * -----+-------------+--------------------------------------------
 *  0	| 0	      |	blocks without timeout until wakeup
 *  0	| <> 0	      |	immediately returns ESYNCH_WOULD_BLOCK
 *  > 0	| x	      |	blocks with timeout until timeout or wakeup
 *
 * return values:
 *  ESYNCH_WOULD_BLOCK 
 *  ESYNCH_TIMEOUT 
 *  ESYNCH_OK_ATOMIC 
 *  ESYNCH_OK_BLOCKED
 */
int waitq_sleep_timeout(waitq_t *wq, __u32 usec, int nonblocking)
{
	volatile pri_t pri; /* must be live after context_restore() */
	
	
restart:
	pri = cpu_priority_high();
	
	/*
	 * Busy waiting for a delayed timeout.
	 * This is an important fix for the race condition between
	 * a delayed timeout and a next call to waitq_sleep_timeout().
	 * Simply, the thread is not allowed to go to sleep if
	 * there are timeouts in progress.
	 */
	spinlock_lock(&the->thread->lock);
	if (the->thread->timeout_pending) {
		spinlock_unlock(&the->thread->lock);
		cpu_priority_restore(pri);		
		goto restart;
	}
	spinlock_unlock(&the->thread->lock);
	
	spinlock_lock(&wq->lock);
	
	/* checks whether to go to sleep at all */
	if (wq->missed_wakeups) {
		wq->missed_wakeups--;
		spinlock_unlock(&wq->lock);
		cpu_priority_restore(pri);
		return ESYNCH_OK_ATOMIC;
	}
	else {
		if (nonblocking && (usec == 0)) {
			/* return immediatelly instead of going to sleep */
			spinlock_unlock(&wq->lock);
			cpu_priority_restore(pri);
			return ESYNCH_WOULD_BLOCK;
		}
	}

	
	/*
	 * Now we are firmly decided to go to sleep.
	 */
	spinlock_lock(&the->thread->lock);
	if (usec) {
		/* We use the timeout variant. */
		if (!context_save(&the->thread->sleep_timeout_context)) {
			/*
			 * Short emulation of scheduler() return code.
			 */
			spinlock_unlock(&the->thread->lock);
			cpu_priority_restore(pri);
			return ESYNCH_TIMEOUT;
		}
		the->thread->timeout_pending = 1;
		timeout_register(&the->thread->sleep_timeout, (__u64) usec, waitq_interrupted_sleep, the->thread);
	}

	list_append(&the->thread->wq_link, &wq->head);

	/*
	 * Suspend execution.
	 */
	the->thread->state = Sleeping;
	the->thread->sleep_queue = wq;

	spinlock_unlock(&the->thread->lock);

	scheduler(); 	/* wq->lock is released in scheduler_separated_stack() */
	cpu_priority_restore(pri);
	
	return ESYNCH_OK_BLOCKED;
}


/*
 * This is the SMP- and IRQ-safe wrapper meant for general use.
 */
/*
 * Besides its 'normal' wakeup operation, it attempts to unregister possible timeout.
 */
void waitq_wakeup(waitq_t *wq, int all)
{
	pri_t pri;

	pri = cpu_priority_high();
	spinlock_lock(&wq->lock);

	_waitq_wakeup_unsafe(wq, all);

	spinlock_unlock(&wq->lock);	
	cpu_priority_restore(pri);	
}

/*
 * This is the internal SMP- and IRQ-unsafe version of waitq_wakeup.
 * It assumes wq->lock is already locked.
 */
void _waitq_wakeup_unsafe(waitq_t *wq, int all)
{
	thread_t *t;

loop:	
	if (list_empty(&wq->head)) {
		wq->missed_wakeups++;
		if (all) wq->missed_wakeups = 0;
		return;
	}

	t = list_get_instance(wq->head.next, thread_t, wq_link);
	
	list_remove(&t->wq_link);
	spinlock_lock(&t->lock);
	if (t->timeout_pending && timeout_unregister(&t->sleep_timeout))
		t->timeout_pending = 0;
	t->sleep_queue = NULL;
	spinlock_unlock(&t->lock);

	thread_ready(t);

	if (all) goto loop;
}
