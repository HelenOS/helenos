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

#ifndef LIBC_FIBRIL_SYNCH_H_
#define LIBC_FIBRIL_SYNCH_H_

#include <fibril.h>
#include <adt/list.h>
#include <libarch/tls.h>
#include <sys/time.h>
#include <bool.h>

typedef struct {
	fibril_owner_info_t oi;  /**< Keep this the first thing. */
	int counter;
	list_t waiters;
} fibril_mutex_t;

#define FIBRIL_MUTEX_INITIALIZER(name) \
	{ \
		.oi = { \
			.owned_by = NULL \
		}, \
		.counter = 1, \
		.waiters = { \
			.head = { \
				.prev = &(name).waiters.head, \
				.next = &(name).waiters.head, \
			} \
		} \
	}
	
#define FIBRIL_MUTEX_INITIALIZE(name) \
	fibril_mutex_t name = FIBRIL_MUTEX_INITIALIZER(name) 

typedef struct {
	fibril_owner_info_t oi;  /**< Keep this the first thing. */
	unsigned writers;
	unsigned readers;
	list_t waiters;
} fibril_rwlock_t;

#define FIBRIL_RWLOCK_INITIALIZER(name) \
	{ \
		.oi = { \
			.owned_by = NULL \
		}, \
		.readers = 0, \
		.writers = 0, \
		.waiters = { \
			.head = { \
				.prev = &(name).waiters.head, \
				.next = &(name).waiters.head, \
			} \
		} \
	}

#define FIBRIL_RWLOCK_INITIALIZE(name) \
	fibril_rwlock_t name = FIBRIL_RWLOCK_INITIALIZER(name)

typedef struct {
	list_t waiters;
} fibril_condvar_t;

#define FIBRIL_CONDVAR_INITIALIZER(name) \
	{ \
		.waiters = { \
			.head = { \
				.next = &(name).waiters.head, \
				.prev = &(name).waiters.head, \
			} \
		} \
	}

#define FIBRIL_CONDVAR_INITIALIZE(name) \
	fibril_condvar_t name = FIBRIL_CONDVAR_INITIALIZER(name)

extern void fibril_mutex_initialize(fibril_mutex_t *);
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
extern int fibril_condvar_wait_timeout(fibril_condvar_t *, fibril_mutex_t *,
    suseconds_t);
extern void fibril_condvar_wait(fibril_condvar_t *, fibril_mutex_t *);
extern void fibril_condvar_signal(fibril_condvar_t *);
extern void fibril_condvar_broadcast(fibril_condvar_t *);

#endif

/** @}
 */
