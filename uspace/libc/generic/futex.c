/*
 * Copyright (C) 2006 Jakub Jermar
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

/** @addtogroup libc
 * @{
 */
/** @file
 */ 

#include <futex.h>
#include <atomic.h>
#include <libc.h>
#include <stdio.h>
#include <types.h>
#include <kernel/synch/synch.h>

/*
 * Note about race conditions.
 * Because of non-atomic nature of operations performed sequentially on the futex
 * counter and the futex wait queue, there is a race condition:
 *
 * (wq->missed_wakeups == 1) && (futex->count = 1)
 *
 * Scenario 1 (wait queue timeout vs. futex_up()):
 * 1. assume wq->missed_wakeups == 0 && futex->count == -1
 *    (ie. thread A sleeping, thread B in the critical section)
 * 2. A receives timeout and gets removed from the wait queue
 * 3. B wants to leave the critical section and calls futex_up()
 * 4. B thus changes futex->count from -1 to 0
 * 5. B has to call SYS_FUTEX_WAKEUP syscall to wake up the sleeping thread
 * 6. B finds the wait queue empty and changes wq->missed_wakeups from 0 to 1
 * 7. A fixes futex->count (i.e. the number of waiting threads) by changing it from 0 to 1
 *
 * Scenario 2 (conditional down operation vs. futex_up)
 * 1. assume wq->missed_wakeups == 0 && futex->count == 0
 *    (i.e. thread A is in the critical section)
 * 2. thread B performs futex_trydown() operation and changes futex->count from 0 to -1
 *    B is now obliged to call SYS_FUTEX_SLEEP syscall
 * 3. A wants to leave the critical section and does futex_up()
 * 4. A thus changes futex->count from -1 to 0 and must call SYS_FUTEX_WAKEUP syscall
 * 5. B finds the wait queue empty and immediatelly aborts the conditional sleep
 * 6. No thread is queueing in the wait queue so wq->missed_wakeups changes from 0 to 1
 * 6. B fixes futex->count (i.e. the number of waiting threads) by changing it from 0 to 1
 *
 * Both scenarios allow two threads to be in the critical section simultaneously.
 * One without kernel intervention and the other through wq->missed_wakeups being 1.
 *
 * To mitigate this problem, futex_down_timeout() detects that the syscall didn't sleep
 * in the wait queue, fixes the futex counter and RETRIES the whole operation again.
 *
 */

/** Initialize futex counter.
 *
 * @param futex Futex.
 * @param val Initialization value.
 */
void futex_initialize(atomic_t *futex, int val)
{
	atomic_set(futex, val);
}

int futex_down(atomic_t *futex)
{
	return futex_down_timeout(futex, SYNCH_NO_TIMEOUT, SYNCH_FLAGS_NONE);
}

int futex_trydown(atomic_t *futex)
{
	return futex_down_timeout(futex, SYNCH_NO_TIMEOUT, SYNCH_FLAGS_NON_BLOCKING);
}

/** Try to down the futex.
 *
 * @param futex Futex.
 * @param usec Microseconds to wait. Zero value means sleep without timeout.
 * @param flags Select mode of operation. See comment for waitq_sleep_timeout(). 
 *
 * @return ENOENT if there is no such virtual address. One of ESYNCH_OK_ATOMIC
 *	   and ESYNCH_OK_BLOCKED on success or ESYNCH_TIMEOUT if the lock was
 *	   not acquired because of a timeout or ESYNCH_WOULD_BLOCK if the
 *	   operation could not be carried out atomically (if requested so).
 */
int futex_down_timeout(atomic_t *futex, uint32_t usec, int flags)
{
	int rc;
	
	while (atomic_predec(futex) < 0) {
		rc = __SYSCALL3(SYS_FUTEX_SLEEP, (sysarg_t) &futex->count, (sysarg_t) usec, (sysarg_t) flags);
		
		switch (rc) {
		case ESYNCH_OK_ATOMIC:
			/*
			 * Because of a race condition between timeout and futex_up()
			 * and between conditional futex_down_timeout() and futex_up(),
			 * we have to give up and try again in this special case.
			 */
			atomic_inc(futex);
			break;

		case ESYNCH_TIMEOUT:
			atomic_inc(futex);
			return ESYNCH_TIMEOUT;
			break;

		case ESYNCH_WOULD_BLOCK:
			/*
			 * The conditional down operation should be implemented this way.
			 * The userspace-only variant tends to accumulate missed wakeups
			 * in the kernel futex wait queue.
			 */
			atomic_inc(futex);
			return ESYNCH_WOULD_BLOCK;
			break;

		case ESYNCH_OK_BLOCKED:
			/*
			 * Enter the critical section.
			 * The futex counter has already been incremented for us.
			 */
			return ESYNCH_OK_BLOCKED;
			break;
		default:
			return rc;
		}
	}

	/*
	 * Enter the critical section.
	 */
	return ESYNCH_OK_ATOMIC;
}

/** Up the futex.
 *
 * @param futex Futex.
 *
 * @return ENOENT if there is no such virtual address. Otherwise zero.
 */
int futex_up(atomic_t *futex)
{
	long val;
	
	val = atomic_postinc(futex);
	if (val < 0)
		return __SYSCALL1(SYS_FUTEX_WAKEUP, (sysarg_t) &futex->count);
		
	return 0;
}

/** @}
 */
