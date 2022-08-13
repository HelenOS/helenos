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

#ifndef KERN_CONDVAR_H_
#define KERN_CONDVAR_H_

#include <stdint.h>
#include <synch/waitq.h>
#include <synch/mutex.h>
#include <synch/spinlock.h>
#include <abi/synch.h>

typedef struct {
	waitq_t wq;
} condvar_t;

#define condvar_wait(cv, mtx) \
	_condvar_wait_timeout((cv), (mtx), SYNCH_NO_TIMEOUT, SYNCH_FLAGS_NONE)
#define condvar_wait_timeout(cv, mtx, usec) \
	_condvar_wait_timeout((cv), (mtx), (usec), SYNCH_FLAGS_NONE)

#ifdef CONFIG_SMP
#define _condvar_wait_timeout_spinlock(cv, lock, usec, flags) \
	_condvar_wait_timeout_spinlock_impl((cv), (lock), (usec), (flags))
#else
#define _condvar_wait_timeout_spinlock(cv, lock, usec, flags) \
	_condvar_wait_timeout_spinlock_impl((cv), NULL, (usec), (flags))
#endif

extern void condvar_initialize(condvar_t *cv);
extern void condvar_signal(condvar_t *cv);
extern void condvar_broadcast(condvar_t *cv);
extern errno_t _condvar_wait_timeout(condvar_t *cv, mutex_t *mtx, uint32_t usec,
    int flags);
extern errno_t _condvar_wait_timeout_spinlock_impl(condvar_t *cv, spinlock_t *lock,
    uint32_t usec, int flags);
extern errno_t _condvar_wait_timeout_irq_spinlock(condvar_t *cv,
    irq_spinlock_t *irq_lock, uint32_t usec, int flags);

#endif

/** @}
 */
