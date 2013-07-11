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
#include <futex.h>
#include <fibril.h>
#include <macros.h>

/**
 * Structures of this type are used for keeping track
 * of sent asynchronous calls and queing unsent calls.
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
	
	/** Fibril waiting for sending this call. */
	fid_t fid;
} async_call_t;

LIST_INITIALIZE(dispatched_calls);

/** List of asynchronous calls that were not accepted by kernel.
 *
 * Protected by async_futex, because if the call is not accepted
 * by the kernel, the async framework is used automatically.
 *
 */
LIST_INITIALIZE(queued_calls);

static atomic_t ipc_futex = FUTEX_INITIALIZER;

/** Send asynchronous message via syscall.
 *
 * @param phoneid Phone handle for the call.
 * @param data    Call data with the request.
 *
 * @return Hash of the call or an error code.
 *
 */
static ipc_callid_t ipc_call_async_internal(int phoneid, ipc_call_t *data)
{
	return __SYSCALL2(SYS_IPC_CALL_ASYNC_SLOW, phoneid, (sysarg_t) data);
}

/** Prolog for ipc_call_async_*() functions.
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

/** Epilog for ipc_call_async_*() functions.
 *
 * @param callid      Value returned by the SYS_IPC_CALL_ASYNC_* syscall.
 * @param phoneid     Phone handle through which the call was made.
 * @param call        Structure returned by ipc_prepare_async().
 * @param can_preempt If true, the current fibril can be preempted
 *                    in this call.
 *
 */
static inline void ipc_finish_async(ipc_callid_t callid, int phoneid,
    async_call_t *call, bool can_preempt)
{
	if (!call) {
		/* Nothing to do regardless if failed or not */
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

/** Fast asynchronous call.
 *
 * This function can only handle four arguments of payload. It is, however,
 * faster than the more generic ipc_call_async_slow().
 *
 * Note that this function is a void function.
 *
 * During normal operation, answering this call will trigger the callback.
 * In case of fatal error, the callback handler is called with the proper
 * error code. If the call cannot be temporarily made, it is queued.
 *
 * @param phoneid     Phone handle for the call.
 * @param imethod     Requested interface and method.
 * @param arg1        Service-defined payload argument.
 * @param arg2        Service-defined payload argument.
 * @param arg3        Service-defined payload argument.
 * @param arg4        Service-defined payload argument.
 * @param private     Argument to be passed to the answer/error callback.
 * @param callback    Answer or error callback.
 * @param can_preempt If true, the current fibril will be preempted in
 *                    case the kernel temporarily refuses to accept more
 *                    asynchronous calls.
 *
 */
void ipc_call_async_fast(int phoneid, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, void *private,
    ipc_async_callback_t callback, bool can_preempt)
{
	async_call_t *call = NULL;
	
	if (callback) {
		call = ipc_prepare_async(private, callback);
		if (!call)
			return;
	}
	
	/*
	 * We need to make sure that we get callid
	 * before another thread accesses the queue again.
	 */
	
	futex_down(&ipc_futex);
	ipc_callid_t callid = __SYSCALL6(SYS_IPC_CALL_ASYNC_FAST, phoneid,
	    imethod, arg1, arg2, arg3, arg4);
	
	if (callid == (ipc_callid_t) IPC_CALLRET_TEMPORARY) {
		if (!call) {
			call = ipc_prepare_async(private, callback);
			if (!call)
				return;
		}
		
		IPC_SET_IMETHOD(call->u.msg.data, imethod);
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

/** Asynchronous call transmitting the entire payload.
 *
 * Note that this function is a void function.
 *
 * During normal operation, answering this call will trigger the callback.
 * In case of fatal error, the callback handler is called with the proper
 * error code. If the call cannot be temporarily made, it is queued.
 *
 * @param phoneid     Phone handle for the call.
 * @param imethod     Requested interface and method.
 * @param arg1        Service-defined payload argument.
 * @param arg2        Service-defined payload argument.
 * @param arg3        Service-defined payload argument.
 * @param arg4        Service-defined payload argument.
 * @param arg5        Service-defined payload argument.
 * @param private     Argument to be passed to the answer/error callback.
 * @param callback    Answer or error callback.
 * @param can_preempt If true, the current fibril will be preempted in
 *                    case the kernel temporarily refuses to accept more
 *                    asynchronous calls.
 *
 */
void ipc_call_async_slow(int phoneid, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5, void *private,
    ipc_async_callback_t callback, bool can_preempt)
{
	async_call_t *call = ipc_prepare_async(private, callback);
	if (!call)
		return;
	
	IPC_SET_IMETHOD(call->u.msg.data, imethod);
	IPC_SET_ARG1(call->u.msg.data, arg1);
	IPC_SET_ARG2(call->u.msg.data, arg2);
	IPC_SET_ARG3(call->u.msg.data, arg3);
	IPC_SET_ARG4(call->u.msg.data, arg4);
	IPC_SET_ARG5(call->u.msg.data, arg5);
	
	/*
	 * We need to make sure that we get callid
	 * before another threadaccesses the queue again.
	 */
	
	futex_down(&ipc_futex);
	ipc_callid_t callid =
	    ipc_call_async_internal(phoneid, &call->u.msg.data);
	
	ipc_finish_async(callid, phoneid, call, can_preempt);
}

/** Answer received call (fast version).
 *
 * The fast answer makes use of passing retval and first four arguments in
 * registers. If you need to return more, use the ipc_answer_slow() instead.
 *
 * @param callid Hash of the call being answered.
 * @param retval Return value.
 * @param arg1   First return argument.
 * @param arg2   Second return argument.
 * @param arg3   Third return argument.
 * @param arg4   Fourth return argument.
 *
 * @return Zero on success.
 * @return Value from @ref errno.h on failure.
 *
 */
sysarg_t ipc_answer_fast(ipc_callid_t callid, sysarg_t retval, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4)
{
	return __SYSCALL6(SYS_IPC_ANSWER_FAST, callid, retval, arg1, arg2, arg3,
	    arg4);
}

/** Answer received call (entire payload).
 *
 * @param callid Hash of the call being answered.
 * @param retval Return value.
 * @param arg1   First return argument.
 * @param arg2   Second return argument.
 * @param arg3   Third return argument.
 * @param arg4   Fourth return argument.
 * @param arg5   Fifth return argument.
 *
 * @return Zero on success.
 * @return Value from @ref errno.h on failure.
 *
 */
sysarg_t ipc_answer_slow(ipc_callid_t callid, sysarg_t retval, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5)
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

/** Try to dispatch queued calls from the async queue.
 *
 */
static void dispatch_queued_calls(void)
{
	/** @todo
	 * Integrate intelligently ipc_futex so that it is locked during
	 * ipc_call_async_*() until it is added to dispatched_calls.
	 */
	
	futex_down(&async_futex);
	
	while (!list_empty(&queued_calls)) {
		async_call_t *call =
		    list_get_instance(list_first(&queued_calls), async_call_t, list);
		ipc_callid_t callid =
		    ipc_call_async_internal(call->u.msg.phoneid, &call->u.msg.data);
		
		if (callid == (ipc_callid_t) IPC_CALLRET_TEMPORARY)
			break;
		
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

/** Handle received answer.
 *
 * Find the hash of the answer and call the answer callback.
 *
 * The answer has the same hash as the request OR'ed with
 * the IPC_CALLID_ANSWERED bit.
 *
 * @todo Use hash table.
 *
 * @param callid Hash of the received answer.
 * @param data   Call data of the answer.
 *
 */
static void handle_answer(ipc_callid_t callid, ipc_call_t *data)
{
	callid &= ~IPC_CALLID_ANSWERED;
	
	futex_down(&ipc_futex);
	
	link_t *item;
	for (item = dispatched_calls.head.next; item != &dispatched_calls.head;
	    item = item->next) {
		async_call_t *call =
		    list_get_instance(item, async_call_t, list);
		
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

/** Wait for first IPC call to come.
 *
 * @param call  Incoming call storage.
 * @param usec  Timeout in microseconds
 * @param flags Flags passed to SYS_IPC_WAIT (blocking, nonblocking).
 *
 * @return Hash of the call. Note that certain bits have special
 *         meaning: IPC_CALLID_ANSWERED is set in an answer
 *         and IPC_CALLID_NOTIFICATION is used for notifications.
 *
 */
ipc_callid_t ipc_wait_cycle(ipc_call_t *call, sysarg_t usec,
    unsigned int flags)
{
	ipc_callid_t callid =
	    __SYSCALL3(SYS_IPC_WAIT, (sysarg_t) call, usec, flags);
	
	/* Handle received answers */
	if (callid & IPC_CALLID_ANSWERED) {
		handle_answer(callid, call);
		dispatch_queued_calls();
	}
	
	return callid;
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
 * @param call Incoming call storage.
 * @param usec Timeout in microseconds
 *
 * @return Hash of the call.
 *
 */
ipc_callid_t ipc_wait_for_call_timeout(ipc_call_t *call, sysarg_t usec)
{
	ipc_callid_t callid;
	
	do {
		callid = ipc_wait_cycle(call, usec, SYNCH_FLAGS_NONE);
	} while (callid & IPC_CALLID_ANSWERED);
	
	return callid;
}

/** Check if there is an IPC call waiting to be picked up.
 *
 * Only requests are returned, answers are processed internally.
 *
 * @param call Incoming call storage.
 *
 * @return Hash of the call.
 *
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

/** Hang up a phone.
 *
 * @param phoneid Handle of the phone to be hung up.
 *
 * @return Zero on success or a negative error code.
 *
 */
int ipc_hangup(int phoneid)
{
	return __SYSCALL1(SYS_IPC_HANGUP, phoneid);
}

/** Forward a received call to another destination.
 *
 * For non-system methods, the old method, arg1 and arg2 are rewritten
 * by the new values. For system methods, the new method, arg1 and arg2
 * are written to the old arg1, arg2 and arg3, respectivelly. Calls with
 * immutable methods are forwarded verbatim.
 *
 * @param callid  Hash of the call to forward.
 * @param phoneid Phone handle to use for forwarding.
 * @param imethod New interface and method for the forwarded call.
 * @param arg1    New value of the first argument for the forwarded call.
 * @param arg2    New value of the second argument for the forwarded call.
 * @param mode    Flags specifying mode of the forward operation.
 *
 * @return Zero on success or an error code.
 *
 */
int ipc_forward_fast(ipc_callid_t callid, int phoneid, sysarg_t imethod,
    sysarg_t arg1, sysarg_t arg2, unsigned int mode)
{
	return __SYSCALL6(SYS_IPC_FORWARD_FAST, callid, phoneid, imethod, arg1,
	    arg2, mode);
}

int ipc_forward_slow(ipc_callid_t callid, int phoneid, sysarg_t imethod,
    sysarg_t arg1, sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5,
    unsigned int mode)
{
	ipc_call_t data;
	
	IPC_SET_IMETHOD(data, imethod);
	IPC_SET_ARG1(data, arg1);
	IPC_SET_ARG2(data, arg2);
	IPC_SET_ARG3(data, arg3);
	IPC_SET_ARG4(data, arg4);
	IPC_SET_ARG5(data, arg5);
	
	return __SYSCALL4(SYS_IPC_FORWARD_SLOW, callid, phoneid, (sysarg_t) &data,
	    mode);
}

/** Connect to a task specified by id.
 *
 */
int ipc_connect_kbox(task_id_t id)
{
#ifdef __32_BITS__
	sysarg64_t arg = (sysarg64_t) id;
	return __SYSCALL1(SYS_IPC_CONNECT_KBOX, (sysarg_t) &arg);
#endif
	
#ifdef __64_BITS__
	return __SYSCALL1(SYS_IPC_CONNECT_KBOX, (sysarg_t) id);
#endif
}

/** @}
 */
