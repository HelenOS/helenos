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

#ifndef ABI_UDEBUG_H_
#define ABI_UDEBUG_H_

#define UDEBUG_EVMASK(event)  (1 << ((event) - 1))

typedef enum { /* udebug_method_t */

	/** Start debugging the recipient.
	 *
	 * Causes all threads in the receiving task to stop. When they
	 * are all stoped, an answer with retval 0 is generated.
	 *
	 */
	UDEBUG_M_BEGIN = 1,

	/** Finish debugging the recipient.
	 *
	 * Answers all pending GO and GUARD messages.
	 *
	 */
	UDEBUG_M_END,

	/** Set which events should be captured. */
	UDEBUG_M_SET_EVMASK,

	/** Make sure the debugged task is still there.
	 *
	 * This message is answered when the debugged task dies
	 * or the debugging session ends.
	 *
	 */
	UDEBUG_M_GUARD,

	/** Run a thread until a debugging event occurs.
	 *
	 * This message is answered when the thread stops
	 * in a debugging event.
	 *
	 * - ARG2 - id of the thread to run
	 *
	 */
	UDEBUG_M_GO,

	/** Stop a thread being debugged.
	 *
	 * Creates a special STOP event in the thread, causing
	 * it to answer a pending GO message (if any).
	 *
	 */
	UDEBUG_M_STOP,

	/** Read arguments of a syscall.
	 *
	 * - ARG2 - thread identification
	 * - ARG3 - destination address in the caller's address space
	 *
	 */
	UDEBUG_M_ARGS_READ,

	/** Read thread's userspace register state (istate_t).
	 *
	 * - ARG2 - thread identification
	 * - ARG3 - destination address in the caller's address space
	 *
	 * or, on error, retval will be
	 * - ENOENT - thread does not exist
	 * - EBUSY - register state not available
	 */
	UDEBUG_M_REGS_READ,

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

	/** Read the name of the debugged task.
	 *
	 * - ARG2 - destination address in the caller's address space
	 * - ARG3 - size of receiving buffer in bytes
	 *
	 * The kernel fills the buffer with a non-terminated string.
	 *
	 * - ARG2 - number of bytes that were actually copied
	 * - ARG3 - number of bytes of the complete data
	 *
	 */
	UDEBUG_M_NAME_READ,

	/** Read the list of the debugged task's address space areas.
	 *
	 * - ARG2 - destination address in the caller's address space
	 * - ARG3 - size of receiving buffer in bytes
	 *
	 * The kernel fills the buffer with a series of as_area_info_t structures.
	 * Upon answer, the kernel will set:
	 *
	 * - ARG2 - number of bytes that were actually copied
	 * - ARG3 - number of bytes of the complete data
	 *
	 */
	UDEBUG_M_AREAS_READ,

	/** Read the debugged tasks's memory.
	 *
	 * - ARG2 - destination address in the caller's address space
	 * - ARG3 - source address in the recipient's address space
	 * - ARG4 - size of receiving buffer in bytes
	 *
	 */
	UDEBUG_M_MEM_READ
} udebug_method_t;

typedef enum {
	UDEBUG_EVENT_FINISHED = 1,  /**< Debuging session has finished */
	UDEBUG_EVENT_STOP,          /**< Stopped on DEBUG_STOP request */
	UDEBUG_EVENT_SYSCALL_B,     /**< Before beginning syscall execution */
	UDEBUG_EVENT_SYSCALL_E,     /**< After finishing syscall execution */
	UDEBUG_EVENT_THREAD_B,      /**< The task created a new thread */
	UDEBUG_EVENT_THREAD_E       /**< A thread exited */
} udebug_event_t;

typedef enum {
	UDEBUG_EM_FINISHED = UDEBUG_EVMASK(UDEBUG_EVENT_FINISHED),
	UDEBUG_EM_STOP = UDEBUG_EVMASK(UDEBUG_EVENT_STOP),
	UDEBUG_EM_SYSCALL_B = UDEBUG_EVMASK(UDEBUG_EVENT_SYSCALL_B),
	UDEBUG_EM_SYSCALL_E = UDEBUG_EVMASK(UDEBUG_EVENT_SYSCALL_E),
	UDEBUG_EM_THREAD_B = UDEBUG_EVMASK(UDEBUG_EVENT_THREAD_B),
	UDEBUG_EM_THREAD_E = UDEBUG_EVMASK(UDEBUG_EVENT_THREAD_E),
	UDEBUG_EM_ALL =
	    (UDEBUG_EVMASK(UDEBUG_EVENT_FINISHED) |
	    UDEBUG_EVMASK(UDEBUG_EVENT_STOP) |
	    UDEBUG_EVMASK(UDEBUG_EVENT_SYSCALL_B) |
	    UDEBUG_EVMASK(UDEBUG_EVENT_SYSCALL_E) |
	    UDEBUG_EVMASK(UDEBUG_EVENT_THREAD_B) |
	    UDEBUG_EVMASK(UDEBUG_EVENT_THREAD_E))
} udebug_evmask_t;

#endif

/** @}
 */
