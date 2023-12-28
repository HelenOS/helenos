/*
 * Copyright (c) 2009 Jakub Jermar
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

#ifndef _LIBC_FIBRIL_SYNCH_H_
#define _LIBC_FIBRIL_SYNCH_H_

#include <fibril.h>
#include <adt/list.h>
#include <time.h>
#include <stdbool.h>
#include <_bits/decls.h>

#ifndef __cplusplus

#define FIBRIL_MUTEX_INITIALIZER(name) \
	{ \
		.oi = { \
			.owned_by = NULL \
		}, \
		.counter = 1, \
		.waiters = LIST_INITIALIZER((name).waiters), \
	}

#define FIBRIL_MUTEX_INITIALIZE(name) \
	fibril_mutex_t name = FIBRIL_MUTEX_INITIALIZER(name)

#define FIBRIL_RWLOCK_INITIALIZER(name) \
	{ \
		.oi = { \
			.owned_by = NULL \
		}, \
		.readers = 0, \
		.writers = 0, \
		.waiters = LIST_INITIALIZER((name).waiters), \
	}

#define FIBRIL_RWLOCK_INITIALIZE(name) \
	fibril_rwlock_t name = FIBRIL_RWLOCK_INITIALIZER(name)

#define FIBRIL_CONDVAR_INITIALIZER(name) \
	{ \
		.waiters = LIST_INITIALIZER((name).waiters), \
	}

#define FIBRIL_CONDVAR_INITIALIZE(name) \
	fibril_condvar_t name = FIBRIL_CONDVAR_INITIALIZER(name)

#define FIBRIL_SEMAPHORE_INITIALIZER(name, cnt) \
	{ \
		.count = (cnt), \
		.waiters = LIST_INITIALIZER((name).waiters), \
	}

#define FIBRIL_SEMAPHORE_INITIALIZE(name, cnt) \
	fibril_semaphore_t name = FIBRIL_SEMAPHORE_INITIALIZER(name, cnt)

#endif

__HELENOS_DECLS_BEGIN;

typedef struct {
	fibril_owner_info_t oi;  /**< Keep this the first thing. */
	int counter;
	list_t waiters;
} fibril_mutex_t;

typedef struct {
	fibril_owner_info_t oi;  /**< Keep this the first thing. */
	unsigned int writers;
	unsigned int readers;
	list_t waiters;
} fibril_rwlock_t;

typedef struct {
	list_t waiters;
} fibril_condvar_t;

typedef void (*fibril_timer_fun_t)(void *);

typedef enum {
	/** Timer has not been set or has been cleared */
	fts_not_set,
	/** Timer was set but did not fire yet */
	fts_active,
	/** Timer has fired and has not been cleared since */
	fts_fired,
	/** Timer fibril is requested to terminate */
	fts_cleanup,
	/** Timer fibril acknowledged termination */
	fts_clean
} fibril_timer_state_t;

/** Fibril timer.
 *
 * When a timer is set it executes a callback function (in a separate
 * fibril) after a specified time interval. The timer can be cleared
 * (canceled) before that. From the return value of fibril_timer_clear()
 * one can tell whether the timer fired or not.
 */
typedef struct {
	fibril_mutex_t lock;
	fibril_mutex_t *lockp;
	fibril_condvar_t cv;
	fid_t fibril;
	fibril_timer_state_t state;
	/** FID of fibril executing handler or 0 if handler is not running */
	fid_t handler_fid;

	usec_t delay;
	fibril_timer_fun_t fun;
	void *arg;
} fibril_timer_t;

/** A counting semaphore for fibrils. */
typedef struct {
	long int count;
	list_t waiters;
	bool closed;
} fibril_semaphore_t;

extern void __fibril_synch_init(void);
extern void __fibril_synch_fini(void);

/** Initialize fibril mutex.
 *
 * Kept as in-line to allow constexpr marker for C++ library where this
 * is used by C++ mutex type (list initialization are two assignments
 * so it is actually reasonable to have this inlined).
 */
static inline __CONSTEXPR void fibril_mutex_initialize(fibril_mutex_t *fm)
{
	fm->oi.owned_by = NULL;
	fm->counter = 1;
	list_initialize(&fm->waiters);
}

extern void fibril_mutex_lock(fibril_mutex_t *);
extern bool fibril_mutex_trylock(fibril_mutex_t *);
extern void fibril_mutex_unlock(fibril_mutex_t *);
extern bool fibril_mutex_is_locked(fibril_mutex_t *);

extern void fibril_rwlock_initialize(fibril_rwlock_t *);
extern void fibril_rwlock_read_lock(fibril_rwlock_t *);
extern void fibril_rwlock_write_lock(fibril_rwlock_t *);
extern void fibril_rwlock_read_unlock(fibril_rwlock_t *);
extern void fibril_rwlock_write_unlock(fibril_rwlock_t *);
extern bool fibril_rwlock_is_read_locked(fibril_rwlock_t *);
extern bool fibril_rwlock_is_write_locked(fibril_rwlock_t *);
extern bool fibril_rwlock_is_locked(fibril_rwlock_t *);

extern void fibril_condvar_initialize(fibril_condvar_t *);
extern errno_t fibril_condvar_wait_timeout(fibril_condvar_t *, fibril_mutex_t *,
    usec_t);
extern void fibril_condvar_wait(fibril_condvar_t *, fibril_mutex_t *);
extern void fibril_condvar_signal(fibril_condvar_t *);
extern void fibril_condvar_broadcast(fibril_condvar_t *);

extern fibril_timer_t *fibril_timer_create(fibril_mutex_t *);
extern void fibril_timer_destroy(fibril_timer_t *);
extern void fibril_timer_set(fibril_timer_t *, usec_t, fibril_timer_fun_t,
    void *);
extern void fibril_timer_set_locked(fibril_timer_t *, usec_t,
    fibril_timer_fun_t, void *);
extern fibril_timer_state_t fibril_timer_clear(fibril_timer_t *);
extern fibril_timer_state_t fibril_timer_clear_locked(fibril_timer_t *);

extern void fibril_semaphore_initialize(fibril_semaphore_t *, long);
extern void fibril_semaphore_up(fibril_semaphore_t *);
extern void fibril_semaphore_down(fibril_semaphore_t *);
extern errno_t fibril_semaphore_down_timeout(fibril_semaphore_t *, usec_t);
extern void fibril_semaphore_close(fibril_semaphore_t *);

typedef struct mpsc mpsc_t;
extern mpsc_t *mpsc_create(size_t);
extern void mpsc_destroy(mpsc_t *);
extern errno_t mpsc_send(mpsc_t *, const void *);
extern errno_t mpsc_receive(mpsc_t *, void *, const struct timespec *);
extern void mpsc_close(mpsc_t *);

__HELENOS_DECLS_END;

#endif

/** @}
 */
