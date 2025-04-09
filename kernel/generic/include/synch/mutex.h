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

/** @addtogroup kernel_sync
 * @{
 */
/** @file
 */

#ifndef KERN_MUTEX_H_
#define KERN_MUTEX_H_

#include <stdbool.h>
#include <stdint.h>
#include <synch/semaphore.h>
#include <abi/synch.h>

typedef enum {
	MUTEX_PASSIVE,
	MUTEX_RECURSIVE,
	MUTEX_ACTIVE
} mutex_type_t;

struct thread;

typedef struct {
	mutex_type_t type;
	semaphore_t sem;
	struct thread *owner;
	unsigned nesting;
} mutex_t;

#define MUTEX_INITIALIZER(name, mtype) (mutex_t) { \
	.type = (mtype), \
	.sem = SEMAPHORE_INITIALIZER((name).sem, 1), \
	.owner = NULL, \
	.nesting = 0, \
}

#define MUTEX_INITIALIZE(name, mtype) \
	mutex_t name = MUTEX_INITIALIZER(name, mtype)

extern void mutex_initialize(mutex_t *, mutex_type_t);
extern bool mutex_locked(mutex_t *);
extern errno_t mutex_trylock(mutex_t *);
extern void mutex_lock(mutex_t *);
extern errno_t mutex_lock_timeout(mutex_t *, uint32_t);
extern void mutex_unlock(mutex_t *);

#endif

/** @}
 */
