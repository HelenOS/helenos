/*
 * Copyright (c) 2008 Jiri Svoboda
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

/** @addtogroup generic	
 * @{
 */
/** @file
 */

#ifndef KERN_UDEBUG_H_
#define KERN_UDEBUG_H_

#include <ipc/ipc.h>

typedef enum { /* udebug_method_t */

/** Start debugging the recipient.
 * Causes all threads in the receiving task to stop. When they
 * are all stoped, an answer with retval 0 is generated.
 */
UDEBUG_M_BEGIN = 1,

/** Finish debugging the recipient.
 * Answers all pending GO and GUARD messages.
 */
UDEBUG_M_END,

/** Set which events should be captured.
 */
UDEBUG_M_SET_EVMASK,

/** Make sure the debugged task is still there.
 * This message is answered when the debugged task dies
 * or the debugging session ends.
 */
UDEBUG_M_GUARD,

/** Run a thread until a debugging event occurs.
 * This message is answered when the thread stops
 * in a debugging event.
 *
 * - ARG2 - id of the thread to run
 */
UDEBUG_M_GO,

/** Stop a thread being debugged.
 * Creates a special STOP event in the thread, causing
 * it to answer a pending GO message (if any).
 */
UDEBUG_M_STOP,

/** Read arguments of a syscall.
 *
 * - ARG2 - thread identification
 * - ARG3 - destination address in the caller's address space
 *
 */
UDEBUG_M_ARGS_READ,

/** Read the list of the debugged tasks's threads.
 *
 * - ARG2 - destination address in the caller's address space
 * - ARG3 - size of receiving buffer in bytes
 *
 * The kernel fills the buffer with a series of sysarg_t values
 * (thread ids). On answer, the kernel will set:
 *
 * - ARG2 - number of bytes that were actually copied
 * - ARG3 - number of bytes of the complete data
 *
 */
UDEBUG_M_THREAD_READ,

/** Read the debugged tasks's memory.
 *
 * - ARG2 - destination address in the caller's address space
 * - ARG3 - source address in the recipient's address space
 * - ARG4 - size of receiving buffer in bytes
 *
 */
UDEBUG_M_MEM_READ,

} udebug_method_t;

				
typedef enum {
	UDEBUG_EVENT_FINISHED = 1,	/**< Debuging session has finished */
	UDEBUG_EVENT_STOP,		/**< Stopped on DEBUG_STOP request */
	UDEBUG_EVENT_SYSCALL_B,		/**< Before beginning syscall execution */
	UDEBUG_EVENT_SYSCALL_E,		/**< After finishing syscall execution */
	UDEBUG_EVENT_THREAD_B,		/**< The task created a new thread */
	UDEBUG_EVENT_THREAD_E		/**< A thread exited */
} udebug_event_t;

#define UDEBUG_EVMASK(event) (1 << ((event) - 1))

typedef enum {
	UDEBUG_EM_FINISHED	= UDEBUG_EVMASK(UDEBUG_EVENT_FINISHED),
	UDEBUG_EM_STOP		= UDEBUG_EVMASK(UDEBUG_EVENT_STOP),
	UDEBUG_EM_SYSCALL_B	= UDEBUG_EVMASK(UDEBUG_EVENT_SYSCALL_B),
	UDEBUG_EM_SYSCALL_E	= UDEBUG_EVMASK(UDEBUG_EVENT_SYSCALL_E),
	UDEBUG_EM_THREAD_B	= UDEBUG_EVMASK(UDEBUG_EVENT_THREAD_B),
	UDEBUG_EM_THREAD_E	= UDEBUG_EVMASK(UDEBUG_EVENT_THREAD_E),
	UDEBUG_EM_ALL		=
		UDEBUG_EVMASK(UDEBUG_EVENT_FINISHED) |
		UDEBUG_EVMASK(UDEBUG_EVENT_STOP) |
		UDEBUG_EVMASK(UDEBUG_EVENT_SYSCALL_B) |
		UDEBUG_EVMASK(UDEBUG_EVENT_SYSCALL_E) |
		UDEBUG_EVMASK(UDEBUG_EVENT_THREAD_B) |
		UDEBUG_EVMASK(UDEBUG_EVENT_THREAD_E)
} udebug_evmask_t;

#ifdef KERNEL

#include <synch/mutex.h>
#include <arch/interrupt.h>
#include <atomic.h>

typedef enum {
	/** Task is not being debugged */
	UDEBUG_TS_INACTIVE,
	/** BEGIN operation in progress (waiting for threads to stop) */
	UDEBUG_TS_BEGINNING,
	/** Debugger fully connected */
	UDEBUG_TS_ACTIVE,
	/** Task is shutting down, no more debug activities allowed */
	UDEBUG_TS_SHUTDOWN
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
	unative_t syscall_args[6];
	istate_t *uspace_state;

	/** What type of event are we stopped in or 0 if none. */
	udebug_event_t cur_event;
	bool go;	   /**< thread is GO */
	bool stoppable;	   /**< thread is stoppable */
	bool debug_active; /**< thread is in a debugging session */
} udebug_thread_t;

struct task;
struct thread;

void udebug_task_init(udebug_task_t *ut);
void udebug_thread_initialize(udebug_thread_t *ut);

void udebug_syscall_event(unative_t a1, unative_t a2, unative_t a3,
    unative_t a4, unative_t a5, unative_t a6, unative_t id, unative_t rc,
    bool end_variant);

void udebug_thread_b_event_attach(struct thread *t, struct task *ta);
void udebug_thread_e_event(void);

void udebug_stoppable_begin(void);
void udebug_stoppable_end(void);

void udebug_before_thread_runs(void);

int udebug_task_cleanup(struct task *ta);

#endif

#endif

/** @}
 */
