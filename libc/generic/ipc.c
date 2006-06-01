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

#include <ipc/ipc.h>
#include <libc.h>
#include <malloc.h>
#include <errno.h>
#include <libadt/list.h>
#include <stdio.h>
#include <unistd.h>
#include <futex.h>
#include <kernel/synch/synch.h>
#include <async.h>
#include <psthread.h>

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
			ipc_call_t data;
			int phoneid;
		} msg;
	}u;
	pstid_t ptid;   /**< Thread waiting for sending this msg */
} async_call_t;

LIST_INITIALIZE(dispatched_calls);

/* queued_calls is protcted by async_futex, because if the
 * call cannot be sent into kernel, async framework is used
 * automatically
 */
LIST_INITIALIZE(queued_calls); /**< List of async calls that were not accepted
				*   by kernel */

static atomic_t ipc_futex = FUTEX_INITIALIZER;

int ipc_call_sync(int phoneid, ipcarg_t method, ipcarg_t arg1, 
		  ipcarg_t *result)
{
	ipc_call_t resdata;
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
	ipc_call_t data;
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
static	ipc_callid_t _ipc_call_async(int phoneid, ipc_call_t *data)
{
	return __SYSCALL2(SYS_IPC_CALL_ASYNC, phoneid, (sysarg_t)data);
}

/** Prolog to ipc_async_send functions */
static inline async_call_t *ipc_prepare_async(void *private, ipc_async_callback_t callback)
{
	async_call_t *call;

	call = malloc(sizeof(*call));
	if (!call) {
		if (callback)
			callback(private, ENOMEM, NULL);
		return NULL;
	}
	call->callback = callback;
	call->private = private;

	return call;
}

/** Epilogue of ipc_async_send functions */
static inline void ipc_finish_async(ipc_callid_t callid, int phoneid, async_call_t *call)
{
	if (callid == IPC_CALLRET_FATAL) {
		futex_up(&ipc_futex);
		/* Call asynchronous handler with error code */
		if (call->callback)
			call->callback(call->private, ENOENT, NULL);
		free(call);
		return;
	}

	if (callid == IPC_CALLRET_TEMPORARY) {
		futex_up(&ipc_futex);

		call->u.msg.phoneid = phoneid;

		call->ptid = psthread_get_id();
		futex_down(&async_futex);
		list_append(&call->list, &queued_calls);

		psthread_schedule_next_adv(PS_TO_MANAGER);
		/* Async futex unlocked by previous call */
		return;
	}
	call->u.callid = callid;
	/* Add call to list of dispatched calls */
	list_append(&call->list, &dispatched_calls);
	futex_up(&ipc_futex);
	
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

	call = ipc_prepare_async(private, callback);
	if (!call)
		return;

	/* We need to make sure that we get callid before
	 * another thread accesses the queue again */
	futex_down(&ipc_futex);
	callid = __SYSCALL4(SYS_IPC_CALL_ASYNC_FAST, phoneid, method, arg1, arg2);

	if (callid == IPC_CALLRET_TEMPORARY) {
		IPC_SET_METHOD(call->u.msg.data, method);
		IPC_SET_ARG1(call->u.msg.data, arg1);
		IPC_SET_ARG2(call->u.msg.data, arg2);
	}
	ipc_finish_async(callid, phoneid, call);
}

/** Send asynchronous message
 *
 * - if fatal error, call callback handler with proper error code
 * - if message cannot be temporarily sent, add to queue
 */
void ipc_call_async_3(int phoneid, ipcarg_t method, ipcarg_t arg1,
		      ipcarg_t arg2, ipcarg_t arg3, void *private,
		      ipc_async_callback_t callback)
{
	async_call_t *call;
	ipc_callid_t callid;

	call = ipc_prepare_async(private, callback);
	if (!call)
		return;

	IPC_SET_METHOD(call->u.msg.data, method);
	IPC_SET_ARG1(call->u.msg.data, arg1);
	IPC_SET_ARG2(call->u.msg.data, arg2);
	IPC_SET_ARG3(call->u.msg.data, arg3);
	/* We need to make sure that we get callid before
	 * another thread accesses the queue again */
	futex_down(&ipc_futex);
	callid = _ipc_call_async(phoneid, &call->u.msg.data);

	ipc_finish_async(callid, phoneid, call);
}


/** Send a fast answer to a received call.
 *
 * The fast answer makes use of passing retval and first two arguments in registers.
 * If you need to return more, use the ipc_answer() instead.
 *
 * @param callid ID of the call being answered.
 * @param retval Return value.
 * @param arg1 First return argument.
 * @param arg2 Second return argument.
 *
 * @return Zero on success or a value from @ref errno.h on failure.
 */
ipcarg_t ipc_answer_fast(ipc_callid_t callid, ipcarg_t retval, ipcarg_t arg1,
		ipcarg_t arg2)
{
	return __SYSCALL4(SYS_IPC_ANSWER_FAST, callid, retval, arg1, arg2);
}

/** Send a full answer to a received call.
 *
 * @param callid ID of the call being answered.
 * @param call Call data. Must be already initialized by the responder.
 *
 * @return Zero on success or a value from @ref errno.h on failure.
 */
ipcarg_t ipc_answer(ipc_callid_t callid, ipc_call_t *call)
{
	return __SYSCALL2(SYS_IPC_ANSWER, callid, (sysarg_t) call);
}


/** Try to dispatch queed calls from async queue */
static void try_dispatch_queued_calls(void)
{
	async_call_t *call;
	ipc_callid_t callid;

	/* TODO: integrate intelligently ipc_futex, so that it
	 * is locked during ipc_call_async, until it is added
	 * to dispatched_calls
	 */
	futex_down(&async_futex);
	while (!list_empty(&queued_calls)) {
		call = list_get_instance(queued_calls.next, async_call_t,
					 list);

		callid = _ipc_call_async(call->u.msg.phoneid,
					 &call->u.msg.data);
		if (callid == IPC_CALLRET_TEMPORARY) {
			break;
		}
		list_remove(&call->list);

		futex_up(&async_futex);
		psthread_add_ready(call->ptid);
		
		if (callid == IPC_CALLRET_FATAL) {
			if (call->callback)
				call->callback(call->private, ENOENT, NULL);
			free(call);
		} else {
			call->u.callid = callid;
			futex_down(&ipc_futex);
			list_append(&call->list, &dispatched_calls);
			futex_up(&ipc_futex);
		}
		futex_down(&async_futex);
	}
	futex_up(&async_futex);
}

/** Handle received answer
 *
 * TODO: Make it use hash table
 *
 * @param callid Callid (with first bit set) of the answered call
 */
static void handle_answer(ipc_callid_t callid, ipc_call_t *data)
{
	link_t *item;
	async_call_t *call;

	callid &= ~IPC_CALLID_ANSWERED;
	
	futex_down(&ipc_futex);
	for (item = dispatched_calls.next; item != &dispatched_calls;
	     item = item->next) {
		call = list_get_instance(item, async_call_t, list);
		if (call->u.callid == callid) {
			list_remove(&call->list);
			futex_up(&ipc_futex);
			if (call->callback)
				call->callback(call->private, 
					       IPC_GET_RETVAL(*data),
					       data);
			free(call);
			return;
		}
	}
	futex_up(&ipc_futex);
	printf("Received unidentified answer: %P!!!\n", callid);
}


/** One cycle of ipc wait for call call
 *
 * - dispatch ASYNC reoutines in the background
 * @param call Space where the message is stored
 * @param usec Timeout in microseconds
 * @param flags Flags passed to SYS_IPC_WAIT (blocking, nonblocking)
 * @return Callid of the answer.
 */
ipc_callid_t ipc_wait_cycle(ipc_call_t *call, uint32_t usec, int flags)
{
	ipc_callid_t callid;

	callid = __SYSCALL3(SYS_IPC_WAIT, (sysarg_t) call, usec, flags);
	/* Handle received answers */
	if (callid & IPC_CALLID_ANSWERED) {
		handle_answer(callid, call);
		try_dispatch_queued_calls();
	}

	return callid;
}

/** Wait some time for an IPC call.
 *
 * - dispatch ASYNC reoutines in the background
 * @param call Space where the message is stored
 * @param usec Timeout in microseconds.
 * @return Callid of the answer.
 */
ipc_callid_t ipc_wait_for_call_timeout(ipc_call_t *call, uint32_t usec)
{
	ipc_callid_t callid;

	do {
		callid = ipc_wait_cycle(call, usec, SYNCH_BLOCKING);
	} while (callid & IPC_CALLID_ANSWERED);

	return callid;
}

/** Check if there is an IPC call waiting to be picked up.
 *
 * - dispatch ASYNC reoutines in the background
 * @param call Space where the message is stored
 * @return Callid of the answer.
 */
ipc_callid_t ipc_trywait_for_call(ipc_call_t *call)
{
	ipc_callid_t callid;

	do {
		callid = ipc_wait_cycle(call, SYNCH_NO_TIMEOUT, SYNCH_NON_BLOCKING);
	} while (callid & IPC_CALLID_ANSWERED);

	return callid;
}

/** Ask destination to do a callback connection
 *
 * @return 0 - OK, error code
 */
int ipc_connect_to_me(int phoneid, int arg1, int arg2, ipcarg_t *phone)
{
	return ipc_call_sync_3(phoneid, IPC_M_CONNECT_TO_ME, arg1,
			       arg2, 0, 0, 0, phone);
}

/** Ask through phone for a new connection to some service
 *
 * @return new phoneid - OK, error code
 */
int ipc_connect_me_to(int phoneid, int arg1, int arg2)
{
	ipcarg_t newphid;
	int res;

	res =  ipc_call_sync_3(phoneid, IPC_M_CONNECT_ME_TO, arg1,
			       arg2, 0, 0, 0, &newphid);
	if (res)
		return res;
	return newphid;
}

/* Hang up specified phone */
int ipc_hangup(int phoneid)
{
	return __SYSCALL1(SYS_IPC_HANGUP, phoneid);
}

int ipc_register_irq(int irq, irq_code_t *ucode)
{
	return __SYSCALL2(SYS_IPC_REGISTER_IRQ, irq, (sysarg_t) ucode);
}

int ipc_unregister_irq(int irq)
{
	return __SYSCALL1(SYS_IPC_UNREGISTER_IRQ, irq);
}

int ipc_forward_fast(ipc_callid_t callid, int phoneid, int method, ipcarg_t arg1)
{
	return __SYSCALL4(SYS_IPC_FORWARD_FAST, callid, phoneid, method, arg1);
}


/** Open shared memory connection over specified phoneid
 *
 * 
 * Allocate as_area, notify the other side about our intention
 * to open the connection
 *
 * @return Connection id identifying this connection
 */
//int ipc_dgr_open(int pohoneid, size_t bufsize)
//{
	/* Find new file descriptor in local descriptor table */
	/* Create AS_area, initialize structures */
	/* Send AS to other side, handle error states */

//}
/*
void ipc_dgr_close(int cid)
{
}

void * ipc_dgr_alloc(int cid, size_t size)
{
}

void ipc_dgr_free(int cid, void *area)
{

}

int ipc_dgr_send(int cid, void *area)
{
}


int ipc_dgr_send_data(int cid, void *data, size_t size)
{
}

*/
