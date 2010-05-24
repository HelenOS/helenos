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
/** @file
 */

#ifndef KERN_RWLOCK_H_
#define KERN_RWLOCK_H_

#include <typedefs.h>
#include <synch/mutex.h>
#include <synch/synch.h>
#include <synch/spinlock.h>

typedef enum {
	RWLOCK_NONE,
	RWLOCK_READER,
	RWLOCK_WRITER
} rwlock_type_t;

typedef struct {
	IRQ_SPINLOCK_DECLARE(lock);
	
	/**
	 * Mutex for writers, readers can bypass it if readers_in is positive.
	 *
	 */
	mutex_t exclusive;
	
	/** Number of readers in critical section. */
	size_t readers_in;
} rwlock_t;

#define rwlock_write_lock(rwl) \
	_rwlock_write_lock_timeout((rwl), SYNCH_NO_TIMEOUT, SYNCH_FLAGS_NONE)

#define rwlock_read_lock(rwl) \
	_rwlock_read_lock_timeout((rwl), SYNCH_NO_TIMEOUT, SYNCH_FLAGS_NONE)

#define rwlock_write_trylock(rwl) \
	_rwlock_write_lock_timeout((rwl), SYNCH_NO_TIMEOUT, \
	    SYNCH_FLAGS_NON_BLOCKING)

#define rwlock_read_trylock(rwl) \
	_rwlock_read_lock_timeout((rwl), SYNCH_NO_TIMEOUT, \
	    SYNCH_FLAGS_NON_BLOCKING)

#define rwlock_write_lock_timeout(rwl, usec) \
	_rwlock_write_lock_timeout((rwl), (usec), SYNCH_FLAGS_NONE)

#define rwlock_read_lock_timeout(rwl, usec) \
	_rwlock_read_lock_timeout((rwl), (usec), SYNCH_FLAGS_NONE)

extern void rwlock_initialize(rwlock_t *);
extern void rwlock_read_unlock(rwlock_t *);
extern void rwlock_write_unlock(rwlock_t *);
extern int _rwlock_read_lock_timeout(rwlock_t *, uint32_t, unsigned int);
extern int _rwlock_write_lock_timeout(rwlock_t *, uint32_t, unsigned int);

#endif

/** @}
 */
