/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_UDEBUG_H_
#define KERN_UDEBUG_H_

#include <abi/udebug.h>
#include <ipc/ipc.h>
#include <synch/mutex.h>
#include <synch/condvar.h>
#include <arch/interrupt.h>
#include <atomic.h>

typedef enum {
	/** Task is not being debugged */
	UDEBUG_TS_INACTIVE,
	/** BEGIN operation in progress (waiting for threads to stop) */
	UDEBUG_TS_BEGINNING,
	/** Debugger fully connected */
	UDEBUG_TS_ACTIVE
} udebug_task_state_t;

/** Debugging part of task_t structure.
 */
typedef struct {
	/** Synchronize debug ops on this task / access to this structure */
	mutex_t lock;
	char *lock_owner;

	udebug_task_state_t dt_state;
	call_t *begin_call;
	int not_stoppable_count;
	struct task *debugger;
	udebug_evmask_t evmask;
} udebug_task_t;

/** Debugging part of thread_t structure.
 */
typedef struct {
	/** Synchronize debug ops on this thread / access to this structure. */
	mutex_t lock;

	waitq_t go_wq;
	call_t *go_call;
	sysarg_t syscall_args[6];
	istate_t *uspace_state;

	/** What type of event are we stopped in or 0 if none. */
	udebug_event_t cur_event;
	bool go;         /**< Thread is GO */
	bool stoppable;  /**< Thread is stoppable */
	bool active;     /**< Thread is in a debugging session */
	condvar_t active_cv;
} udebug_thread_t;

struct task;
struct thread;

void udebug_task_init(udebug_task_t *);
void udebug_thread_initialize(udebug_thread_t *);

void udebug_syscall_event(sysarg_t, sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, bool);

void udebug_thread_b_event_attach(struct thread *, struct task *);
void udebug_thread_e_event(void);

void udebug_stoppable_begin(void);
void udebug_stoppable_end(void);

void udebug_before_thread_runs(void);

errno_t udebug_task_cleanup(struct task *);
void udebug_thread_fault(void);

#endif

/** @}
 */
