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

/** Reader/Writer locks
 *
 * A reader/writer lock can be held by multiple readers at a time.
 * Or it can be exclusively held by a sole writer at a time.
 */

/*
 * These locks are not recursive.
 * Neither readers nor writers will suffer starvation.
 *
 * If there is a writer followed by a reader waiting for the rwlock
 * and the writer times out, all leading readers are automatically woken up
 * and allowed in.
 */

/*
 * NOTE ON rwlock_holder_type
 * This field is set on an attempt to acquire the exclusive mutex
 * to the respective value depending whether the caller is a reader
 * or a writer. The field is examined only if the thread had been
 * previously blocked on the exclusive mutex. Thus it is save
 * to store the rwlock type in the thread structure, because
 * each thread can block on only one rwlock at a time.
 */
 
#include <synch/rwlock.h>
#include <synch/spinlock.h>
#include <synch/mutex.h>
#include <synch/waitq.h>
#include <synch/synch.h>
#include <list.h>
#include <typedefs.h>
#include <arch/asm.h>
#include <arch.h>
#include <proc/thread.h>
#include <panic.h>

#define ALLOW_ALL		0
#define ALLOW_READERS_ONLY	1

static void let_others_in(rwlock_t *rwl, int readers_only);
static void release_spinlock(void *arg);

/** Initialize reader/writer lock
 *
 * Initialize reader/writer lock.
 *
 * @param rwl Reader/Writer lock.
 */
void rwlock_initialize(rwlock_t *rwl) {
	spinlock_initialize(&rwl->lock, "rwlock_t");
	mutex_initialize(&rwl->exclusive);
	rwl->readers_in = 0;
}

/** Acquire reader/writer lock for reading
 *
 * Acquire reader/writer lock for reading.
 * Timeout and willingness to block may be specified.
 *
 * @param rwl Reader/Writer lock.
 * @param usec Timeout in microseconds.
 * @param trylock Switches between blocking and non-blocking mode.
 *
 * For exact description of possible combinations of
 * 'usec' and 'trylock', see comment for waitq_sleep_timeout().
 *
 * @return See comment for waitq_sleep_timeout().
 */
int _rwlock_write_lock_timeout(rwlock_t *rwl, __u32 usec, int trylock)
{
	ipl_t ipl;
	int rc;
	
	ipl = interrupts_disable();
	spinlock_lock(&THREAD->lock);
	THREAD->rwlock_holder_type = RWLOCK_WRITER;
	spinlock_unlock(&THREAD->lock);	
	interrupts_restore(ipl);

	/*
	 * Writers take the easy part.
	 * They just need to acquire the exclusive mutex.
	 */
	rc = _mutex_lock_timeout(&rwl->exclusive, usec, trylock);
	if (SYNCH_FAILED(rc)) {

		/*
		 * Lock operation timed out.
		 * The state of rwl is UNKNOWN at this point.
		 * No claims about its holder can be made.
		 */
		 
		ipl = interrupts_disable();
		spinlock_lock(&rwl->lock);
		/*
		 * Now when rwl is locked, we can inspect it again.
		 * If it is held by some readers already, we can let
		 * readers from the head of the wait queue in.
		 */
		if (rwl->readers_in)
			let_others_in(rwl, ALLOW_READERS_ONLY);
		spinlock_unlock(&rwl->lock);
		interrupts_restore(ipl);
	}
	
	return rc;
}

/** Acquire reader/writer lock for writing
 *
 * Acquire reader/writer lock for writing.
 * Timeout and willingness to block may be specified.
 *
 * @param rwl Reader/Writer lock.
 * @param usec Timeout in microseconds.
 * @param trylock Switches between blocking and non-blocking mode.
 *
 * For exact description of possible combinations of
 * 'usec' and 'trylock', see comment for waitq_sleep_timeout().
 *
 * @return See comment for waitq_sleep_timeout().
 */
int _rwlock_read_lock_timeout(rwlock_t *rwl, __u32 usec, int trylock)
{
	int rc;
	ipl_t ipl;
	
	ipl = interrupts_disable();
	spinlock_lock(&THREAD->lock);
	THREAD->rwlock_holder_type = RWLOCK_READER;
	spinlock_unlock(&THREAD->lock);	

	spinlock_lock(&rwl->lock);

	/*
	 * Find out whether we can get what we want without blocking.
	 */
	rc = mutex_trylock(&rwl->exclusive);
	if (SYNCH_FAILED(rc)) {

		/*
		 * 'exclusive' mutex is being held by someone else.
		 * If the holder is a reader and there is no one
		 * else waiting for it, we can enter the critical
		 * section.
		 */

		if (rwl->readers_in) {
			spinlock_lock(&rwl->exclusive.sem.wq.lock);
			if (list_empty(&rwl->exclusive.sem.wq.head)) {
				/*
				 * We can enter.
				 */
				spinlock_unlock(&rwl->exclusive.sem.wq.lock);
				goto shortcut;
			}
			spinlock_unlock(&rwl->exclusive.sem.wq.lock);
		}

		/*
		 * In order to prevent a race condition when a reader
		 * could block another reader at the head of the waitq,
		 * we register a function to unlock rwl->lock
		 * after this thread is put asleep.
		 */
		thread_register_call_me(release_spinlock, &rwl->lock);
				 
		rc = _mutex_lock_timeout(&rwl->exclusive, usec, trylock);
		switch (rc) {
			case ESYNCH_WOULD_BLOCK:
				/*
				 * release_spinlock() wasn't called
				 */
				thread_register_call_me(NULL, NULL);				 
				spinlock_unlock(&rwl->lock);
			case ESYNCH_TIMEOUT:
				/*
				 * The sleep timeouted.
				 * We just restore interrupt priority level.
				 */
			case ESYNCH_OK_BLOCKED:		
				/*
				 * We were woken with rwl->readers_in already incremented.
				 * Note that this arrangement avoids race condition between
				 * two concurrent readers. (Race is avoided if 'exclusive' is
				 * locked at the same time as 'readers_in' is incremented.
				 * Same time means both events happen atomically when
				 * rwl->lock is held.)
				 */
				interrupts_restore(ipl);
				break;
			case ESYNCH_OK_ATOMIC:
				panic("_mutex_lock_timeout()==ESYNCH_OK_ATOMIC\n");
				break;
			dafault:
				panic("invalid ESYNCH\n");
				break;
		}
		return rc;
	}

shortcut:

	/*
	 * We can increment readers_in only if we didn't go to sleep.
	 * For sleepers, rwlock_let_others_in() will do the job.
	 */
	rwl->readers_in++;
	
	spinlock_unlock(&rwl->lock);
	interrupts_restore(ipl);

	return ESYNCH_OK_ATOMIC;
}

/** Release reader/writer lock held by writer
 *
 * Release reader/writer lock held by writer.
 * Handoff reader/writer lock ownership directly
 * to waiting readers or a writer.
 *
 * @param rwl Reader/Writer lock.
 */
void rwlock_write_unlock(rwlock_t *rwl)
{
	ipl_t ipl;
	
	ipl = interrupts_disable();
	spinlock_lock(&rwl->lock);
	let_others_in(rwl, ALLOW_ALL);
	spinlock_unlock(&rwl->lock);
	interrupts_restore(ipl);
	
}

/** Release reader/writer lock held by reader
 *
 * Release reader/writer lock held by reader.
 * Handoff reader/writer lock ownership directly
 * to a waiting writer or don't do anything if more
 * readers poses the lock.
 *
 * @param rwl Reader/Writer lock.
 */
void rwlock_read_unlock(rwlock_t *rwl)
{
	ipl_t ipl;

	ipl = interrupts_disable();
	spinlock_lock(&rwl->lock);
	if (!--rwl->readers_in)
		let_others_in(rwl, ALLOW_ALL);
	spinlock_unlock(&rwl->lock);
	interrupts_restore(ipl);
}


/** Direct handoff of reader/writer lock ownership.
 *
 * Direct handoff of reader/writer lock ownership
 * to waiting readers or a writer.
 *
 * Must be called with rwl->lock locked.
 * Must be called with interrupts_disable()'d.
 *
 * @param rwl Reader/Writer lock.
 * @param readers_only See the description below.
 *
 * If readers_only is false: (unlock scenario)
 * Let the first sleeper on 'exclusive' mutex in, no matter
 * whether it is a reader or a writer. If there are more leading
 * readers in line, let each of them in.
 *
 * Otherwise: (timeout scenario)
 * Let all leading readers in.
 */
void let_others_in(rwlock_t *rwl, int readers_only)
{
	rwlock_type_t type = RWLOCK_NONE;
	thread_t *t = NULL;
	bool one_more = true;
	
	spinlock_lock(&rwl->exclusive.sem.wq.lock);

	if (!list_empty(&rwl->exclusive.sem.wq.head))
		t = list_get_instance(rwl->exclusive.sem.wq.head.next, thread_t, wq_link);
	do {
		if (t) {
			spinlock_lock(&t->lock);
			type = t->rwlock_holder_type;
			spinlock_unlock(&t->lock);			
		}
	
		/*
		 * If readers_only is true, we wake all leading readers
		 * if and only if rwl is locked by another reader.
		 * Assumption: readers_only ==> rwl->readers_in
		 */
		if (readers_only && (type != RWLOCK_READER))
			break;


		if (type == RWLOCK_READER) {
			/*
			 * Waking up a reader.
			 * We are responsible for incrementing rwl->readers_in for it.
			 */
			 rwl->readers_in++;
		}

		/*
		 * Only the last iteration through this loop can increment
		 * rwl->exclusive.sem.wq.missed_wakeup's. All preceeding
		 * iterations will wake up a thread.
		 */
		/* We call the internal version of waitq_wakeup, which
		 * relies on the fact that the waitq is already locked.
		 */
		_waitq_wakeup_unsafe(&rwl->exclusive.sem.wq, WAKEUP_FIRST);
		
		t = NULL;
		if (!list_empty(&rwl->exclusive.sem.wq.head)) {
			t = list_get_instance(rwl->exclusive.sem.wq.head.next, thread_t, wq_link);
			if (t) {
				spinlock_lock(&t->lock);
				if (t->rwlock_holder_type != RWLOCK_READER)
					one_more = false;
				spinlock_unlock(&t->lock);	
			}
		}
	} while ((type == RWLOCK_READER) && t && one_more);

	spinlock_unlock(&rwl->exclusive.sem.wq.lock);
}

/** Release spinlock callback
 *
 * This is a callback function invoked from the scheduler.
 * The callback is registered in _rwlock_read_lock_timeout().
 *
 * @param arg Spinlock.
 */
void release_spinlock(void *arg)
{
	spinlock_unlock((spinlock_t *) arg);
}
