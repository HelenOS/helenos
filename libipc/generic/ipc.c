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
#include <malloc.h>
#include <errno.h>
#include <list.h>
#include <stdio.h>
#include <unistd.h>

/** Structure used for keeping track of sent async msgs 
 * and queing unsent msgs
 *
 */
typedef struct {
	link_t list;

	ipc_async_callback_t callback;
	void *private;
	union {
		ipc_callid_t callid;
		struct {
			int phoneid;
			ipc_data_t data;
		} msg;
	}u;
} async_call_t;

LIST_INITIALIZE(dispatched_calls);
LIST_INITIALIZE(queued_calls);

int ipc_call_sync(int phoneid, ipcarg_t method, ipcarg_t arg1, 
		  ipcarg_t *result)
{
	ipc_data_t resdata;
	int callres;
	
	callres = __SYSCALL4(SYS_IPC_CALL_SYNC_FAST, phoneid, method, arg1,
			     (sysarg_t)&resdata);
	if (callres)
		return callres;
	if (result)
		*result = IPC_GET_ARG1(resdata);
	return IPC_GET_RETVAL(resdata);
}

int ipc_call_sync_3(int phoneid, ipcarg_t method, ipcarg_t arg1,
		    ipcarg_t arg2, ipcarg_t arg3,
		    ipcarg_t *result1, ipcarg_t *result2, ipcarg_t *result3)
{
	ipc_data_t data;
	int callres;

	IPC_SET_METHOD(data, method);
	IPC_SET_ARG1(data, arg1);
	IPC_SET_ARG2(data, arg2);
	IPC_SET_ARG3(data, arg3);

	callres = __SYSCALL3(SYS_IPC_CALL_SYNC, phoneid, (sysarg_t)&data,
			     (sysarg_t)&data);
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

/** Syscall to send asynchronous message */
static	ipc_callid_t _ipc_call_async(int phoneid, ipc_data_t *data)
{
	return __SYSCALL2(SYS_IPC_CALL_ASYNC, phoneid, (sysarg_t)data);
}

/** Send asynchronous message
 *
 * - if fatal error, call callback handler with proper error code
 * - if message cannot be temporarily sent, add to queue
 */
void ipc_call_async_2(int phoneid, ipcarg_t method, ipcarg_t arg1,
		      ipcarg_t arg2, void *private,
		      ipc_async_callback_t callback)
{
	async_call_t *call;
	ipc_callid_t callid;

	call = malloc(sizeof(*call));
	if (!call) {
		callback(private, ENOMEM, NULL);
	}
		
	callid = __SYSCALL4(SYS_IPC_CALL_ASYNC_FAST, phoneid, method, arg1, arg2);
	if (callid == IPC_CALLRET_FATAL) {
		/* Call asynchronous handler with error code */
		callback(private, ENOENT, NULL);
		free(call);
		return;
	}

	call->callback = callback;
	call->private = private;

	if (callid == IPC_CALLRET_TEMPORARY) {
		/* Add asynchronous call to queue of non-dispatched async calls */
		call->u.msg.phoneid = phoneid;
		IPC_SET_METHOD(call->u.msg.data, method);
		IPC_SET_ARG1(call->u.msg.data, arg1);
		IPC_SET_ARG2(call->u.msg.data, arg2);

		list_append(&call->list, &queued_calls);
		return;
	}
	call->u.callid = callid;
	/* Add call to list of dispatched calls */
	list_append(&call->list, &dispatched_calls);
}


/** Send answer to a received call */
void ipc_answer(ipc_callid_t callid, ipcarg_t retval, ipcarg_t arg1,
		ipcarg_t arg2)
{
	__SYSCALL4(SYS_IPC_ANSWER_FAST, callid, retval, arg1, arg2);
}


/** Call syscall function sys_ipc_wait_for_call */
static inline ipc_callid_t _ipc_wait_for_call(ipc_call_t *call, int flags)
{
	return __SYSCALL3(SYS_IPC_WAIT, (sysarg_t)&call->data, 
			  (sysarg_t)&call->taskid, flags);
}

/** Try to dispatch queed calls from async queue */
static void try_dispatch_queued_calls(void)
{
	async_call_t *call;
	ipc_callid_t callid;

	while (!list_empty(&queued_calls)) {
		call = list_get_instance(queued_calls.next, async_call_t,
					 list);

		callid = _ipc_call_async(call->u.msg.phoneid,
					 &call->u.msg.data);
		if (callid == IPC_CALLRET_TEMPORARY)
			break;
		list_remove(&call->list);
		if (callid == IPC_CALLRET_FATAL) {
			call->callback(call->private, ENOENT, NULL);
			free(call);
		} else {
			call->u.callid = callid;
			list_append(&call->list, &dispatched_calls);
		}
	}
}

/** Handle received answer
 *
 * TODO: Make it use hash table
 *
 * @param callid Callid (with first bit set) of the answered call
 */
static void handle_answer(ipc_callid_t callid, ipc_data_t *data)
{
	link_t *item;
	async_call_t *call;

	callid &= ~IPC_CALLID_ANSWERED;
	
	for (item = dispatched_calls.next; item != &dispatched_calls;
	     item = item->next) {
		call = list_get_instance(item, async_call_t, list);
		if (call->u.callid == callid) {
			list_remove(&call->list);
			call->callback(call->private, 
				       IPC_GET_RETVAL(*data),
				       data);
			return;
		}
	}
	printf("Received unidentified answer: %P!!!\n", callid);
}


/** Wait for IPC call and return
 *
 * - dispatch ASYNC reoutines in the background
 * @param data Space where the message is stored
 * @return Callid or 0 if nothing available and started with 
 *         IPC_WAIT_NONBLOCKING
 */
int ipc_wait_for_call(ipc_call_t *call, int flags)
{
	ipc_callid_t callid;

	do {
		try_dispatch_queued_calls();

		callid = _ipc_wait_for_call(call, flags);
		/* Handle received answers */
		if (callid & IPC_CALLID_ANSWERED)
			handle_answer(callid, &call->data);
	} while (callid & IPC_CALLID_ANSWERED);

	return callid;
}
