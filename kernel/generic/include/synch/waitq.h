/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

#ifndef __WAITQ_H__
#define __WAITQ_H__

#include <arch/types.h>
#include <typedefs.h>
#include <synch/spinlock.h>
#include <synch/synch.h>
#include <adt/list.h>

#define WAKEUP_FIRST	0
#define WAKEUP_ALL	1

/** Wait queue structure. */
struct waitq {

	/** Lock protecting wait queue structure.
	 *
	 * Must be acquired before T.lock for each T of type thread_t.
	 */
	SPINLOCK_DECLARE(lock);

	int missed_wakeups;	/**< Number of waitq_wakeup() calls that didn't find a thread to wake up. */
	link_t head;		/**< List of sleeping threads for wich there was no missed_wakeup. */
};

#define waitq_sleep(wq) \
	waitq_sleep_timeout((wq),SYNCH_NO_TIMEOUT,SYNCH_FLAGS_NONE)

extern void waitq_initialize(waitq_t *wq);
extern int waitq_sleep_timeout(waitq_t *wq, uint32_t usec, int flags);
extern ipl_t waitq_sleep_prepare(waitq_t *wq);
extern int waitq_sleep_timeout_unsafe(waitq_t *wq, uint32_t usec, int flags);
extern void waitq_sleep_finish(waitq_t *wq, int rc, ipl_t ipl);
extern void waitq_wakeup(waitq_t *wq, bool all);
extern void _waitq_wakeup_unsafe(waitq_t *wq, bool all);
extern void waitq_interrupt_sleep(thread_t *t);

#endif

 /** @}
 */

