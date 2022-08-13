/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

#define mutex_lock(mtx) \
	_mutex_lock_timeout((mtx), SYNCH_NO_TIMEOUT, SYNCH_FLAGS_NONE)

#define mutex_trylock(mtx) \
	_mutex_lock_timeout((mtx), SYNCH_NO_TIMEOUT, SYNCH_FLAGS_NON_BLOCKING)

#define mutex_lock_timeout(mtx, usec) \
	_mutex_lock_timeout((mtx), (usec), SYNCH_FLAGS_NON_BLOCKING)

extern void mutex_initialize(mutex_t *, mutex_type_t);
extern bool mutex_locked(mutex_t *);
extern errno_t _mutex_lock_timeout(mutex_t *, uint32_t, unsigned int);
extern void mutex_unlock(mutex_t *);

#endif

/** @}
 */
