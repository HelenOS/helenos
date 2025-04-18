/*
 * Copyright (c) 2001-2004 Jakub Jermar
 * Copyright (c) 2025 Jiří Zárevúcky
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

/** @addtogroup kernel_sync
 * @{
 */

/**
 * @file
 * @brief Mutexes.
 */

#include <assert.h>
#include <errno.h>
#include <proc/thread.h>
#include <stdatomic.h>
#include <synch/mutex.h>
#include <synch/semaphore.h>

/** Initialize mutex.
 *
 * @param mtx   Mutex.
 * @param type  Type of the mutex.
 */
void mutex_initialize(mutex_t *mtx, mutex_type_t type)
{
	*mtx = MUTEX_INITIALIZER(*mtx, type);
}

/** A race in mtx->owner access is unavoidable, so we have to make
 * access to it formally atomic. These are convenience functions to
 * read/write the variable without memory barriers, since we don't need
 * them and C11 atomics default to the strongest possible memory ordering
 * by default, which is utterly ridiculous.
 */
static inline thread_t *_get_owner(mutex_t *mtx)
{
	return atomic_load_explicit(&mtx->owner, memory_order_relaxed);
}

/** Counterpart to _get_owner(). */
static inline void _set_owner(mutex_t *mtx, thread_t *owner)
{
	atomic_store_explicit(&mtx->owner, owner, memory_order_relaxed);
}

/** Find out whether the mutex is currently locked.
 *
 * @param mtx  Mutex.
 *
 * @return  True if the mutex is locked, false otherwise.
 */
bool mutex_locked(mutex_t *mtx)
{
	if (!THREAD)
		return mtx->nesting > 0;

	return _get_owner(mtx) == THREAD;
}

/** Acquire mutex.
 *
 * This operation is uninterruptible and cannot fail.
 */
void mutex_lock(mutex_t *mtx)
{
	if (!THREAD) {
		assert(mtx->type == MUTEX_RECURSIVE || mtx->nesting == 0);
		mtx->nesting++;
		return;
	}

	if (_get_owner(mtx) == THREAD) {
		/* This will also detect nested locks on a non-recursive mutex. */
		assert(mtx->type == MUTEX_RECURSIVE);
		assert(mtx->nesting > 0);
		mtx->nesting++;
		return;
	}

	semaphore_down(&mtx->sem);

	_set_owner(mtx, THREAD);
	assert(mtx->nesting == 0);
	mtx->nesting = 1;
}

/** Acquire mutex with timeout.
 *
 * @param mtx    Mutex.
 * @param usec   Timeout in microseconds.
 *
 * @return EOK if lock was successfully acquired, something else otherwise.
 */
errno_t mutex_lock_timeout(mutex_t *mtx, uint32_t usec)
{
	if (!THREAD) {
		assert(mtx->type == MUTEX_RECURSIVE || mtx->nesting == 0);
		mtx->nesting++;
		return EOK;
	}

	if (_get_owner(mtx) == THREAD) {
		assert(mtx->type == MUTEX_RECURSIVE);
		assert(mtx->nesting > 0);
		mtx->nesting++;
		return EOK;
	}

	errno_t rc = semaphore_down_timeout(&mtx->sem, usec);
	if (rc != EOK)
		return rc;

	_set_owner(mtx, THREAD);
	assert(mtx->nesting == 0);
	mtx->nesting = 1;
	return EOK;
}

/** Attempt to acquire mutex without blocking.
 *
 * @return EOK if lock was successfully acquired, something else otherwise.
 */
errno_t mutex_trylock(mutex_t *mtx)
{
	return mutex_lock_timeout(mtx, 0);
}

/** Release mutex.
 *
 * @param mtx  Mutex.
 */
void mutex_unlock(mutex_t *mtx)
{
	if (--mtx->nesting > 0) {
		assert(mtx->type == MUTEX_RECURSIVE);
		return;
	}

	assert(mtx->nesting == 0);

	if (!THREAD)
		return;

	assert(_get_owner(mtx) == THREAD);
	_set_owner(mtx, NULL);

	semaphore_up(&mtx->sem);
}

/** @}
 */
