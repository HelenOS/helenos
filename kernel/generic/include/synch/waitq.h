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

#ifndef KERN_WAITQ_H_
#define KERN_WAITQ_H_

#include <typedefs.h>
#include <synch/spinlock.h>
#include <abi/synch.h>
#include <adt/list.h>

typedef enum {
	WAKEUP_FIRST = 0,
	WAKEUP_ALL
} wakeup_mode_t;

/** Wait queue structure.
 *
 */
typedef struct {
	/** Lock protecting wait queue structure.
	 *
	 * Must be acquired before T.lock for each T of type thread_t.
	 */
	IRQ_SPINLOCK_DECLARE(lock);

	/**
	 * Number of waitq_wakeup() calls that didn't find a thread to wake up.
	 *
	 */
	int missed_wakeups;

	/** List of sleeping threads for which there was no missed_wakeup. */
	list_t sleepers;
} waitq_t;

#define waitq_sleep(wq) \
	waitq_sleep_timeout((wq), SYNCH_NO_TIMEOUT, SYNCH_FLAGS_NONE, NULL)

struct thread;

extern void waitq_initialize(waitq_t *);
extern errno_t waitq_sleep_timeout(waitq_t *, uint32_t, unsigned int, bool *);
extern ipl_t waitq_sleep_prepare(waitq_t *);
extern errno_t waitq_sleep_timeout_unsafe(waitq_t *, uint32_t, unsigned int, bool *);
extern void waitq_sleep_finish(waitq_t *, bool, ipl_t);
extern void waitq_wakeup(waitq_t *, wakeup_mode_t);
extern void _waitq_wakeup_unsafe(waitq_t *, wakeup_mode_t);
extern void waitq_interrupt_sleep(struct thread *);
extern void waitq_unsleep(waitq_t *);
extern int waitq_count_get(waitq_t *);
extern void waitq_count_set(waitq_t *, int val);

#endif

/** @}
 */
