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
typedef struct waitq {
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

	/** Number of wakeups that need to be ignored due to futex timeout. */
	int ignore_wakeups;

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
extern int waitq_count_get(waitq_t *);
extern void waitq_count_set(waitq_t *, int val);

#endif

/** @}
 */
