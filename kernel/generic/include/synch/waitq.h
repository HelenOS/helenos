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

#ifndef KERN_WAITQ_H_
#define KERN_WAITQ_H_

#include <typedefs.h>
#include <synch/spinlock.h>
#include <abi/synch.h>
#include <adt/list.h>

/** Wait queue structure.
 *
 */
typedef struct waitq {
	/** Lock protecting wait queue structure.
	 *
	 * Must be acquired before T.lock for each T of type thread_t.
	 */
	IRQ_SPINLOCK_DECLARE(lock);

	/**
	 * If negative, number of wakeups that are to be ignored (necessary for futex operation).
	 * If positive, number of wakeups that weren't able to wake a thread.
	 */
	int wakeup_balance;

	/** List of sleeping threads for which there was no missed_wakeup. */
	list_t sleepers;

	bool closed;
} waitq_t;

typedef struct wait_guard {
	ipl_t ipl;
} wait_guard_t;

struct thread;

extern void waitq_initialize(waitq_t *);
extern void waitq_initialize_with_count(waitq_t *, int);
extern errno_t waitq_sleep(waitq_t *);
extern errno_t _waitq_sleep_timeout(waitq_t *, uint32_t, unsigned int);
extern errno_t waitq_sleep_timeout(waitq_t *, uint32_t);
extern wait_guard_t waitq_sleep_prepare(waitq_t *);
extern errno_t waitq_sleep_unsafe(waitq_t *, wait_guard_t);
extern errno_t waitq_sleep_timeout_unsafe(waitq_t *, uint32_t, unsigned int, wait_guard_t);

extern void waitq_wake_one(waitq_t *);
extern void waitq_wake_all(waitq_t *);
extern void waitq_signal(waitq_t *);
extern void waitq_close(waitq_t *);

#endif

/** @}
 */
