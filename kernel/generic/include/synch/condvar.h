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

extern errno_t condvar_wait(condvar_t *cv, mutex_t *mtx);
extern errno_t condvar_wait_timeout(condvar_t *cv, mutex_t *mtx, uint32_t usec);

extern errno_t _condvar_wait_timeout_spinlock_impl(condvar_t *cv, spinlock_t *lock,
    uint32_t usec, int flags);
extern errno_t _condvar_wait_timeout_irq_spinlock(condvar_t *cv,
    irq_spinlock_t *irq_lock, uint32_t usec, int flags);

#endif

/** @}
 */
