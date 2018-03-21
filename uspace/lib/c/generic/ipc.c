/*
 * Copyright (c) 2006 Ondrej Palkovsky
 * Copyright (c) 2017 Jakub Jermar
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

/** @addtogroup libc
 * @{
 * @}
 */

/** @addtogroup libcipc IPC
 * @brief HelenOS uspace IPC
 * @{
 * @ingroup libc
 */
/** @file
 */

#include <ipc/ipc.h>
#include <libc.h>
#include <stdlib.h>
#include <errno.h>
#include <adt/list.h>
#include <futex.h>
#include <fibril.h>
#include <macros.h>

/**
 * Structures of this type are used for keeping track of sent asynchronous calls.
 */
typedef struct async_call {
	ipc_async_callback_t callback;
	void *private;

	struct {
		ipc_call_t data;
	} msg;
} async_call_t;

/** Prologue for ipc_call_async_*() functions.
 *
 * @param private  Argument for the answer/error callback.
 * @param callback Answer/error callback.
 *
 * @return New, partially initialized async_call structure or NULL.
 *
 */
static inline async_call_t *ipc_prepare_async(void *private,
    ipc_async_callback_t callback)
{
	async_call_t *call =
	    (async_call_t *) malloc(sizeof(async_call_t));
	if (!call) {
		if (callback)
			callback(private, ENOMEM, NULL);

		return NULL;
	}

	call->callback = callback;
	call->private = private;

	return call;
}

/** Epilogue for ipc_call_async_*() functions.
 *
 * @param rc       Value returned by the SYS_IPC_CALL_ASYNC_* syscall.
 * @param call     Structure returned by ipc_prepare_async().
 */
static inline void ipc_finish_async(errno_t rc, async_call_t *call)
{
	if (!call) {
		/* Nothing to do regardless if failed or not */
		return;
	}

	if (rc != EOK) {
		/* Call asynchronous handler with error code */
		if (call->callback)
			call->callback(call->private, ENOENT, NULL);

		free(call);
		return;
	}
}

/** Fast asynchronous call.
 *
 * This function can only handle three arguments of payload. It is, however,
 * faster than the more generic ipc_call_async_slow().
 *
 * Note that this function is a void function.
 *
 * During normal operation, answering this call will trigger the callback.
 * In case of fatal error, the callback handler is called with the proper
 * error code. If the call cannot be temporarily made, it is queued.
 *
 * @param phandle   Phone handle for the call.
 * @param imethod   Requested interface and method.
 * @param arg1      Service-defined payload argument.
 * @param arg2      Service-defined payload argument.
 * @param arg3      Service-defined payload argument.
 * @param private   Argument to be passed to the answer/error callback.
 * @param callback  Answer or error callback.
 */
void ipc_call_async_fast(cap_phone_handle_t phandle, sysarg_t imethod,
    sysarg_t arg1, sysarg_t arg2, sysarg_t arg3, void *private,
    ipc_async_callback_t callback)
{
	async_call_t *call = ipc_prepare_async(private, callback);
	if (!call)
		return;

	errno_t rc = (errno_t) __SYSCALL6(SYS_IPC_CALL_ASYNC_FAST,
	    CAP_HANDLE_RAW(phandle), imethod, arg1, arg2, arg3,
	    (sysarg_t) call);

	ipc_finish_async(rc, call);
}

/** Asynchronous call transmitting the entire payload.
 *
 * Note that this function is a void function.
 *
 * During normal operation, answering this call will trigger the callback.
 * In case of fatal error, the callback handler is called with the proper
 * error code. If the call cannot be temporarily made, it is queued.
 *
 * @param phandle   Phone handle for the call.
 * @param imethod   Requested interface and method.
 * @param arg1      Service-defined payload argument.
 * @param arg2      Service-defined payload argument.
 * @param arg3      Service-defined payload argument.
 * @param arg4      Service-defined payload argument.
 * @param arg5      Service-defined payload argument.
 * @param private   Argument to be passed to the answer/error callback.
 * @param callback  Answer or error callback.
 */
void ipc_call_async_slow(cap_phone_handle_t phandle, sysarg_t imethod,
    sysarg_t arg1, sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5,
    void *private, ipc_async_callback_t callback)
{
	async_call_t *call = ipc_prepare_async(private, callback);
	if (!call)
		return;

	IPC_SET_IMETHOD(call->msg.data, imethod);
	IPC_SET_ARG1(call->msg.data, arg1);
	IPC_SET_ARG2(call->msg.data, arg2);
	IPC_SET_ARG3(call->msg.data, arg3);
	IPC_SET_ARG4(call->msg.data, arg4);
	IPC_SET_ARG5(call->msg.data, arg5);

	errno_t rc = (errno_t) __SYSCALL3(SYS_IPC_CALL_ASYNC_SLOW,
	    CAP_HANDLE_RAW(phandle), (sysarg_t) &call->msg.data,
	    (sysarg_t) call);

	ipc_finish_async(rc, call);
}

/** Answer received call (fast version).
 *
 * The fast answer makes use of passing retval and first four arguments in
 * registers. If you need to return more, use the ipc_answer_slow() instead.
 *
 * @param chandle  Handle of the call being answered.
 * @param retval   Return value.
 * @param arg1     First return argument.
 * @param arg2     Second return argument.
 * @param arg3     Third return argument.
 * @param arg4     Fourth return argument.
 *
 * @return Zero on success.
 * @return Value from @ref errno.h on failure.
 *
 */
errno_t ipc_answer_fast(cap_call_handle_t chandle, errno_t retval,
    sysarg_t arg1, sysarg_t arg2, sysarg_t arg3, sysarg_t arg4)
{
	return (errno_t) __SYSCALL6(SYS_IPC_ANSWER_FAST,
	    CAP_HANDLE_RAW(chandle), (sysarg_t) retval, arg1, arg2, arg3, arg4);
}

/** Answer received call (entire payload).
 *
 * @param chandle  Handle of the call being answered.
 * @param retval   Return value.
 * @param arg1     First return argument.
 * @param arg2     Second return argument.
 * @param arg3     Third return argument.
 * @param arg4     Fourth return argument.
 * @param arg5     Fifth return argument.
 *
 * @return Zero on success.
 * @return Value from @ref errno.h on failure.
 *
 */
errno_t ipc_answer_slow(cap_call_handle_t chandle, errno_t retval,
    sysarg_t arg1, sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5)
{
	ipc_call_t data;

	IPC_SET_RETVAL(data, retval);
	IPC_SET_ARG1(data, arg1);
	IPC_SET_ARG2(data, arg2);
	IPC_SET_ARG3(data, arg3);
	IPC_SET_ARG4(data, arg4);
	IPC_SET_ARG5(data, arg5);

	return (errno_t) __SYSCALL2(SYS_IPC_ANSWER_SLOW,
	    CAP_HANDLE_RAW(chandle), (sysarg_t) &data);
}

/** Handle received answer.
 *
 * @param data  Call data of the answer.
 */
static void handle_answer(ipc_call_t *data)
{
	async_call_t *call = data->label;

	if (!call)
		return;

	if (call->callback)
		call->callback(call->private, IPC_GET_RETVAL(*data), data);
	free(call);
}

/** Wait for first IPC call to come.
 *
 * @param[out] call   Storage for the received call.
 * @param[in]  usec   Timeout in microseconds
 * @param[in[  flags  Flags passed to SYS_IPC_WAIT (blocking, nonblocking).
 *
 * @return  Error code.
 */
errno_t ipc_wait_cycle(ipc_call_t *call, sysarg_t usec, unsigned int flags)
{
	errno_t rc = (errno_t) __SYSCALL3(SYS_IPC_WAIT, (sysarg_t) call, usec,
	    flags);

	/* Handle received answers */
	if ((rc == EOK) && (call->cap_handle == CAP_NIL) &&
	    (call->flags & IPC_CALL_ANSWERED)) {
		handle_answer(call);
	}

	return rc;
}

/** Interrupt one thread of this task from waiting for IPC.
 *
 */
void ipc_poke(void)
{
	__SYSCALL0(SYS_IPC_POKE);
}

/** Wait for first IPC call to come.
 *
 * Only requests are returned, answers are processed internally.
 *
 * @param call  Incoming call storage.
 * @param usec  Timeout in microseconds
 *
 * @return  Error code.
 *
 */
errno_t ipc_wait_for_call_timeout(ipc_call_t *call, sysarg_t usec)
{
	errno_t rc;

	do {
		rc = ipc_wait_cycle(call, usec, SYNCH_FLAGS_NONE);
	} while ((rc == EOK) && (call->cap_handle == CAP_NIL) &&
	    (call->flags & IPC_CALL_ANSWERED));

	return rc;
}

/** Check if there is an IPC call waiting to be picked up.
 *
 * Only requests are returned, answers are processed internally.
 *
 * @param call  Incoming call storage.
 *
 * @return  Error code.
 *
 */
errno_t ipc_trywait_for_call(ipc_call_t *call)
{
	errno_t rc;

	do {
		rc = ipc_wait_cycle(call, SYNCH_NO_TIMEOUT,
		    SYNCH_FLAGS_NON_BLOCKING);
	} while ((rc == EOK) && (call->cap_handle == CAP_NIL) &&
	    (call->flags & IPC_CALL_ANSWERED));

	return rc;
}

/** Hang up a phone.
 *
 * @param phandle  Handle of the phone to be hung up.
 *
 * @return  Zero on success or an error code.
 *
 */
errno_t ipc_hangup(cap_phone_handle_t phandle)
{
	return (errno_t) __SYSCALL1(SYS_IPC_HANGUP, CAP_HANDLE_RAW(phandle));
}

/** Forward a received call to another destination.
 *
 * For non-system methods, the old method, arg1 and arg2 are rewritten by the
 * new values. For system methods, the new method, arg1 and arg2 are written to
 * the old arg1, arg2 and arg3, respectivelly. Calls with immutable methods are
 * forwarded verbatim.
 *
 * @param chandle  Handle of the call to forward.
 * @param phandle  Phone handle to use for forwarding.
 * @param imethod  New interface and method for the forwarded call.
 * @param arg1     New value of the first argument for the forwarded call.
 * @param arg2     New value of the second argument for the forwarded call.
 * @param mode     Flags specifying mode of the forward operation.
 *
 * @return  Zero on success or an error code.
 *
 */
errno_t ipc_forward_fast(cap_call_handle_t chandle, cap_phone_handle_t phandle,
    sysarg_t imethod, sysarg_t arg1, sysarg_t arg2, unsigned int mode)
{
	return (errno_t) __SYSCALL6(SYS_IPC_FORWARD_FAST,
	    CAP_HANDLE_RAW(chandle), CAP_HANDLE_RAW(phandle), imethod, arg1,
	    arg2, mode);
}

errno_t ipc_forward_slow(cap_call_handle_t chandle, cap_phone_handle_t phandle,
    sysarg_t imethod, sysarg_t arg1, sysarg_t arg2, sysarg_t arg3,
    sysarg_t arg4, sysarg_t arg5, unsigned int mode)
{
	ipc_call_t data;

	IPC_SET_IMETHOD(data, imethod);
	IPC_SET_ARG1(data, arg1);
	IPC_SET_ARG2(data, arg2);
	IPC_SET_ARG3(data, arg3);
	IPC_SET_ARG4(data, arg4);
	IPC_SET_ARG5(data, arg5);

	return (errno_t) __SYSCALL4(SYS_IPC_FORWARD_SLOW,
	    CAP_HANDLE_RAW(chandle), CAP_HANDLE_RAW(phandle), (sysarg_t) &data,
	    mode);
}

/** Connect to a task specified by id.
 *
 */
errno_t ipc_connect_kbox(task_id_t id, cap_phone_handle_t *phone)
{
	return (errno_t) __SYSCALL2(SYS_IPC_CONNECT_KBOX, (sysarg_t) &id, (sysarg_t) phone);
}

/** @}
 */
