/*
 * Copyright (C) 2006 Ondrej Palkovsky
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

#include <ipc.h>
#include <libc.h>

int ipc_call_sync(int phoneid, sysarg_t method, sysarg_t arg1, 
		  sysarg_t *result)
{
	ipc_data_t resdata;
	int callres;
	
	callres = __SYSCALL4(SYS_IPC_CALL_SYNC, phoneid, method, arg1,
			     (sysarg_t)&resdata);
	if (callres)
		return callres;
	if (result)
		*result = IPC_GET_ARG1(resdata);
	return IPC_GET_RETVAL(resdata);
}

int ipc_call_sync_3(int phoneid, sysarg_t method, sysarg_t arg1,
		    sysarg_t arg2, sysarg_t arg3,
		    sysarg_t *result1, sysarg_t *result2, sysarg_t *result3)
{
	ipc_data_t data;
	int callres;

	IPC_SET_METHOD(data, method);
	IPC_SET_ARG1(data, arg1);
	IPC_SET_ARG2(data, arg2);
	IPC_SET_ARG3(data, arg3);

	callres = __SYSCALL2(SYS_IPC_CALL_SYNC_MEDIUM, phoneid, (sysarg_t)&data);
	if (callres)
		return callres;

	if (result1)
		*result1 = IPC_GET_ARG1(data);
	if (result2)
		*result2 = IPC_GET_ARG2(data);
	if (result3)
		*result3 = IPC_GET_ARG3(data);
	return IPC_GET_RETVAL(data);
}

/** Send asynchronous message
 *
 * - if fatal error, call callback handler with proper error code
 * - if message cannot be temporarily sent, add to queue
 */
void ipc_call_async_2(int phoneid, sysarg_t method, sysarg_t arg1,
		      sysarg_t arg2,
		      ipc_async_callback_t callback)
{
	ipc_callid_t callid;
	ipc_data_t data; /* Data storage for saving calls */

	callid = __SYSCALL4(SYS_IPC_CALL_ASYNC, phoneid, method, arg1, arg2);
	if (callid == IPC_CALLRET_FATAL) {
		/* Call asynchronous handler with error code */
		IPC_SET_RETVAL(data, IPC_CALLRET_FATAL);
		callback(&data);
		return;
	}
	if (callid == IPC_CALLRET_TEMPORARY) {
		/* Add asynchronous call to queue of non-dispatched async calls */
		IPC_SET_METHOD(data, method);
		IPC_SET_ARG1(data, arg1);
		IPC_SET_ARG2(data, arg2);
		
		return;
	}
	/* Add callid to list of dispatched calls */
	
}


/** Send answer to a received call */
void ipc_answer(ipc_callid_t callid, sysarg_t retval, sysarg_t arg1,
		sysarg_t arg2)
{
	__SYSCALL4(SYS_IPC_ANSWER, callid, retval, arg1, arg2);
}


/** Call syscall function sys_ipc_wait_for_call */
static inline ipc_callid_t _ipc_wait_for_call(ipc_data_t *data, int flags)
{
	return __SYSCALL2(SYS_IPC_WAIT, (sysarg_t)data, flags);
}

/** Wait for IPC call and return
 *
 * - dispatch ASYNC reoutines in the background
 */
int ipc_wait_for_call(ipc_data_t *data, int flags)
{
	ipc_callid_t callid;

	do {
		/* Try to dispatch non-dispatched async calls */
		callid = _ipc_wait_for_call(data, flags);
		if (callid & IPC_CALLID_ANSWERED) {
			/* TODO: Call async answer handler */
		}
	} while (callid & IPC_CALLID_ANSWERED);
	return callid;
}
