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

#ifndef KERN_SEMAPHORE_H_
#define KERN_SEMAPHORE_H_

#include <errno.h>
#include <stdint.h>
#include <synch/waitq.h>
#include <abi/synch.h>

typedef struct {
	waitq_t wq;
} semaphore_t;

#define semaphore_down(s) \
	_semaphore_down_timeout((s), SYNCH_NO_TIMEOUT, SYNCH_FLAGS_NONE)

#define semaphore_trydown(s) \
	_semaphore_down_timeout((s), SYNCH_NO_TIMEOUT, SYNCH_FLAGS_NON_BLOCKING)

#define semaphore_down_timeout(s, usec) \
	_semaphore_down_timeout((s), (usec), SYNCH_FLAGS_NONE)

#define semaphore_down_interruptable(s) \
	(_semaphore_down_timeout((s), SYNCH_NO_TIMEOUT, \
		SYNCH_FLAGS_INTERRUPTIBLE) != EINTR)

extern void semaphore_initialize(semaphore_t *, int);
extern errno_t _semaphore_down_timeout(semaphore_t *, uint32_t, unsigned int);
extern void semaphore_up(semaphore_t *);
extern int semaphore_count_get(semaphore_t *);

#endif

/** @}
 */
