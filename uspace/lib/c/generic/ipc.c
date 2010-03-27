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
#include <malloc.h>
#include <errno.h>
#include <adt/list.h>
#include <stdio.h>
#include <unistd.h>
#include <futex.h>
#include <kernel/synch/synch.h>
#include <async.h>
#include <fibril.h>
#include <assert.h>

/**
 * Structures of this type are used for keeping track of sent asynchronous calls
 * and queing unsent calls.
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
	} u;
	fid_t fid;	/**< Fibril waiting for sending this call. */
} async_call_t;

LIST_INITIALIZE(dispatched_calls);

/** List of asynchronous calls that were not accepted by kernel.
 *
 * It is protected by async_futex, because if the call cannot be sent into the
 * kernel, the async framework is used automatically.
 */
LIST_INITIALIZE(queued_calls);

static atomic_t ipc_futex = FUTEX_INITIALIZER;

/** Make a fast synchronous call.
 *
 * Only three payload arguments can be passed using this function. However, this
 * function is faster than the generic ipc_call_sync_slow() because the payload
 * is passed directly in registers.
 *
 * @param phoneid	Phone handle for the call.
 * @param method	Requested method.
 * @param arg1		Service-defined payload argument.
 * @param arg2		Service-defined payload argument.
 * @param arg3		Service-defined payload argument.
 * @param result1	If non-NULL, the return ARG1 will be stored there.
 * @param result2	If non-NULL, the return ARG2 will be stored there.
 * @param result3	If non-NULL, the return ARG3 will be stored there.
 * @param result4	If non-NULL, the return ARG4 will be stored there.
 * @param result5	If non-NULL, the return ARG5 will be stored there.
 *
 * @return		Negative values represent errors returned by IPC.
 *			Otherwise the RETVAL of the answer is returned.
 */
int
ipc_call_sync_fast(int phoneid, ipcarg_t method, ipcarg_t arg1, ipcarg_t arg2,
    ipcarg_t arg3, ipcarg_t *result1, ipcarg_t *result2, ipcarg_t *result3,
    ipcarg_t *result4, ipcarg_t *result5)
{
	ipc_call_t resdata;
	int callres;
	
	callres = __SYSCALL6(SYS_IPC_CALL_SYNC_FAST, phoneid, method, arg1,
	    arg2, arg3, (sysarg_t) &resdata);
	if (callres)
		return callres;
	if (result1)
		*result1 = IPC_GET_ARG1(resdata);
	if (result2)
		*result2 = IPC_GET_ARG2(resdata);
	if (result3)
		*result3 = IPC_GET_ARG3(resdata);
	if (result4)
		*result4 = IPC_GET_ARG4(resdata);
	if (result5)
		*result5 = IPC_GET_ARG5(resdata);

	return IPC_GET_RETVAL(resdata);
}

/** Make a synchronous call transmitting 5 arguments of payload.
 *
 * @param phoneid	Phone handle for the call.
 * @param method	Requested method.
 * @param arg1		Service-defined payload argument.
 * @param arg2		Service-defined payload argument.
 * @param arg3		Service-defined payload argument.
 * @param arg4		Service-defined payload argument.
 * @param arg5		Service-defined payload argument.
 * @param result1	If non-NULL, storage for the first return argument.
 * @param result2	If non-NULL, storage for the second return argument.
 * @param result3	If non-NULL, storage for the third return argument.
 * @param result4	If non-NULL, storage for the fourth return argument.
 * @param result5	If non-NULL, storage for the fifth return argument.
 *
 * @return		Negative value means IPC error.
 *			Otherwise the RETVAL of the answer.
 */
int
ipc_call_sync_slow(int phoneid, ipcarg_t method, ipcarg_t arg1, ipcarg_t arg2,
    ipcarg_t arg3, ipcarg_t arg4, ipcarg_t arg5, ipcarg_t *result1,
    ipcarg_t *result2, ipcarg_t *result3, ipcarg_t *result4, ipcarg_t *result5)
{
	ipc_call_t data;
	int callres;

	IPC_SET_METHOD(data, method);
	IPC_SET_ARG1(data, arg1);
	IPC_SET_ARG2(data, arg2);
	IPC_SET_ARG3(data, arg3);
	IPC_SET_ARG4(data, arg4);
	IPC_SET_ARG5(data, arg5);

	callres = __SYSCALL3(SYS_IPC_CALL_SYNC_SLOW, phoneid, (sysarg_t) &data,
	    (sysarg_t) &data);
	if (callres)
		return callres;

	if (result1)
		*result1 = IPC_GET_ARG1(data);
	if (result2)
		*result2 = IPC_GET_ARG2(data);
	if (result3)
		*result3 = IPC_GET_ARG3(data);
	if (result4)
		*result4 = IPC_GET_ARG4(data);
	if (result5)
		*result5 = IPC_GET_ARG5(data);

	return IPC_GET_RETVAL(data);
}

/** Syscall to send asynchronous message.
 *
 * @param phoneid	Phone handle for the call.
 * @param data		Call data with the request.
 *
 * @return		Hash of the call or an error code.
 */
static ipc_callid_t _ipc_call_async(int phoneid, ipc_call_t *data)
{
	return __SYSCALL2(SYS_IPC_CALL_ASYNC_SLOW, phoneid, (sysarg_t) data);
}

/** Prolog to ipc_call_async_*() functions.
 *
 * @param private	Argument for the answer/error callback.
 * @param callback	Answer/error callback.
 *
 * @return		New, partially initialized async_call structure or NULL.
 */
static inline async_call_t *ipc_prepare_async(void *private,
    ipc_async_callback_t callback)
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

/** Epilogue of ipc_call_async_*() functions.
 *
 * @param callid	Value returned by the SYS_IPC_CALL_ASYNC_* syscall.
 * @param phoneid	Phone handle through which the call was made.
 * @param call		async_call structure returned by ipc_prepare_async().
 * @param can_preempt	If non-zero, the current fibril can be preempted in this
 *			call.
 */
static inline void ipc_finish_async(ipc_callid_t callid, int phoneid,
    async_call_t *call, int can_preempt)
{
	if (!call) { /* Nothing to do regardless if failed or not */
		futex_up(&ipc_futex);
		return;
	}

	if (callid == (ipc_callid_t) IPC_CALLRET_FATAL) {
		futex_up(&ipc_futex);
		/* Call asynchronous handler with error code */
		if (call->callback)
			call->callback(call->private, ENOENT, NULL);
		free(call);
		return;
	}

	if (callid == (ipc_callid_t) IPC_CALLRET_TEMPORARY) {
		futex_up(&ipc_futex);

		call->u.msg.phoneid = phoneid;
		
		futex_down(&async_futex);
		list_append(&call->list, &queued_calls);

		if (can_preempt) {
			call->fid = fibril_get_id();
			fibril_switch(FIBRIL_TO_MANAGER);
			/* Async futex unlocked by previous call */
		} else {
			call->fid = 0;
			futex_up(&async_futex);
		}
		return;
	}
	call->u.callid = callid;
	/* Add call to the list of dispatched calls */
	list_append(&call->list, &dispatched_calls);
	futex_up(&ipc_futex);
	
}

/** Make a fast asynchronous call.
 *
 * This function can only handle four arguments of payload. It is, however,
 * faster than the more generic ipc_call_async_slow().
 *
 * Note that this function is a void function.
 * During normal opertation, answering this call will trigger the callback.
 * In case of fatal error, call the callback handler with the proper error code.
 * If the call cannot be temporarily made, queue it.
 *
 * @param phoneid	Phone handle for the call.
 * @param method	Requested method.
 * @param arg1		Service-defined payload argument.
 * @param arg2		Service-defined payload argument.
 * @param arg3		Service-defined payload argument.
 * @param arg4		Service-defined payload argument.
 * @param private	Argument to be passed to the answer/error callback.
 * @param callback	Answer or error callback.
 * @param can_preempt	If non-zero, the current fibril will be preempted in
 *			case the kernel temporarily refuses to accept more
 *			asynchronous calls.
 */
void ipc_call_async_fast(int phoneid, ipcarg_t method, ipcarg_t arg1,
    ipcarg_t arg2, ipcarg_t arg3, ipcarg_t arg4, void *private,
    ipc_async_callback_t callback, int can_preempt)
{
	async_call_t *call = NULL;
	ipc_callid_t callid;

	if (callback) {
		call = ipc_prepare_async(private, callback);
		if (!call)
			return;
	}

	/*
	 * We need to make sure that we get callid before another thread
	 * accesses the queue again.
	 */
	futex_down(&ipc_futex);
	callid = __SYSCALL6(SYS_IPC_CALL_ASYNC_FAST, phoneid, method, arg1,
	    arg2, arg3, arg4);

	if (callid == (ipc_callid_t) IPC_CALLRET_TEMPORARY) {
		if (!call) {
			call = ipc_prepare_async(private, callback);
			if (!call)
				return;
		}
		IPC_SET_METHOD(call->u.msg.data, method);
		IPC_SET_ARG1(call->u.msg.data, arg1);
		IPC_SET_ARG2(call->u.msg.data, arg2);
		IPC_SET_ARG3(call->u.msg.data, arg3);
		IPC_SET_ARG4(call->u.msg.data, arg4);
		/*
		 * To achieve deterministic behavior, we always zero out the
		 * arguments that are beyond the limits of the fast version.
		 */
		IPC_SET_ARG5(call->u.msg.data, 0);
	}
	ipc_finish_async(callid, phoneid, call, can_preempt);
}

/** Make an asynchronous call transmitting the entire payload.
 *
 * Note that this function is a void function.
 * During normal opertation, answering this call will trigger the callback.
 * In case of fatal error, call the callback handler with the proper error code.
 * If the call cannot be temporarily made, queue it.
 *
 * @param phoneid	Phone handle for the call.
 * @param method	Requested method.
 * @param arg1		Service-defined payload argument.
 * @param arg2		Service-defined payload argument.
 * @param arg3		Service-defined payload argument.
 * @param arg4		Service-defined payload argument.
 * @param arg5		Service-defined payload argument.
 * @param private	Argument to be passed to the answer/error callback.
 * @param callback	Answer or error callback.
 * @param can_preempt	If non-zero, the current fibril will be preempted in
 *			case the kernel temporarily refuses to accept more
 *			asynchronous calls.
 *
 */
void ipc_call_async_slow(int phoneid, ipcarg_t method, ipcarg_t arg1,
    ipcarg_t arg2, ipcarg_t arg3, ipcarg_t arg4, ipcarg_t arg5, void *private,
    ipc_async_callback_t callback, int can_preempt)
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
	IPC_SET_ARG4(call->u.msg.data, arg4);
	IPC_SET_ARG5(call->u.msg.data, arg5);
	/*
	 * We need to make sure that we get callid before another thread
	 * accesses the queue again.
	 */
	futex_down(&ipc_futex);
	callid = _ipc_call_async(phoneid, &call->u.msg.data);

	ipc_finish_async(callid, phoneid, call, can_preempt);
}


/** Answer a received call - fast version.
 *
 * The fast answer makes use of passing retval and first four arguments in
 * registers. If you need to return more, use the ipc_answer_slow() instead.
 *
 * @param callid	Hash of the call being answered.
 * @param retval	Return value.
 * @param arg1		First return argument.
 * @param arg2		Second return argument.
 * @param arg3		Third return argument.
 * @param arg4		Fourth return argument.
 *
 * @return		Zero on success or a value from @ref errno.h on failure.
 */
ipcarg_t ipc_answer_fast(ipc_callid_t callid, ipcarg_t retval, ipcarg_t arg1,
    ipcarg_t arg2, ipcarg_t arg3, ipcarg_t arg4)
{
	return __SYSCALL6(SYS_IPC_ANSWER_FAST, callid, retval, arg1, arg2, arg3,
	    arg4);
}

/** Answer a received call - slow full version.
 *
 * @param callid	Hash of the call being answered.
 * @param retval	Return value.
 * @param arg1		First return argument.
 * @param arg2		Second return argument.
 * @param arg3		Third return argument.
 * @param arg4		Fourth return argument.
 * @param arg5		Fifth return argument.
 *
 * @return		Zero on success or a value from @ref errno.h on failure.
 */
ipcarg_t ipc_answer_slow(ipc_callid_t callid, ipcarg_t retval, ipcarg_t arg1,
    ipcarg_t arg2, ipcarg_t arg3, ipcarg_t arg4, ipcarg_t arg5)
{
	ipc_call_t data;

	IPC_SET_RETVAL(data, retval);
	IPC_SET_ARG1(data, arg1);
	IPC_SET_ARG2(data, arg2);
	IPC_SET_ARG3(data, arg3);
	IPC_SET_ARG4(data, arg4);
	IPC_SET_ARG5(data, arg5);

	return __SYSCALL2(SYS_IPC_ANSWER_SLOW, callid, (sysarg_t) &data);
}


/** Try to dispatch queued calls from the async queue. */
static void try_dispatch_queued_calls(void)
{
	async_call_t *call;
	ipc_callid_t callid;

	/** @todo
	 * Integrate intelligently ipc_futex, so that it is locked during
	 * ipc_call_async_*(), until it is added to dispatched_calls.
	 */
	futex_down(&async_futex);
	while (!list_empty(&queued_calls)) {
		call = list_get_instance(queued_calls.next, async_call_t, list);
		callid = _ipc_call_async(call->u.msg.phoneid,
		    &call->u.msg.data);
		if (callid == (ipc_callid_t) IPC_CALLRET_TEMPORARY) {
			break;
		}
		list_remove(&call->list);

		futex_up(&async_futex);
		if (call->fid)
			fibril_add_ready(call->fid);
		
		if (callid == (ipc_callid_t) IPC_CALLRET_FATAL) {
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

/** Handle a received answer.
 *
 * Find the hash of the answer and call the answer callback.
 *
 * @todo Make it use hash table.
 *
 * @param callid	Hash of the received answer.
 *			The answer has the same hash as the request OR'ed with
 *			the IPC_CALLID_ANSWERED bit.
 * @param data		Call data of the answer.
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
				    IPC_GET_RETVAL(*data), data);
			free(call);
			return;
		}
	}
	futex_up(&ipc_futex);
}


/** Wait for a first call to come.
 *
 * @param call		Storage where the incoming call data will be stored.
 * @param usec		Timeout in microseconds
 * @param flags		Flags passed to SYS_IPC_WAIT (blocking, nonblocking).
 *
 * @return		Hash of the call. Note that certain bits have special
 *			meaning. IPC_CALLID_ANSWERED will be set in an answer
 *			and IPC_CALLID_NOTIFICATION is used for notifications.
 *			
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
 * The call will return after an answer is received.
 *
 * @param call		Storage where the incoming call data will be stored.
 * @param usec		Timeout in microseconds.
 *
 * @return		Hash of the answer.
 */
ipc_callid_t ipc_wait_for_call_timeout(ipc_call_t *call, uint32_t usec)
{
	ipc_callid_t callid;

	do {
		callid = ipc_wait_cycle(call, usec, SYNCH_FLAGS_NONE);
	} while (callid & IPC_CALLID_ANSWERED);

	return callid;
}

/** Check if there is an IPC call waiting to be picked up.
 *
 * @param call		Storage where the incoming call will be stored.
 * @return		Hash of the answer.
 */
ipc_callid_t ipc_trywait_for_call(ipc_call_t *call)
{
	ipc_callid_t callid;

	do {
		callid = ipc_wait_cycle(call, SYNCH_NO_TIMEOUT,
		    SYNCH_FLAGS_NON_BLOCKING);
	} while (callid & IPC_CALLID_ANSWERED);

	return callid;
}

/** Interrupt one thread of this task from waiting for IPC. */
void ipc_poke(void)
{
	__SYSCALL0(SYS_IPC_POKE);
}

/** Ask destination to do a callback connection.
 *
 * @param phoneid	Phone handle used for contacting the other side.
 * @param arg1		Service-defined argument.
 * @param arg2		Service-defined argument.
 * @param arg3		Service-defined argument.
 * @param phonehash	Storage where the library will store an opaque
 *			identifier of the phone that will be used for incoming
 *			calls. This identifier can be used for connection
 *			tracking.
 *
 * @return		Zero on success or a negative error code.
 */
int ipc_connect_to_me(int phoneid, int arg1, int arg2, int arg3, 
    ipcarg_t *phonehash)
{
	return ipc_call_sync_3_5(phoneid, IPC_M_CONNECT_TO_ME, arg1, arg2,
	    arg3, NULL, NULL, NULL, NULL, phonehash);
}

/** Ask through phone for a new connection to some service.
 *
 * @param phoneid	Phone handle used for contacting the other side.
 * @param arg1		User defined argument.
 * @param arg2		User defined argument.
 * @param arg3		User defined argument.
 *
 * @return		New phone handle on success or a negative error code.
 */
int ipc_connect_me_to(int phoneid, int arg1, int arg2, int arg3)
{
	ipcarg_t newphid;
	int res;

	res = ipc_call_sync_3_5(phoneid, IPC_M_CONNECT_ME_TO, arg1, arg2, arg3,
	    NULL, NULL, NULL, NULL, &newphid);
	if (res)
		return res;
	return newphid;
}

/** Ask through phone for a new connection to some service.
 *
 * If the connection is not available at the moment, the
 * call will block.
 *
 * @param phoneid	Phone handle used for contacting the other side.
 * @param arg1		User defined argument.
 * @param arg2		User defined argument.
 * @param arg3		User defined argument.
 *
 * @return		New phone handle on success or a negative error code.
 */
int ipc_connect_me_to_blocking(int phoneid, int arg1, int arg2, int arg3)
{
	ipcarg_t newphid;
	int res;

	res = ipc_call_sync_4_5(phoneid, IPC_M_CONNECT_ME_TO, arg1, arg2, arg3,
	    IPC_FLAG_BLOCKING, NULL, NULL, NULL, NULL, &newphid);
	if (res)
		return res;
	return newphid;
}

/** Hang up a phone.
 *
 * @param phoneid	Handle of the phone to be hung up.
 *
 * @return		Zero on success or a negative error code.
 */
int ipc_hangup(int phoneid)
{
	return __SYSCALL1(SYS_IPC_HANGUP, phoneid);
}

/** Register IRQ notification.
 *
 * @param inr		IRQ number.
 * @param devno		Device number of the device generating inr.
 * @param method	Use this method for notifying me.
 * @param ucode		Top-half pseudocode handler.
 *
 * @return		Value returned by the kernel.
 */
int ipc_register_irq(int inr, int devno, int method, irq_code_t *ucode)
{
	return __SYSCALL4(SYS_IPC_REGISTER_IRQ, inr, devno, method,
	    (sysarg_t) ucode);
}

/** Unregister IRQ notification.
 *
 * @param inr		IRQ number.
 * @param devno		Device number of the device generating inr.
 *
 * @return		Value returned by the kernel.
 */
int ipc_unregister_irq(int inr, int devno)
{
	return __SYSCALL2(SYS_IPC_UNREGISTER_IRQ, inr, devno);
}

/** Forward a received call to another destination.
 *
 * @param callid	Hash of the call to forward.
 * @param phoneid	Phone handle to use for forwarding.
 * @param method	New method for the forwarded call.
 * @param arg1		New value of the first argument for the forwarded call.
 * @param arg2		New value of the second argument for the forwarded call.
 * @param mode		Flags specifying mode of the forward operation.
 *
 * @return		Zero on success or an error code.
 *
 * For non-system methods, the old method, arg1 and arg2 are rewritten by the
 * new values. For system methods, the new method, arg1 and arg2 are written 
 * to the old arg1, arg2 and arg3, respectivelly. Calls with immutable 
 * methods are forwarded verbatim.
 */
int ipc_forward_fast(ipc_callid_t callid, int phoneid, int method,
    ipcarg_t arg1, ipcarg_t arg2, int mode)
{
	return __SYSCALL6(SYS_IPC_FORWARD_FAST, callid, phoneid, method, arg1, 
	    arg2, mode);
}


int ipc_forward_slow(ipc_callid_t callid, int phoneid, int method,
    ipcarg_t arg1, ipcarg_t arg2, ipcarg_t arg3, ipcarg_t arg4, ipcarg_t arg5,
    int mode)
{
	ipc_call_t data;

	IPC_SET_METHOD(data, method);
	IPC_SET_ARG1(data, arg1);
	IPC_SET_ARG2(data, arg2);
	IPC_SET_ARG3(data, arg3);
	IPC_SET_ARG4(data, arg4);
	IPC_SET_ARG5(data, arg5);

	return __SYSCALL4(SYS_IPC_FORWARD_SLOW, callid, phoneid, (sysarg_t) &data, mode);
}

/** Wrapper for making IPC_M_SHARE_IN calls.
 *
 * @param phoneid	Phone that will be used to contact the receiving side.
 * @param dst		Destination address space area base.
 * @param size		Size of the destination address space area.
 * @param arg		User defined argument.
 * @param flags		Storage where the received flags will be stored. Can be
 *			NULL.
 *
 * @return		Zero on success or a negative error code from errno.h.
 */
int ipc_share_in_start(int phoneid, void *dst, size_t size, ipcarg_t arg,
    int *flags)
{
	sysarg_t tmp_flags = 0;
	int res = ipc_call_sync_3_2(phoneid, IPC_M_SHARE_IN, (ipcarg_t) dst,
	    (ipcarg_t) size, arg, NULL, &tmp_flags);
	
	if (flags)
		*flags = tmp_flags;
	
	return res;
}

/** Wrapper for answering the IPC_M_SHARE_IN calls.
 *
 * This wrapper only makes it more comfortable to answer IPC_M_DATA_READ calls
 * so that the user doesn't have to remember the meaning of each IPC argument.
 *
 * @param callid	Hash of the IPC_M_DATA_READ call to answer.
 * @param src		Source address space base.
 * @param flags		Flags to be used for sharing. Bits can be only cleared.
 *
 * @return		Zero on success or a value from @ref errno.h on failure.
 */
int ipc_share_in_finalize(ipc_callid_t callid, void *src, int flags)
{
	return ipc_answer_2(callid, EOK, (ipcarg_t) src, (ipcarg_t) flags);
}

/** Wrapper for making IPC_M_SHARE_OUT calls.
 *
 * @param phoneid	Phone that will be used to contact the receiving side.
 * @param src		Source address space area base address.
 * @param flags		Flags to be used for sharing. Bits can be only cleared.
 *
 * @return		Zero on success or a negative error code from errno.h.
 */
int ipc_share_out_start(int phoneid, void *src, int flags)
{
	return ipc_call_sync_3_0(phoneid, IPC_M_SHARE_OUT, (ipcarg_t) src, 0,
	    (ipcarg_t) flags);
}

/** Wrapper for answering the IPC_M_SHARE_OUT calls.
 *
 * This wrapper only makes it more comfortable to answer IPC_M_SHARE_OUT calls
 * so that the user doesn't have to remember the meaning of each IPC argument.
 *
 * @param callid	Hash of the IPC_M_DATA_WRITE call to answer.
 * @param dst		Destination address space area base address.	
 *
 * @return		Zero on success or a value from @ref errno.h on failure.
 */
int ipc_share_out_finalize(ipc_callid_t callid, void *dst)
{
	return ipc_answer_1(callid, EOK, (ipcarg_t) dst);
}


/** Wrapper for making IPC_M_DATA_READ calls.
 *
 * @param phoneid	Phone that will be used to contact the receiving side.
 * @param dst		Address of the beginning of the destination buffer.
 * @param size		Size of the destination buffer.
 *
 * @return		Zero on success or a negative error code from errno.h.
 */
int ipc_data_read_start(int phoneid, void *dst, size_t size)
{
	return ipc_call_sync_2_0(phoneid, IPC_M_DATA_READ, (ipcarg_t) dst,
	    (ipcarg_t) size);
}

/** Wrapper for answering the IPC_M_DATA_READ calls.
 *
 * This wrapper only makes it more comfortable to answer IPC_M_DATA_READ calls
 * so that the user doesn't have to remember the meaning of each IPC argument.
 *
 * @param callid	Hash of the IPC_M_DATA_READ call to answer.
 * @param src		Source address for the IPC_M_DATA_READ call.
 * @param size		Size for the IPC_M_DATA_READ call. Can be smaller than
 *			the maximum size announced by the sender.
 *
 * @return		Zero on success or a value from @ref errno.h on failure.
 */
int ipc_data_read_finalize(ipc_callid_t callid, const void *src, size_t size)
{
	return ipc_answer_2(callid, EOK, (ipcarg_t) src, (ipcarg_t) size);
}

/** Wrapper for making IPC_M_DATA_WRITE calls.
 *
 * @param phoneid	Phone that will be used to contact the receiving side.
 * @param src		Address of the beginning of the source buffer.
 * @param size		Size of the source buffer.
 *
 * @return		Zero on success or a negative error code from errno.h.
 */
int ipc_data_write_start(int phoneid, const void *src, size_t size)
{
	return ipc_call_sync_2_0(phoneid, IPC_M_DATA_WRITE, (ipcarg_t) src,
	    (ipcarg_t) size);
}

/** Wrapper for answering the IPC_M_DATA_WRITE calls.
 *
 * This wrapper only makes it more comfortable to answer IPC_M_DATA_WRITE calls
 * so that the user doesn't have to remember the meaning of each IPC argument.
 *
 * @param callid	Hash of the IPC_M_DATA_WRITE call to answer.
 * @param dst		Final destination address for the IPC_M_DATA_WRITE call.
 * @param size		Final size for the IPC_M_DATA_WRITE call.
 *
 * @return		Zero on success or a value from @ref errno.h on failure.
 */
int ipc_data_write_finalize(ipc_callid_t callid, void *dst, size_t size)
{
	return ipc_answer_2(callid, EOK, (ipcarg_t) dst, (ipcarg_t) size);
}

#include <kernel/syscall/sysarg64.h>
/** Connect to a task specified by id.
 */
int ipc_connect_kbox(task_id_t id)
{
	sysarg64_t arg;

	arg.value = (unsigned long long) id;

	return __SYSCALL1(SYS_IPC_CONNECT_KBOX, (sysarg_t) &arg);
}
 
/** @}
 */
