/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcipc
 * @{
 */
/** @file
 */

#if ((defined(_LIBC_ASYNC_H_)) && (!defined(_LIBC_ASYNC_C_)))
#error Do not intermix low-level IPC interface and async framework
#endif

#ifndef _LIBC_IPC_H_
#define _LIBC_IPC_H_

#include <ipc/common.h>
#include <abi/ipc/methods.h>
#include <abi/synch.h>
#include <abi/proc/task.h>
#include <abi/cap.h>

extern errno_t ipc_wait(ipc_call_t *, sysarg_t, unsigned int);
extern void ipc_poke(void);

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

extern errno_t ipc_answer_fast(cap_call_handle_t, errno_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t);
extern errno_t ipc_answer_slow(cap_call_handle_t, errno_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t);

/*
 * User-friendly wrappers for ipc_call_async_fast() and ipc_call_async_slow().
 * They are in the form of ipc_call_async_m(), where m is the number of payload
 * arguments. The macros decide between the fast and the slow version according
 * to m.
 */

#define ipc_call_async_0(phandle, method, label) \
	ipc_call_async_fast((phandle), (method), 0, 0, 0, (label))
#define ipc_call_async_1(phandle, method, arg1, label) \
	ipc_call_async_fast((phandle), (method), (arg1), 0, 0, (label))
#define ipc_call_async_2(phandle, method, arg1, arg2, label) \
	ipc_call_async_fast((phandle), (method), (arg1), (arg2), 0, (label))
#define ipc_call_async_3(phandle, method, arg1, arg2, arg3, label) \
	ipc_call_async_fast((phandle), (method), (arg1), (arg2), (arg3), \
	    (label))
#define ipc_call_async_4(phandle, method, arg1, arg2, arg3, arg4, label) \
	ipc_call_async_slow((phandle), (method), (arg1), (arg2), (arg3), \
	    (arg4), 0, (label))
#define ipc_call_async_5(phandle, method, arg1, arg2, arg3, arg4, arg5, \
    label) \
	ipc_call_async_slow((phandle), (method), (arg1), (arg2), (arg3), \
	    (arg4), (arg5), (label))

extern errno_t ipc_call_async_fast(cap_phone_handle_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, void *);
extern errno_t ipc_call_async_slow(cap_phone_handle_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, sysarg_t, void *);

extern errno_t ipc_hangup(cap_phone_handle_t);

extern errno_t ipc_forward_fast(cap_call_handle_t, cap_phone_handle_t, sysarg_t,
    sysarg_t, sysarg_t, unsigned int);
extern errno_t ipc_forward_slow(cap_call_handle_t, cap_phone_handle_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t, sysarg_t, sysarg_t, unsigned int);

extern errno_t ipc_connect_kbox(task_id_t, cap_phone_handle_t *);

#endif

/** @}
 */
