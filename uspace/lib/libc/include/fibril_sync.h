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

#ifndef LIBC_FIBRIL_SYNC_H_
#define LIBC_FIBRIL_SYNC_H_

#include <async.h>
#include <fibril.h>
#include <adt/list.h>
#include <libarch/tls.h>

typedef struct {
	int counter;
	link_t waiters;
} fibril_mutex_t;

#define FIBRIL_MUTEX_INITIALIZE(name) \
	fibril_mutex_t name = {	\
		.counter = 1, \
		.waiters = { \
			.prev = &name.waiters, \
			.next = &name.waiters, \
		} \
	}

typedef struct {
	unsigned writers;
	unsigned readers;
	link_t waiters;
} fibril_rwlock_t;

#define FIBRIL_RWLOCK_INITIALIZE(name) \
	fibril_rwlock_t name = { \
		.readers = 0, \
		.writers = 0, \
		.waiters = { \
			.prev = &name.waiters, \
			.next = &name.waiters, \
		} \
	}

typedef struct {
	link_t waiters;
} fibril_condvar_t;

#define FIBRIL_CONDVAR_INITIALIZE(name) \
	fibril_condvar_t name = { \
		.waiters = { \
			.next = &name.waiters, \
			.prev = &name.waiters, \
		} \
	}

extern void fibril_mutex_initialize(fibril_mutex_t *);
extern void fibril_mutex_lock(fibril_mutex_t *);
extern bool fibril_mutex_trylock(fibril_mutex_t *);
extern void fibril_mutex_unlock(fibril_mutex_t *);

extern void fibril_rwlock_initialize(fibril_rwlock_t *);
extern void fibril_rwlock_read_lock(fibril_rwlock_t *);
extern void fibril_rwlock_write_lock(fibril_rwlock_t *);
extern void fibril_rwlock_read_unlock(fibril_rwlock_t *);
extern void fibril_rwlock_write_unlock(fibril_rwlock_t *);

extern void fibril_condvar_initialize(fibril_condvar_t *);
extern void fibril_condvar_wait(fibril_condvar_t *, fibril_mutex_t *);
extern void fibril_condvar_signal(fibril_condvar_t *);
extern void fibril_condvar_broadcast(fibril_condvar_t *);

#endif

/** @}
 */
