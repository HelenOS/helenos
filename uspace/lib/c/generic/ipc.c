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
#include <fibril.h>
#include <macros.h>

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
 * @param label     A value to set to the label field of the answer.
 */
errno_t ipc_call_async_fast(cap_phone_handle_t phandle, sysarg_t imethod,
    sysarg_t arg1, sysarg_t arg2, sysarg_t arg3, void *label)
{
	return __SYSCALL6(SYS_IPC_CALL_ASYNC_FAST,
	    cap_handle_raw(phandle), imethod, arg1, arg2, arg3,
	    (sysarg_t) label);
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
 * @param label     A value to set to the label field of the answer.
 */
errno_t ipc_call_async_slow(cap_phone_handle_t phandle, sysarg_t imethod,
    sysarg_t arg1, sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5,
    void *label)
{
	ipc_call_t data;

	ipc_set_imethod(&data, imethod);
	ipc_set_arg1(&data, arg1);
	ipc_set_arg2(&data, arg2);
	ipc_set_arg3(&data, arg3);
	ipc_set_arg4(&data, arg4);
	ipc_set_arg5(&data, arg5);

	return __SYSCALL3(SYS_IPC_CALL_ASYNC_SLOW,
	    cap_handle_raw(phandle), (sysarg_t) &data,
	    (sysarg_t) label);
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
	    cap_handle_raw(chandle), (sysarg_t) retval, arg1, arg2, arg3, arg4);
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

	ipc_set_retval(&data, retval);
	ipc_set_arg1(&data, arg1);
	ipc_set_arg2(&data, arg2);
	ipc_set_arg3(&data, arg3);
	ipc_set_arg4(&data, arg4);
	ipc_set_arg5(&data, arg5);

	return (errno_t) __SYSCALL2(SYS_IPC_ANSWER_SLOW,
	    cap_handle_raw(chandle), (sysarg_t) &data);
}

/** Interrupt one thread of this task from waiting for IPC.
 *
 */
void ipc_poke(void)
{
	__SYSCALL0(SYS_IPC_POKE);
}

errno_t ipc_wait(ipc_call_t *call, sysarg_t usec, unsigned int flags)
{
	// TODO: Use expiration time instead of timeout.
	return __SYSCALL3(SYS_IPC_WAIT, (sysarg_t) call, usec, flags);
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
	return (errno_t) __SYSCALL1(SYS_IPC_HANGUP, cap_handle_raw(phandle));
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
	    cap_handle_raw(chandle), cap_handle_raw(phandle), imethod, arg1,
	    arg2, mode);
}

errno_t ipc_forward_slow(cap_call_handle_t chandle, cap_phone_handle_t phandle,
    sysarg_t imethod, sysarg_t arg1, sysarg_t arg2, sysarg_t arg3,
    sysarg_t arg4, sysarg_t arg5, unsigned int mode)
{
	ipc_call_t data;

	ipc_set_imethod(&data, imethod);
	ipc_set_arg1(&data, arg1);
	ipc_set_arg2(&data, arg2);
	ipc_set_arg3(&data, arg3);
	ipc_set_arg4(&data, arg4);
	ipc_set_arg5(&data, arg5);

	return (errno_t) __SYSCALL4(SYS_IPC_FORWARD_SLOW,
	    cap_handle_raw(chandle), cap_handle_raw(phandle), (sysarg_t) &data,
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
