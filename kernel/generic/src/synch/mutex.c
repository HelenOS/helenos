/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sync
 * @{
 */

/**
 * @file
 * @brief Mutexes.
 */

#include <assert.h>
#include <errno.h>
#include <synch/mutex.h>
#include <synch/semaphore.h>
#include <arch.h>
#include <stacktrace.h>
#include <cpu.h>
#include <proc/thread.h>

/** Initialize mutex.
 *
 * @param mtx   Mutex.
 * @param type  Type of the mutex.
 */
void mutex_initialize(mutex_t *mtx, mutex_type_t type)
{
	mtx->type = type;
	mtx->owner = NULL;
	mtx->nesting = 0;
	semaphore_initialize(&mtx->sem, 1);
}

/** Find out whether the mutex is currently locked.
 *
 * @param mtx  Mutex.
 *
 * @return  True if the mutex is locked, false otherwise.
 */
bool mutex_locked(mutex_t *mtx)
{
	return semaphore_count_get(&mtx->sem) <= 0;
}

#define MUTEX_DEADLOCK_THRESHOLD	100000000

/** Acquire mutex.
 *
 * Timeout mode and non-blocking mode can be requested.
 *
 * @param mtx    Mutex.
 * @param usec   Timeout in microseconds.
 * @param flags  Specify mode of operation.
 *
 * For exact description of possible combinations of usec and flags, see
 * comment for waitq_sleep_timeout().
 *
 * @return See comment for waitq_sleep_timeout().
 *
 */
errno_t _mutex_lock_timeout(mutex_t *mtx, uint32_t usec, unsigned int flags)
{
	errno_t rc;

	if (mtx->type == MUTEX_PASSIVE && THREAD) {
		rc = _semaphore_down_timeout(&mtx->sem, usec, flags);
	} else if (mtx->type == MUTEX_RECURSIVE) {
		assert(THREAD);

		if (mtx->owner == THREAD) {
			mtx->nesting++;
			return EOK;
		} else {
			rc = _semaphore_down_timeout(&mtx->sem, usec, flags);
			if (rc == EOK) {
				mtx->owner = THREAD;
				mtx->nesting = 1;
			}
		}
	} else {
		assert((mtx->type == MUTEX_ACTIVE) || !THREAD);
		assert(usec == SYNCH_NO_TIMEOUT);
		assert(!(flags & SYNCH_FLAGS_INTERRUPTIBLE));

		unsigned int cnt = 0;
		bool deadlock_reported = false;
		do {
			if (cnt++ > MUTEX_DEADLOCK_THRESHOLD) {
				printf("cpu%u: looping on active mutex %p\n",
				    CPU->id, mtx);
				stack_trace();
				cnt = 0;
				deadlock_reported = true;
			}
			rc = semaphore_trydown(&mtx->sem);
		} while (rc != EOK && !(flags & SYNCH_FLAGS_NON_BLOCKING));
		if (deadlock_reported)
			printf("cpu%u: not deadlocked\n", CPU->id);
	}

	return rc;
}

/** Release mutex.
 *
 * @param mtx  Mutex.
 */
void mutex_unlock(mutex_t *mtx)
{
	if (mtx->type == MUTEX_RECURSIVE) {
		assert(mtx->owner == THREAD);
		if (--mtx->nesting > 0)
			return;
		mtx->owner = NULL;
	}
	semaphore_up(&mtx->sem);
}

/** @}
 */
