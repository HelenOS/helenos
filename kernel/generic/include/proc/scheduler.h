/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_proc
 * @{
 */
/** @file
 */

#ifndef KERN_SCHEDULER_H_
#define KERN_SCHEDULER_H_

#include <stddef.h>
#include <synch/spinlock.h>
#include <time/clock.h>
#include <atomic.h>
#include <adt/list.h>

#define RQ_COUNT          16
#define NEEDS_RELINK_MAX  (HZ)

/** Scheduler run queue structure. */
typedef struct {
	IRQ_SPINLOCK_DECLARE(lock);
	list_t rq;			/**< List of ready threads. */
	size_t n;			/**< Number of threads in rq_ready. */
} runq_t;

extern atomic_size_t nrdy;
extern void scheduler_init(void);

extern void scheduler_fpu_lazy_request(void);
extern void scheduler(void);
extern void kcpulb(void *arg);

extern void sched_print_list(void);

/*
 * To be defined by architectures.
 */
extern void before_task_runs_arch(void);
extern void before_thread_runs_arch(void);
extern void after_thread_ran_arch(void);

#endif

/** @}
 */
