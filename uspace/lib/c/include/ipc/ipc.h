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

#include <ipc/common.h>
#include <abi/ipc/methods.h>
#include <abi/synch.h>
#include <abi/proc/task.h>
#include <abi/cap.h>

typedef void (*ipc_async_callback_t)(void *, errno_t, ipc_call_t *);

extern errno_t ipc_wait_cycle(ipc_call_t *, sysarg_t, unsigned int);
extern void ipc_poke(void);

#define ipc_wait_for_call(data) \
	ipc_wait_for_call_timeout(data, SYNCH_NO_TIMEOUT);

extern errno_t ipc_wait_for_call_timeout(ipc_call_t *, sysarg_t);
extern errno_t ipc_trywait_for_call(ipc_call_t *);

/*
 * User-friendly wrappers for ipc_answer_fast() and ipc_answer_slow().
 * They are in the form of ipc_answer_m(), where m is the number of return
 * arguments. The macros decide between the fast and the slow version according
 * to m.
 */

#define ipc_answer_0(chandle, retval) \
	ipc_answer_fast((chandle), (retval), 0, 0, 0, 0)
#define ipc_answer_1(chandle, retval, arg1) \
	ipc_answer_fast((chandle), (retval), (arg1), 0, 0, 0)
#define ipc_answer_2(chandle, retval, arg1, arg2) \
	ipc_answer_fast((chandle), (retval), (arg1), (arg2), 0, 0)
#define ipc_answer_3(chandle, retval, arg1, arg2, arg3) \
	ipc_answer_fast((chandle), (retval), (arg1), (arg2), (arg3), 0)
#define ipc_answer_4(chandle, retval, arg1, arg2, arg3, arg4) \
	ipc_answer_fast((chandle), (retval), (arg1), (arg2), (arg3), (arg4))
#define ipc_answer_5(chandle, retval, arg1, arg2, arg3, arg4, arg5) \
	ipc_answer_slow((chandle), (retval), (arg1), (arg2), (arg3), (arg4), \
	    (arg5))

extern errno_t ipc_answer_fast(cap_handle_t, errno_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t);
extern errno_t ipc_answer_slow(cap_handle_t, errno_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t);

/*
 * User-friendly wrappers for ipc_call_async_fast() and ipc_call_async_slow().
 * They are in the form of ipc_call_async_m(), where m is the number of payload
 * arguments. The macros decide between the fast and the slow version according
 * to m.
 */

#define ipc_call_async_0(phandle, method, private, callback) \
	ipc_call_async_fast((phandle), (method), 0, 0, 0, (private), (callback))
#define ipc_call_async_1(phandle, method, arg1, private, callback) \
	ipc_call_async_fast((phandle), (method), (arg1), 0, 0, (private), \
	    (callback))
#define ipc_call_async_2(phandle, method, arg1, arg2, private, callback) \
	ipc_call_async_fast((phandle), (method), (arg1), (arg2), 0, \
	    (private), (callback))
#define ipc_call_async_3(phandle, method, arg1, arg2, arg3, private, callback) \
	ipc_call_async_fast((phandle), (method), (arg1), (arg2), (arg3), \
	    (private), (callback))
#define ipc_call_async_4(phandle, method, arg1, arg2, arg3, arg4, private, \
    callback) \
	ipc_call_async_slow((phandle), (method), (arg1), (arg2), (arg3), \
	    (arg4), 0, (private), (callback))
#define ipc_call_async_5(phandle, method, arg1, arg2, arg3, arg4, arg5, \
    private, callback) \
	ipc_call_async_slow((phandle), (method), (arg1), (arg2), (arg3), \
	    (arg4), (arg5), (private), (callback))

extern void ipc_call_async_fast(cap_handle_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, void *, ipc_async_callback_t);
extern void ipc_call_async_slow(cap_handle_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, void *, ipc_async_callback_t);

extern errno_t ipc_hangup(cap_handle_t);

extern errno_t ipc_forward_fast(cap_handle_t, cap_handle_t, sysarg_t, sysarg_t,
    sysarg_t, unsigned int);
extern errno_t ipc_forward_slow(cap_handle_t, cap_handle_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, sysarg_t, unsigned int);

extern errno_t ipc_connect_kbox(task_id_t, cap_handle_t *);

#endif

/** @}
 */
