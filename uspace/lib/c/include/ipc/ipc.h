/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

/** @addtogroup libcipc
 * @{
 */
/** @file
 */

#if ((defined(LIBC_ASYNC_H_)) && (!defined(LIBC_ASYNC_C_)))
	#error Do not intermix low-level IPC interface and async framework
#endif

#ifndef LIBC_IPC_H_
#define LIBC_IPC_H_

#include <sys/types.h>
#include <ipc/common.h>
#include <abi/ipc/methods.h>
#include <abi/synch.h>
#include <abi/proc/task.h>

typedef void (*ipc_async_callback_t)(void *, int, ipc_call_t *);

extern ipc_callid_t ipc_wait_cycle(ipc_call_t *, sysarg_t, unsigned int);
extern void ipc_poke(void);

#define ipc_wait_for_call(data) \
	ipc_wait_for_call_timeout(data, SYNCH_NO_TIMEOUT);

extern ipc_callid_t ipc_wait_for_call_timeout(ipc_call_t *, sysarg_t);
extern ipc_callid_t ipc_trywait_for_call(ipc_call_t *);

/*
 * User-friendly wrappers for ipc_answer_fast() and ipc_answer_slow().
 * They are in the form of ipc_answer_m(), where m is the number of return
 * arguments. The macros decide between the fast and the slow version according
 * to m.
 */

#define ipc_answer_0(callid, retval) \
	ipc_answer_fast((callid), (retval), 0, 0, 0, 0)
#define ipc_answer_1(callid, retval, arg1) \
	ipc_answer_fast((callid), (retval), (arg1), 0, 0, 0)
#define ipc_answer_2(callid, retval, arg1, arg2) \
	ipc_answer_fast((callid), (retval), (arg1), (arg2), 0, 0)
#define ipc_answer_3(callid, retval, arg1, arg2, arg3) \
	ipc_answer_fast((callid), (retval), (arg1), (arg2), (arg3), 0)
#define ipc_answer_4(callid, retval, arg1, arg2, arg3, arg4) \
	ipc_answer_fast((callid), (retval), (arg1), (arg2), (arg3), (arg4))
#define ipc_answer_5(callid, retval, arg1, arg2, arg3, arg4, arg5) \
	ipc_answer_slow((callid), (retval), (arg1), (arg2), (arg3), (arg4), (arg5))

extern sysarg_t ipc_answer_fast(ipc_callid_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t);
extern sysarg_t ipc_answer_slow(ipc_callid_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t);

/*
 * User-friendly wrappers for ipc_call_async_fast() and ipc_call_async_slow().
 * They are in the form of ipc_call_async_m(), where m is the number of payload
 * arguments. The macros decide between the fast and the slow version according
 * to m.
 */

#define ipc_call_async_0(phoneid, method, private, callback, can_preempt) \
	ipc_call_async_fast((phoneid), (method), 0, 0, 0, 0, (private), \
	    (callback), (can_preempt))
#define ipc_call_async_1(phoneid, method, arg1, private, callback, \
    can_preempt) \
	ipc_call_async_fast((phoneid), (method), (arg1), 0, 0, 0, (private), \
	    (callback), (can_preempt))
#define ipc_call_async_2(phoneid, method, arg1, arg2, private, callback, \
    can_preempt) \
	ipc_call_async_fast((phoneid), (method), (arg1), (arg2), 0, 0, \
	    (private), (callback), (can_preempt))
#define ipc_call_async_3(phoneid, method, arg1, arg2, arg3, private, callback, \
    can_preempt) \
	ipc_call_async_fast((phoneid), (method), (arg1), (arg2), (arg3), 0, \
	    (private), (callback), (can_preempt))
#define ipc_call_async_4(phoneid, method, arg1, arg2, arg3, arg4, private, \
    callback, can_preempt) \
	ipc_call_async_fast((phoneid), (method), (arg1), (arg2), (arg3), \
	    (arg4), (private), (callback), (can_preempt))
#define ipc_call_async_5(phoneid, method, arg1, arg2, arg3, arg4, arg5, \
    private, callback, can_preempt) \
	ipc_call_async_slow((phoneid), (method), (arg1), (arg2), (arg3), \
	    (arg4), (arg5), (private), (callback), (can_preempt))

extern void ipc_call_async_fast(int, sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, void *, ipc_async_callback_t, bool);
extern void ipc_call_async_slow(int, sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, void *, ipc_async_callback_t, bool);

extern int ipc_hangup(int);

extern int ipc_forward_fast(ipc_callid_t, int, sysarg_t, sysarg_t, sysarg_t,
    unsigned int);
extern int ipc_forward_slow(ipc_callid_t, int, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, unsigned int);

extern int ipc_connect_kbox(task_id_t);

#endif

/** @}
 */
