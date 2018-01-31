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

/** @addtogroup sync
 * @{
 */

/**
 * @file
 * @brief Wait queue.
 *
 * Wait queue is the basic synchronization primitive upon which all
 * other synchronization primitives build.
 *
 * It allows threads to wait for an event in first-come, first-served
 * fashion. Conditional operation as well as timeouts and interruptions
 * are supported.
 *
 */

#include <assert.h>
#include <errno.h>
#include <synch/waitq.h>
#include <synch/spinlock.h>
#include <proc/thread.h>
#include <proc/scheduler.h>
#include <arch/asm.h>
#include <typedefs.h>
#include <time/timeout.h>
#include <arch.h>
#include <context.h>
#include <adt/list.h>
#include <arch/cycle.h>

static void waitq_sleep_timed_out(void *);
static void waitq_complete_wakeup(waitq_t *);


/** Initialize wait queue
 *
 * Initialize wait queue.
 *
 * @param wq Pointer to wait queue to be initialized.
 *
 */
void waitq_initialize(waitq_t *wq)
{
	irq_spinlock_initialize(&wq->lock, "wq.lock");
	list_initialize(&wq->sleepers);
	wq->missed_wakeups = 0;
}

/** Handle timeout during waitq_sleep_timeout() call
 *
 * This routine is called when waitq_sleep_timeout() times out.
 * Interrupts are disabled.
 *
 * It is supposed to try to remove 'its' thread from the wait queue;
 * it can eventually fail to achieve this goal when these two events
 * overlap. In that case it behaves just as though there was no
 * timeout at all.
 *
 * @param data Pointer to the thread that called waitq_sleep_timeout().
 *
 */
void waitq_sleep_timed_out(void *data)
{
	thread_t *thread = (thread_t *) data;
	bool do_wakeup = false;
	DEADLOCK_PROBE_INIT(p_wqlock);
	
	irq_spinlock_lock(&threads_lock, false);
	if (!thread_exists(thread))
		goto out;
	
grab_locks:
	irq_spinlock_lock(&thread->lock, false);
	
	waitq_t *wq;
	if ((wq = thread->sleep_queue)) {  /* Assignment */
		if (!irq_spinlock_trylock(&wq->lock)) {
			irq_spinlock_unlock(&thread->lock, false);
			DEADLOCK_PROBE(p_wqlock, DEADLOCK_THRESHOLD);
			/* Avoid deadlock */
			goto grab_locks;
		}
		
		list_remove(&thread->wq_link);
		thread->saved_context = thread->sleep_timeout_context;
		do_wakeup = true;
		thread->sleep_queue = NULL;
		irq_spinlock_unlock(&wq->lock, false);
	}
	
	thread->timeout_pending = false;
	irq_spinlock_unlock(&thread->lock, false);
	
	if (do_wakeup)
		thread_ready(thread);
	
out:
	irq_spinlock_unlock(&threads_lock, false);
}

/** Interrupt sleeping thread.
 *
 * This routine attempts to interrupt a thread from its sleep in
 * a waitqueue. If the thread is not found sleeping, no action
 * is taken.
 *
 * The threads_lock must be already held and interrupts must be
 * disabled upon calling this function.
 *
 * @param thread Thread to be interrupted.
 *
 */
void waitq_interrupt_sleep(thread_t *thread)
{
	bool do_wakeup = false;
	DEADLOCK_PROBE_INIT(p_wqlock);
	
	/*
	 * The thread is quaranteed to exist because
	 * threads_lock is held.
	 */
	
grab_locks:
	irq_spinlock_lock(&thread->lock, false);
	
	waitq_t *wq;
	if ((wq = thread->sleep_queue)) {  /* Assignment */
		if (!(thread->sleep_interruptible)) {
			/*
			 * The sleep cannot be interrupted.
			 */
			irq_spinlock_unlock(&thread->lock, false);
			return;
		}
		
		if (!irq_spinlock_trylock(&wq->lock)) {
			/* Avoid deadlock */
			irq_spinlock_unlock(&thread->lock, false);
			DEADLOCK_PROBE(p_wqlock, DEADLOCK_THRESHOLD);
			goto grab_locks;
		}
		
		if ((thread->timeout_pending) &&
		    (timeout_unregister(&thread->sleep_timeout)))
			thread->timeout_pending = false;
		
		list_remove(&thread->wq_link);
		thread->saved_context = thread->sleep_interruption_context;
		do_wakeup = true;
		thread->sleep_queue = NULL;
		irq_spinlock_unlock(&wq->lock, false);
	}
	
	irq_spinlock_unlock(&thread->lock, false);
	
	if (do_wakeup)
		thread_ready(thread);
}

/** Interrupt the first thread sleeping in the wait queue.
 *
 * Note that the caller somehow needs to know that the thread to be interrupted
 * is sleeping interruptibly.
 *
 * @param wq Pointer to wait queue.
 *
 */
void waitq_unsleep(waitq_t *wq)
{
	irq_spinlock_lock(&wq->lock, true);
	
	if (!list_empty(&wq->sleepers)) {
		thread_t *thread = list_get_instance(list_first(&wq->sleepers),
		    thread_t, wq_link);
		
		irq_spinlock_lock(&thread->lock, false);
		
		assert(thread->sleep_interruptible);
		
		if ((thread->timeout_pending) &&
		    (timeout_unregister(&thread->sleep_timeout)))
			thread->timeout_pending = false;
		
		list_remove(&thread->wq_link);
		thread->saved_context = thread->sleep_interruption_context;
		thread->sleep_queue = NULL;
		
		irq_spinlock_unlock(&thread->lock, false);
		thread_ready(thread);
	}
	
	irq_spinlock_unlock(&wq->lock, true);
}

#define PARAM_NON_BLOCKING(flags, usec) \
	(((flags) & SYNCH_FLAGS_NON_BLOCKING) && ((usec) == 0))

/** Sleep until either wakeup, timeout or interruption occurs
 *
 * This is a sleep implementation which allows itself to time out or to be
 * interrupted from the sleep, restoring a failover context.
 *
 * Sleepers are organised in a FIFO fashion in a structure called wait queue.
 *
 * This function is really basic in that other functions as waitq_sleep()
 * and all the *_timeout() functions use it.
 *
 * @param wq    Pointer to wait queue.
 * @param usec  Timeout in microseconds.
 * @param flags Specify mode of the sleep.
 *
 * @param[out] blocked  On return, regardless of the return code,
 *                      `*blocked` is set to `true` iff the thread went to
 *                      sleep.
 *
 * The sleep can be interrupted only if the
 * SYNCH_FLAGS_INTERRUPTIBLE bit is specified in flags.
 *
 * If usec is greater than zero, regardless of the value of the
 * SYNCH_FLAGS_NON_BLOCKING bit in flags, the call will not return until either
 * timeout, interruption or wakeup comes.
 *
 * If usec is zero and the SYNCH_FLAGS_NON_BLOCKING bit is not set in flags,
 * the call will not return until wakeup or interruption comes.
 *
 * If usec is zero and the SYNCH_FLAGS_NON_BLOCKING bit is set in flags, the
 * call will immediately return, reporting either success or failure.
 *
 * @return EAGAIN, meaning that the sleep failed because it was requested
 *                 as SYNCH_FLAGS_NON_BLOCKING, but there was no pending wakeup.
 * @return ETIMEOUT, meaning that the sleep timed out.
 * @return EINTR, meaning that somebody interrupted the sleeping
 *         thread. Check the value of `*blocked` to see if the thread slept,
 *         or if a pending interrupt forced it to return immediately.
 * @return EOK, meaning that none of the above conditions occured, and the
 *              thread was woken up successfuly by `waitq_wakeup()`. Check
 *              the value of `*blocked` to see if the thread slept or if
 *              the wakeup was already pending.
 *
 */
errno_t waitq_sleep_timeout(waitq_t *wq, uint32_t usec, unsigned int flags, bool *blocked)
{
	assert((!PREEMPTION_DISABLED) || (PARAM_NON_BLOCKING(flags, usec)));
	
	ipl_t ipl = waitq_sleep_prepare(wq);
	bool nblocked;
	errno_t rc = waitq_sleep_timeout_unsafe(wq, usec, flags, &nblocked);
	waitq_sleep_finish(wq, nblocked, ipl);

	if (blocked != NULL) {
		*blocked = nblocked;
	}
	return rc;
}

/** Prepare to sleep in a waitq.
 *
 * This function will return holding the lock of the wait queue
 * and interrupts disabled.
 *
 * @param wq Wait queue.
 *
 * @return Interrupt level as it existed on entry to this function.
 *
 */
ipl_t waitq_sleep_prepare(waitq_t *wq)
{
	ipl_t ipl;
	
restart:
	ipl = interrupts_disable();
	
	if (THREAD) {  /* Needed during system initiailzation */
		/*
		 * Busy waiting for a delayed timeout.
		 * This is an important fix for the race condition between
		 * a delayed timeout and a next call to waitq_sleep_timeout().
		 * Simply, the thread is not allowed to go to sleep if
		 * there are timeouts in progress.
		 *
		 */
		irq_spinlock_lock(&THREAD->lock, false);
		
		if (THREAD->timeout_pending) {
			irq_spinlock_unlock(&THREAD->lock, false);
			interrupts_restore(ipl);
			goto restart;
		}
		
		irq_spinlock_unlock(&THREAD->lock, false);
	}
	
	irq_spinlock_lock(&wq->lock, false);
	return ipl;
}

/** Finish waiting in a wait queue.
 *
 * This function restores interrupts to the state that existed prior
 * to the call to waitq_sleep_prepare(). If necessary, the wait queue
 * lock is released.
 *
 * @param wq       Wait queue.
 * @param blocked  Out parameter of waitq_sleep_timeout_unsafe().
 * @param ipl      Interrupt level returned by waitq_sleep_prepare().
 *
 */
void waitq_sleep_finish(waitq_t *wq, bool blocked, ipl_t ipl)
{
	if (blocked) {
		/*
		 * Wait for a waitq_wakeup() or waitq_unsleep() to complete
		 * before returning from waitq_sleep() to the caller. Otherwise
		 * the caller might expect that the wait queue is no longer used 
		 * and deallocate it (although the wakeup on a another cpu has 
		 * not yet completed and is using the wait queue).
		 *
		 * Note that we have to do this for EOK and EINTR, but not
		 * necessarily for ETIMEOUT where the timeout handler stops
		 * using the waitq before waking us up. To be on the safe side,
		 * ensure the waitq is not in use anymore in this case as well.
		 */
		waitq_complete_wakeup(wq);
	} else {
		irq_spinlock_unlock(&wq->lock, false);
	}
	
	interrupts_restore(ipl);
}

/** Internal implementation of waitq_sleep_timeout().
 *
 * This function implements logic of sleeping in a wait queue.
 * This call must be preceded by a call to waitq_sleep_prepare()
 * and followed by a call to waitq_sleep_finish().
 *
 * @param wq    See waitq_sleep_timeout().
 * @param usec  See waitq_sleep_timeout().
 * @param flags See waitq_sleep_timeout().
 *
 * @param[out] blocked  See waitq_sleep_timeout().
 *
 * @return See waitq_sleep_timeout().
 *
 */
errno_t waitq_sleep_timeout_unsafe(waitq_t *wq, uint32_t usec, unsigned int flags, bool *blocked)
{
	*blocked = false;

	/* Checks whether to go to sleep at all */
	if (wq->missed_wakeups) {
		wq->missed_wakeups--;
		return EOK;
	} else {
		if (PARAM_NON_BLOCKING(flags, usec)) {
			/* Return immediately instead of going to sleep */
			return EAGAIN;
		}
	}
	
	/*
	 * Now we are firmly decided to go to sleep.
	 *
	 */
	irq_spinlock_lock(&THREAD->lock, false);
	
	if (flags & SYNCH_FLAGS_INTERRUPTIBLE) {
		/*
		 * If the thread was already interrupted,
		 * don't go to sleep at all.
		 */
		if (THREAD->interrupted) {
			irq_spinlock_unlock(&THREAD->lock, false);
			return EINTR;
		}
		
		/*
		 * Set context that will be restored if the sleep
		 * of this thread is ever interrupted.
		 */
		THREAD->sleep_interruptible = true;
		if (!context_save(&THREAD->sleep_interruption_context)) {
			/* Short emulation of scheduler() return code. */
			THREAD->last_cycle = get_cycle();
			irq_spinlock_unlock(&THREAD->lock, false);
			return EINTR;
		}
	} else
		THREAD->sleep_interruptible = false;
	
	if (usec) {
		/* We use the timeout variant. */
		if (!context_save(&THREAD->sleep_timeout_context)) {
			/* Short emulation of scheduler() return code. */
			THREAD->last_cycle = get_cycle();
			irq_spinlock_unlock(&THREAD->lock, false);
			return ETIMEOUT;
		}
		
		THREAD->timeout_pending = true;
		timeout_register(&THREAD->sleep_timeout, (uint64_t) usec,
		    waitq_sleep_timed_out, THREAD);
	}
	
	list_append(&THREAD->wq_link, &wq->sleepers);
	
	/*
	 * Suspend execution.
	 *
	 */
	THREAD->state = Sleeping;
	THREAD->sleep_queue = wq;
	
	/* Must be before entry to scheduler, because there are multiple
	 * return vectors.
	 */
	*blocked = true;
	
	irq_spinlock_unlock(&THREAD->lock, false);
	
	/* wq->lock is released in scheduler_separated_stack() */
	scheduler();
	
	return EOK;
}

/** Wake up first thread sleeping in a wait queue
 *
 * Wake up first thread sleeping in a wait queue. This is the SMP- and IRQ-safe
 * wrapper meant for general use.
 *
 * Besides its 'normal' wakeup operation, it attempts to unregister possible
 * timeout.
 *
 * @param wq   Pointer to wait queue.
 * @param mode Wakeup mode.
 *
 */
void waitq_wakeup(waitq_t *wq, wakeup_mode_t mode)
{
	irq_spinlock_lock(&wq->lock, true);
	_waitq_wakeup_unsafe(wq, mode);
	irq_spinlock_unlock(&wq->lock, true);
}

/** If there is a wakeup in progress actively waits for it to complete.
 * 
 * The function returns once the concurrently running waitq_wakeup()
 * exits. It returns immediately if there are no concurrent wakeups 
 * at the time.
 * 
 * Interrupts must be disabled.
 * 
 * Example usage:
 * @code
 * void callback(waitq *wq)
 * {
 *     // Do something and notify wait_for_completion() that we're done.
 *     waitq_wakeup(wq);
 * }
 * void wait_for_completion(void) 
 * {
 *     waitq wg;
 *     waitq_initialize(&wq);
 *     // Run callback() in the background, pass it wq.
 *     do_asynchronously(callback, &wq);
 *     // Wait for callback() to complete its work.
 *     waitq_sleep(&wq);
 *     // callback() completed its work, but it may still be accessing 
 *     // wq in waitq_wakeup(). Therefore it is not yet safe to return 
 *     // from waitq_sleep() or it would clobber up our stack (where wq 
 *     // is stored). waitq_sleep() ensures the wait queue is no longer
 *     // in use by invoking waitq_complete_wakeup() internally.
 *     
 *     // waitq_sleep() returned, it is safe to free wq.
 * }
 * @endcode
 * 
 * @param wq  Pointer to a wait queue.
 */
static void waitq_complete_wakeup(waitq_t *wq)
{
	assert(interrupts_disabled());
	
	irq_spinlock_lock(&wq->lock, false);
	irq_spinlock_unlock(&wq->lock, false);
}


/** Internal SMP- and IRQ-unsafe version of waitq_wakeup()
 *
 * This is the internal SMP- and IRQ-unsafe version of waitq_wakeup(). It
 * assumes wq->lock is already locked and interrupts are already disabled.
 *
 * @param wq   Pointer to wait queue.
 * @param mode If mode is WAKEUP_FIRST, then the longest waiting
 *             thread, if any, is woken up. If mode is WAKEUP_ALL, then
 *             all waiting threads, if any, are woken up. If there are
 *             no waiting threads to be woken up, the missed wakeup is
 *             recorded in the wait queue.
 *
 */
void _waitq_wakeup_unsafe(waitq_t *wq, wakeup_mode_t mode)
{
	size_t count = 0;

	assert(interrupts_disabled());
	assert(irq_spinlock_locked(&wq->lock));
	
loop:
	if (list_empty(&wq->sleepers)) {
		wq->missed_wakeups++;
		if ((count) && (mode == WAKEUP_ALL))
			wq->missed_wakeups--;
		
		return;
	}
	
	count++;
	thread_t *thread = list_get_instance(list_first(&wq->sleepers),
	    thread_t, wq_link);
	
	/*
	 * Lock the thread prior to removing it from the wq.
	 * This is not necessary because of mutual exclusion
	 * (the link belongs to the wait queue), but because
	 * of synchronization with waitq_sleep_timed_out()
	 * and thread_interrupt_sleep().
	 *
	 * In order for these two functions to work, the following
	 * invariant must hold:
	 *
	 * thread->sleep_queue != NULL <=> thread sleeps in a wait queue
	 *
	 * For an observer who locks the thread, the invariant
	 * holds only when the lock is held prior to removing
	 * it from the wait queue.
	 *
	 */
	irq_spinlock_lock(&thread->lock, false);
	list_remove(&thread->wq_link);
	
	if ((thread->timeout_pending) &&
	    (timeout_unregister(&thread->sleep_timeout)))
		thread->timeout_pending = false;
	
	thread->sleep_queue = NULL;
	irq_spinlock_unlock(&thread->lock, false);
	
	thread_ready(thread);
	
	if (mode == WAKEUP_ALL)
		goto loop;
}

/** Get the missed wakeups count.
 *
 * @param wq	Pointer to wait queue.
 * @return	The wait queue's missed_wakeups count.
 */
int waitq_count_get(waitq_t *wq)
{
	int cnt;

	irq_spinlock_lock(&wq->lock, true);
	cnt = wq->missed_wakeups;
	irq_spinlock_unlock(&wq->lock, true);

	return cnt;
}

/** Set the missed wakeups count.
 *
 * @param wq	Pointer to wait queue.
 * @param val	New value of the missed_wakeups count.
 */
void waitq_count_set(waitq_t *wq, int val)
{
	irq_spinlock_lock(&wq->lock, true);
	wq->missed_wakeups = val;
	irq_spinlock_unlock(&wq->lock, true);
}

/** @}
 */
