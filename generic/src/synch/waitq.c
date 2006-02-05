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

#include <synch/waitq.h>
#include <synch/synch.h>
#include <synch/spinlock.h>
#include <proc/thread.h>
#include <proc/scheduler.h>
#include <arch/asm.h>
#include <arch/types.h>
#include <typedefs.h>
#include <time/timeout.h>
#include <arch.h>
#include <context.h>
#include <adt/list.h>

/** Initialize wait queue
 *
 * Initialize wait queue.
 *
 * @param wq Pointer to wait queue to be initialized.
 */
void waitq_initialize(waitq_t *wq)
{
	spinlock_initialize(&wq->lock, "waitq_lock");
	list_initialize(&wq->head);
	wq->missed_wakeups = 0;
}

/** Handle timeout during waitq_sleep_timeout() call
 *
 * This routine is called when waitq_sleep_timeout() timeouts.
 * Interrupts are disabled.
 *
 * It is supposed to try to remove 'its' thread from the wait queue;
 * it can eventually fail to achieve this goal when these two events
 * overlap. In that case it behaves just as though there was no
 * timeout at all.
 *
 * @param data Pointer to the thread that called waitq_sleep_timeout().
 */
void waitq_interrupted_sleep(void *data)
{
	thread_t *t = (thread_t *) data;
	waitq_t *wq;
	bool do_wakeup = false;

	spinlock_lock(&threads_lock);
	if (!list_member(&t->threads_link, &threads_head))
		goto out;

grab_locks:
	spinlock_lock(&t->lock);
	if ((wq = t->sleep_queue)) {		/* assignment */
		if (!spinlock_trylock(&wq->lock)) {
			spinlock_unlock(&t->lock);
			goto grab_locks;	/* avoid deadlock */
		}

		list_remove(&t->wq_link);
		t->saved_context = t->sleep_timeout_context;
		do_wakeup = true;
		
		spinlock_unlock(&wq->lock);
		t->sleep_queue = NULL;
	}
	
	t->timeout_pending = false;
	spinlock_unlock(&t->lock);
	
	if (do_wakeup)
		thread_ready(t);

out:
	spinlock_unlock(&threads_lock);
}

/** Sleep until either wakeup or timeout occurs
 *
 * This is a sleep implementation which allows itself to be
 * interrupted from the sleep, restoring a failover context.
 *
 * Sleepers are organised in FIFO fashion in a structure called wait queue.
 *
 * This function is really basic in that other functions as waitq_sleep()
 * and all the *_timeout() functions use it.
 *
 * @param wq Pointer to wait queue.
 * @param usec Timeout in microseconds.
 * @param nonblocking Blocking vs. non-blocking operation mode switch.
 *
 * If usec is greater than zero, regardless of the value of nonblocking,
 * the call will not return until either timeout or wakeup comes.
 *
 * If usec is zero and nonblocking is zero (false), the call
 * will not return until wakeup comes.
 *
 * If usec is zero and nonblocking is non-zero (true), the call will
 * immediately return, reporting either success or failure.
 *
 * @return Returns one of: ESYNCH_WOULD_BLOCK, ESYNCH_TIMEOUT,
 *         ESYNCH_OK_ATOMIC, ESYNCH_OK_BLOCKED.
 *
 * ESYNCH_WOULD_BLOCK means that the sleep failed because at the time
 * of the call there was no pending wakeup.
 *
 * ESYNCH_TIMEOUT means that the sleep timed out.
 *
 * ESYNCH_OK_ATOMIC means that the sleep succeeded and that there was
 * a pending wakeup at the time of the call. The caller was not put
 * asleep at all.
 * 
 * ESYNCH_OK_BLOCKED means that the sleep succeeded; the full sleep was 
 * attempted.
 */
int waitq_sleep_timeout(waitq_t *wq, __u32 usec, int nonblocking)
{
	volatile ipl_t ipl; /* must be live after context_restore() */
	
	
restart:
	ipl = interrupts_disable();
	
	/*
	 * Busy waiting for a delayed timeout.
	 * This is an important fix for the race condition between
	 * a delayed timeout and a next call to waitq_sleep_timeout().
	 * Simply, the thread is not allowed to go to sleep if
	 * there are timeouts in progress.
	 */
	spinlock_lock(&THREAD->lock);
	if (THREAD->timeout_pending) {
		spinlock_unlock(&THREAD->lock);
		interrupts_restore(ipl);		
		goto restart;
	}
	spinlock_unlock(&THREAD->lock);
	
	spinlock_lock(&wq->lock);
	
	/* checks whether to go to sleep at all */
	if (wq->missed_wakeups) {
		wq->missed_wakeups--;
		spinlock_unlock(&wq->lock);
		interrupts_restore(ipl);
		return ESYNCH_OK_ATOMIC;
	}
	else {
		if (nonblocking && (usec == 0)) {
			/* return immediatelly instead of going to sleep */
			spinlock_unlock(&wq->lock);
			interrupts_restore(ipl);
			return ESYNCH_WOULD_BLOCK;
		}
	}

	
	/*
	 * Now we are firmly decided to go to sleep.
	 */
	spinlock_lock(&THREAD->lock);
	if (usec) {
		/* We use the timeout variant. */
		if (!context_save(&THREAD->sleep_timeout_context)) {
			/*
			 * Short emulation of scheduler() return code.
			 */
			before_thread_runs();
			spinlock_unlock(&THREAD->lock);
			interrupts_restore(ipl);
			return ESYNCH_TIMEOUT;
		}
		THREAD->timeout_pending = true;
		timeout_register(&THREAD->sleep_timeout, (__u64) usec, waitq_interrupted_sleep, THREAD);
	}

	list_append(&THREAD->wq_link, &wq->head);

	/*
	 * Suspend execution.
	 */
	THREAD->state = Sleeping;
	THREAD->sleep_queue = wq;

	spinlock_unlock(&THREAD->lock);

	scheduler(); 	/* wq->lock is released in scheduler_separated_stack() */
	interrupts_restore(ipl);
	
	return ESYNCH_OK_BLOCKED;
}


/** Wake up first thread sleeping in a wait queue
 *
 * Wake up first thread sleeping in a wait queue.
 * This is the SMP- and IRQ-safe wrapper meant for
 * general use.
 *
 * Besides its 'normal' wakeup operation, it attempts
 * to unregister possible timeout.
 *
 * @param wq Pointer to wait queue.
 * @param all If this is non-zero, all sleeping threads
 *        will be woken up and missed count will be zeroed.
 */
void waitq_wakeup(waitq_t *wq, bool all)
{
	ipl_t ipl;

	ipl = interrupts_disable();
	spinlock_lock(&wq->lock);

	_waitq_wakeup_unsafe(wq, all);

	spinlock_unlock(&wq->lock);	
	interrupts_restore(ipl);	
}

/** Internal SMP- and IRQ-unsafe version of waitq_wakeup()
 *
 * This is the internal SMP- and IRQ-unsafe version
 * of waitq_wakeup(). It assumes wq->lock is already
 * locked and interrupts are already disabled.
 *
 * @param wq Pointer to wait queue.
 * @param all If this is non-zero, all sleeping threads
 *        will be woken up and missed count will be zeroed.
 */
void _waitq_wakeup_unsafe(waitq_t *wq, bool all)
{
	thread_t *t;

loop:	
	if (list_empty(&wq->head)) {
		wq->missed_wakeups++;
		if (all)
			wq->missed_wakeups = 0;
		return;
	}

	t = list_get_instance(wq->head.next, thread_t, wq_link);
	
	list_remove(&t->wq_link);
	spinlock_lock(&t->lock);
	if (t->timeout_pending && timeout_unregister(&t->sleep_timeout))
		t->timeout_pending = false;
	t->sleep_queue = NULL;
	spinlock_unlock(&t->lock);

	thread_ready(t);

	if (all)
		goto loop;
}
