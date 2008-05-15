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

/** @addtogroup genericipc
 * @{
 */
/** @file
 */

#include <arch.h>
#include <proc/task.h>
#include <proc/thread.h>
#include <errno.h>
#include <memstr.h>
#include <debug.h>
#include <ipc/ipc.h>
#include <ipc/sysipc.h>
#include <ipc/irq.h>
#include <ipc/ipcrsc.h>
#include <arch/interrupt.h>
#include <print.h>
#include <syscall/copy.h>
#include <security/cap.h>
#include <mm/as.h>
#include <print.h>

/**
 * Maximum buffer size allowed for IPC_M_DATA_WRITE and IPC_M_DATA_READ
 * requests.
 */
#define DATA_XFER_LIMIT		(64 * 1024)

#define GET_CHECK_PHONE(phone, phoneid, err) \
{ \
	if (phoneid > IPC_MAX_PHONES) { \
		err; \
	} \
	phone = &TASK->phones[phoneid]; \
}

#define STRUCT_TO_USPACE(dst, src)	copy_to_uspace(dst, src, sizeof(*(src)))

/** Decide if the method is a system method.
 *
 * @param method	Method to be decided.
 *
 * @return		Return 1 if the method is a system method.
 *			Otherwise return 0.
 */
static inline int method_is_system(unative_t method)
{
	if (method <= IPC_M_LAST_SYSTEM)
		return 1;
	return 0;
}

/** Decide if the message with this method is forwardable.
 *
 * - some system messages may be forwarded, for some of them
 *   it is useless
 *
 * @param method	Method to be decided.
 *
 * @return		Return 1 if the method is forwardable.
 *			Otherwise return 0.
 */
static inline int method_is_forwardable(unative_t method)
{
	switch (method) {
	case IPC_M_PHONE_HUNGUP:
		/* This message is meant only for the original recipient. */
		return 0;
	default:
		return 1;
	}
}

/** Decide if the message with this method is immutable on forward.
 *
 * - some system messages may be forwarded but their content cannot be altered
 *
 * @param method	Method to be decided.
 *
 * @return		Return 1 if the method is immutable on forward.
 *			Otherwise return 0.
 */
static inline int method_is_immutable(unative_t method)
{
	switch (method) {
	case IPC_M_SHARE_OUT:
	case IPC_M_SHARE_IN:
	case IPC_M_DATA_WRITE:
	case IPC_M_DATA_READ:
		return 1;
		break;
	default:
		return 0;
	}
}


/***********************************************************************
 * Functions that preprocess answer before sending it to the recepient.
 ***********************************************************************/

/** Decide if the caller (e.g. ipc_answer()) should save the old call contents
 * for answer_preprocess().
 *
 * @param call		Call structure to be decided.
 *
 * @return		Return 1 if the old call contents should be saved.
 *			Return 0 otherwise.
 */
static inline int answer_need_old(call_t *call)
{
	switch (IPC_GET_METHOD(call->data)) {
	case IPC_M_CONNECT_TO_ME:
	case IPC_M_CONNECT_ME_TO:
	case IPC_M_SHARE_OUT:
	case IPC_M_SHARE_IN:
	case IPC_M_DATA_WRITE:
	case IPC_M_DATA_READ:
		return 1;
	default:
		return 0;
	}
}

/** Interpret process answer as control information.
 *
 * This function is called directly after sys_ipc_answer().
 *
 * @param answer	Call structure with the answer.
 * @param olddata	Saved data of the request.
 *
 * @return		Return 0 on success or an error code. 
 */
static inline int answer_preprocess(call_t *answer, ipc_data_t *olddata)
{
	int phoneid;

	if ((native_t) IPC_GET_RETVAL(answer->data) == EHANGUP) {
		/* In case of forward, hangup the forwared phone,
		 * not the originator
		 */
		spinlock_lock(&answer->data.phone->lock);
		spinlock_lock(&TASK->answerbox.lock);
		if (answer->data.phone->state == IPC_PHONE_CONNECTED) {
			list_remove(&answer->data.phone->link);
			answer->data.phone->state = IPC_PHONE_SLAMMED;
		}
		spinlock_unlock(&TASK->answerbox.lock);
		spinlock_unlock(&answer->data.phone->lock);
	}

	if (!olddata)
		return 0;

	if (IPC_GET_METHOD(*olddata) == IPC_M_CONNECT_TO_ME) {
		phoneid = IPC_GET_ARG5(*olddata);
		if (IPC_GET_RETVAL(answer->data)) {
			/* The connection was not accepted */
			phone_dealloc(phoneid);
		} else {
			/* The connection was accepted */
			phone_connect(phoneid, &answer->sender->answerbox);
			/* Set 'phone hash' as arg5 of response */
			IPC_SET_ARG5(answer->data,
			    (unative_t) &TASK->phones[phoneid]);
		}
	} else if (IPC_GET_METHOD(*olddata) == IPC_M_CONNECT_ME_TO) {
		/* If the users accepted call, connect */
		if (!IPC_GET_RETVAL(answer->data)) {
			ipc_phone_connect((phone_t *) IPC_GET_ARG5(*olddata),
			    &TASK->answerbox);
		}
	} else if (IPC_GET_METHOD(*olddata) == IPC_M_SHARE_OUT) {
		if (!IPC_GET_RETVAL(answer->data)) {
			/* Accepted, handle as_area receipt */
			ipl_t ipl;
			int rc;
			as_t *as;
			
			ipl = interrupts_disable();
			spinlock_lock(&answer->sender->lock);
			as = answer->sender->as;
			spinlock_unlock(&answer->sender->lock);
			interrupts_restore(ipl);
			
			rc = as_area_share(as, IPC_GET_ARG1(*olddata),
			    IPC_GET_ARG2(*olddata), AS,
			    IPC_GET_ARG1(answer->data), IPC_GET_ARG3(*olddata));
			IPC_SET_RETVAL(answer->data, rc);
			return rc;
		}
	} else if (IPC_GET_METHOD(*olddata) == IPC_M_SHARE_IN) {
		if (!IPC_GET_RETVAL(answer->data)) { 
			ipl_t ipl;
			as_t *as;
			int rc;
			
			ipl = interrupts_disable();
			spinlock_lock(&answer->sender->lock);
			as = answer->sender->as;
			spinlock_unlock(&answer->sender->lock);
			interrupts_restore(ipl);
			
			rc = as_area_share(AS, IPC_GET_ARG1(answer->data),
			    IPC_GET_ARG2(*olddata), as, IPC_GET_ARG1(*olddata),
			    IPC_GET_ARG2(answer->data));
			IPC_SET_RETVAL(answer->data, rc);
		}
	} else if (IPC_GET_METHOD(*olddata) == IPC_M_DATA_READ) {
		ASSERT(!answer->buffer);
		if (!IPC_GET_RETVAL(answer->data)) {
			/* The recipient agreed to send data. */
			uintptr_t src = IPC_GET_ARG1(answer->data);
			uintptr_t dst = IPC_GET_ARG1(*olddata);
			size_t max_size = IPC_GET_ARG2(*olddata);
			size_t size = IPC_GET_ARG2(answer->data);
			if (size && size <= max_size) {
				/*
				 * Copy the destination VA so that this piece of
				 * information is not lost.
				 */
				IPC_SET_ARG1(answer->data, dst);

				answer->buffer = malloc(size, 0);
				int rc = copy_from_uspace(answer->buffer,
				    (void *) src, size);
				if (rc) {
					IPC_SET_RETVAL(answer->data, rc);
					free(answer->buffer);
					answer->buffer = NULL;
				}
			} else if (!size) {
				IPC_SET_RETVAL(answer->data, EOK);
			} else {
				IPC_SET_RETVAL(answer->data, ELIMIT);
			}
		}
	} else if (IPC_GET_METHOD(*olddata) == IPC_M_DATA_WRITE) {
		ASSERT(answer->buffer);
		if (!IPC_GET_RETVAL(answer->data)) {
			/* The recipient agreed to receive data. */
			int rc;
			uintptr_t dst;
			uintptr_t size;
			uintptr_t max_size;

			dst = IPC_GET_ARG1(answer->data);
			size = IPC_GET_ARG2(answer->data);
			max_size = IPC_GET_ARG2(*olddata);

			if (size <= max_size) {
				rc = copy_to_uspace((void *) dst,
				    answer->buffer, size);
				if (rc)
					IPC_SET_RETVAL(answer->data, rc);
			} else {
				IPC_SET_RETVAL(answer->data, ELIMIT);
			}
		}
		free(answer->buffer);
		answer->buffer = NULL;
	}
	return 0;
}

/** Called before the request is sent.
 *
 * @param call		Call structure with the request.
 *
 * @return 		Return 0 on success, ELIMIT or EPERM on error.
 */
static int request_preprocess(call_t *call)
{
	int newphid;
	size_t size;
	uintptr_t src;
	int rc;

	switch (IPC_GET_METHOD(call->data)) {
	case IPC_M_CONNECT_ME_TO:
		newphid = phone_alloc();
		if (newphid < 0)
			return ELIMIT;
		/* Set arg5 for server */
		IPC_SET_ARG5(call->data, (unative_t) &TASK->phones[newphid]);
		call->flags |= IPC_CALL_CONN_ME_TO;
		call->priv = newphid;
		break;
	case IPC_M_SHARE_OUT:
		size = as_area_get_size(IPC_GET_ARG1(call->data));
		if (!size)
			return EPERM;
		IPC_SET_ARG2(call->data, size);
		break;
	case IPC_M_DATA_READ:
		size = IPC_GET_ARG2(call->data);
		if ((size <= 0 || (size > DATA_XFER_LIMIT)))
			return ELIMIT;
		break;
	case IPC_M_DATA_WRITE:
		src = IPC_GET_ARG1(call->data);
		size = IPC_GET_ARG2(call->data);
		
		if ((size <= 0) || (size > DATA_XFER_LIMIT))
			return ELIMIT;
		
		call->buffer = (uint8_t *) malloc(size, 0);
		rc = copy_from_uspace(call->buffer, (void *) src, size);
		if (rc != 0) {
			free(call->buffer);
			return rc;
		}
		break;
	default:
		break;
	}
	return 0;
}

/*******************************************************************************
 * Functions called to process received call/answer before passing it to uspace.
 *******************************************************************************/

/** Do basic kernel processing of received call answer.
 *
 * @param call		Call structure with the answer.
 */
static void process_answer(call_t *call)
{
	if (((native_t) IPC_GET_RETVAL(call->data) == EHANGUP) &&
	    (call->flags & IPC_CALL_FORWARDED))
		IPC_SET_RETVAL(call->data, EFORWARD);

	if (call->flags & IPC_CALL_CONN_ME_TO) {
		if (IPC_GET_RETVAL(call->data))
			phone_dealloc(call->priv);
		else
			IPC_SET_ARG5(call->data, call->priv);
	}

	if (call->buffer) {
		/* This must be an affirmative answer to IPC_M_DATA_READ. */
		uintptr_t dst = IPC_GET_ARG1(call->data);
		size_t size = IPC_GET_ARG2(call->data);
		int rc = copy_to_uspace((void *) dst, call->buffer, size);
		if (rc)
			IPC_SET_RETVAL(call->data, rc);
		free(call->buffer);
		call->buffer = NULL;
	}
}

/** Do basic kernel processing of received call request.
 *
 * @param box		Destination answerbox structure.
 * @param call		Call structure with the request.
 *
 * @return 		Return 0 if the call should be passed to userspace.
 *			Return -1 if the call should be ignored.
 */
static int process_request(answerbox_t *box, call_t *call)
{
	int phoneid;

	if (IPC_GET_METHOD(call->data) == IPC_M_CONNECT_TO_ME) {
		phoneid = phone_alloc();
		if (phoneid < 0) { /* Failed to allocate phone */
			IPC_SET_RETVAL(call->data, ELIMIT);
			ipc_answer(box, call);
			return -1;
		}
		IPC_SET_ARG5(call->data, phoneid);
	} 
	return 0;
}

/** Make a fast call over IPC, wait for reply and return to user.
 *
 * This function can handle only three arguments of payload, but is faster than
 * the generic function (i.e. sys_ipc_call_sync_slow()).
 *
 * @param phoneid	Phone handle for the call.
 * @param method	Method of the call.
 * @param arg1		Service-defined payload argument.
 * @param arg2		Service-defined payload argument.
 * @param arg3		Service-defined payload argument.
 * @param data		Address of userspace structure where the reply call will
 *			be stored.
 *
 * @return		Returns 0 on success.
 *			Return ENOENT if there is no such phone handle.
 */
unative_t sys_ipc_call_sync_fast(unative_t phoneid, unative_t method,
    unative_t arg1, unative_t arg2, unative_t arg3, ipc_data_t *data)
{
	call_t call;
	phone_t *phone;
	int res;
	int rc;

	GET_CHECK_PHONE(phone, phoneid, return ENOENT);

	ipc_call_static_init(&call);
	IPC_SET_METHOD(call.data, method);
	IPC_SET_ARG1(call.data, arg1);
	IPC_SET_ARG2(call.data, arg2);
	IPC_SET_ARG3(call.data, arg3);
	/*
	 * To achieve deterministic behavior, zero out arguments that are beyond
	 * the limits of the fast version.
	 */
	IPC_SET_ARG4(call.data, 0);
	IPC_SET_ARG5(call.data, 0);

	if (!(res = request_preprocess(&call))) {
		ipc_call_sync(phone, &call);
		process_answer(&call);
	} else {
		IPC_SET_RETVAL(call.data, res);
	}
	rc = STRUCT_TO_USPACE(&data->args, &call.data.args);
	if (rc != 0)
		return rc;

	return 0;
}

/** Make a synchronous IPC call allowing to transmit the entire payload.
 *
 * @param phoneid	Phone handle for the call.
 * @param question	Userspace address of call data with the request.
 * @param reply		Userspace address of call data where to store the
 *			answer.
 *
 * @return		Zero on success or an error code.
 */
unative_t sys_ipc_call_sync_slow(unative_t phoneid, ipc_data_t *question,
    ipc_data_t *reply)
{
	call_t call;
	phone_t *phone;
	int res;
	int rc;

	ipc_call_static_init(&call);
	rc = copy_from_uspace(&call.data.args, &question->args,
	    sizeof(call.data.args));
	if (rc != 0)
		return (unative_t) rc;

	GET_CHECK_PHONE(phone, phoneid, return ENOENT);

	if (!(res = request_preprocess(&call))) {
		ipc_call_sync(phone, &call);
		process_answer(&call);
	} else 
		IPC_SET_RETVAL(call.data, res);

	rc = STRUCT_TO_USPACE(&reply->args, &call.data.args);
	if (rc != 0)
		return rc;

	return 0;
}

/** Check that the task did not exceed the allowed limit of asynchronous calls.
 *
 * @return 		Return 0 if limit not reached or -1 if limit exceeded.
 */
static int check_call_limit(void)
{
	if (atomic_preinc(&TASK->active_calls) > IPC_MAX_ASYNC_CALLS) {
		atomic_dec(&TASK->active_calls);
		return -1;
	}
	return 0;
}

/** Make a fast asynchronous call over IPC.
 *
 * This function can only handle four arguments of payload, but is faster than
 * the generic function sys_ipc_call_async_slow().
 *
 * @param phoneid	Phone handle for the call.
 * @param method	Method of the call.
 * @param arg1		Service-defined payload argument.
 * @param arg2		Service-defined payload argument.
 * @param arg3		Service-defined payload argument.
 * @param arg4		Service-defined payload argument.
 *
 * @return 		Return call hash on success.
 *			Return IPC_CALLRET_FATAL in case of a fatal error and 
 *			IPC_CALLRET_TEMPORARY if there are too many pending
 *			asynchronous requests; answers should be handled first.
 */
unative_t sys_ipc_call_async_fast(unative_t phoneid, unative_t method,
    unative_t arg1, unative_t arg2, unative_t arg3, unative_t arg4)
{
	call_t *call;
	phone_t *phone;
	int res;

	if (check_call_limit())
		return IPC_CALLRET_TEMPORARY;

	GET_CHECK_PHONE(phone, phoneid, return IPC_CALLRET_FATAL);

	call = ipc_call_alloc(0);
	IPC_SET_METHOD(call->data, method);
	IPC_SET_ARG1(call->data, arg1);
	IPC_SET_ARG2(call->data, arg2);
	IPC_SET_ARG3(call->data, arg3);
	IPC_SET_ARG4(call->data, arg4);
	/*
	 * To achieve deterministic behavior, zero out arguments that are beyond
	 * the limits of the fast version.
	 */
	IPC_SET_ARG5(call->data, 0);

	if (!(res = request_preprocess(call)))
		ipc_call(phone, call);
	else
		ipc_backsend_err(phone, call, res);

	return (unative_t) call;
}

/** Make an asynchronous IPC call allowing to transmit the entire payload.
 *
 * @param phoneid	Phone handle for the call.
 * @param data		Userspace address of call data with the request.
 *
 * @return		See sys_ipc_call_async_fast(). 
 */
unative_t sys_ipc_call_async_slow(unative_t phoneid, ipc_data_t *data)
{
	call_t *call;
	phone_t *phone;
	int res;
	int rc;

	if (check_call_limit())
		return IPC_CALLRET_TEMPORARY;

	GET_CHECK_PHONE(phone, phoneid, return IPC_CALLRET_FATAL);

	call = ipc_call_alloc(0);
	rc = copy_from_uspace(&call->data.args, &data->args,
	    sizeof(call->data.args));
	if (rc != 0) {
		ipc_call_free(call);
		return (unative_t) rc;
	}
	if (!(res = request_preprocess(call)))
		ipc_call(phone, call);
	else
		ipc_backsend_err(phone, call, res);

	return (unative_t) call;
}

/** Forward a received call to another destination.
 *
 * @param callid	Hash of the call to forward.
 * @param phoneid	Phone handle to use for forwarding.
 * @param method	New method to use for the forwarded call.
 * @param arg1		New value of the first argument for the forwarded call.
 * @param arg2		New value of the second argument for the forwarded call.
 * @param mode		Flags that specify mode of the forward operation.
 *
 * @return		Return 0 on succes, otherwise return an error code.
 *
 * In case the original method is a system method, ARG1, ARG2 and ARG3 are
 * overwritten in the forwarded message with the new method and the new arg1 and
 * arg2, respectively. Otherwise the METHOD, ARG1 and ARG2 are rewritten with
 * the new method, arg1 and arg2, respectively. Also note there is a set of
 * immutable methods, for which the new method and argument is not set and
 * these values are ignored.
 *
 * Warning:	When implementing support for changing additional payload
 *		arguments, make sure that ARG5 is not rewritten for certain
 *		system IPC
 */
unative_t sys_ipc_forward_fast(unative_t callid, unative_t phoneid,
    unative_t method, unative_t arg1, unative_t arg2, int mode)
{
	call_t *call;
	phone_t *phone;

	call = get_call(callid);
	if (!call)
		return ENOENT;

	call->flags |= IPC_CALL_FORWARDED;

	GET_CHECK_PHONE(phone, phoneid, { 
		IPC_SET_RETVAL(call->data, EFORWARD);
		ipc_answer(&TASK->answerbox, call);
		return ENOENT;
	});		

	if (!method_is_forwardable(IPC_GET_METHOD(call->data))) {
		IPC_SET_RETVAL(call->data, EFORWARD);
		ipc_answer(&TASK->answerbox, call);
		return EPERM;
	}

	/*
	 * Userspace is not allowed to change method of system methods on
	 * forward, allow changing ARG1, ARG2 and ARG3 by means of method,
	 * arg1 and arg2.
	 * If the method is immutable, don't change anything.
	 */
	if (!method_is_immutable(IPC_GET_METHOD(call->data))) {
		if (method_is_system(IPC_GET_METHOD(call->data))) {
			if (IPC_GET_METHOD(call->data) == IPC_M_CONNECT_TO_ME)
				phone_dealloc(IPC_GET_ARG5(call->data));

			IPC_SET_ARG1(call->data, method);
			IPC_SET_ARG2(call->data, arg1);
			IPC_SET_ARG3(call->data, arg2);
		} else {
			IPC_SET_METHOD(call->data, method);
			IPC_SET_ARG1(call->data, arg1);
			IPC_SET_ARG2(call->data, arg2);
		}
	}

	return ipc_forward(call, phone, &TASK->answerbox, mode);
}

/** Answer an IPC call - fast version.
 *
 * This function can handle only two return arguments of payload, but is faster
 * than the generic sys_ipc_answer().
 *
 * @param callid	Hash of the call to be answered.
 * @param retval	Return value of the answer.
 * @param arg1		Service-defined return value.
 * @param arg2		Service-defined return value.
 * @param arg3		Service-defined return value.
 * @param arg4		Service-defined return value.
 *
 * @return		Return 0 on success, otherwise return an error code.	
 */
unative_t sys_ipc_answer_fast(unative_t callid, unative_t retval,
    unative_t arg1, unative_t arg2, unative_t arg3, unative_t arg4)
{
	call_t *call;
	ipc_data_t saved_data;
	int saveddata = 0;
	int rc;

	/* Do not answer notification callids */
	if (callid & IPC_CALLID_NOTIFICATION)
		return 0;

	call = get_call(callid);
	if (!call)
		return ENOENT;

	if (answer_need_old(call)) {
		memcpy(&saved_data, &call->data, sizeof(call->data));
		saveddata = 1;
	}

	IPC_SET_RETVAL(call->data, retval);
	IPC_SET_ARG1(call->data, arg1);
	IPC_SET_ARG2(call->data, arg2);
	IPC_SET_ARG3(call->data, arg3);
	IPC_SET_ARG4(call->data, arg4);
	/*
	 * To achieve deterministic behavior, zero out arguments that are beyond
	 * the limits of the fast version.
	 */
	IPC_SET_ARG5(call->data, 0);
	rc = answer_preprocess(call, saveddata ? &saved_data : NULL);

	ipc_answer(&TASK->answerbox, call);
	return rc;
}

/** Answer an IPC call.
 *
 * @param callid	Hash of the call to be answered.
 * @param data		Userspace address of call data with the answer.
 *
 * @return		Return 0 on success, otherwise return an error code.
 */
unative_t sys_ipc_answer_slow(unative_t callid, ipc_data_t *data)
{
	call_t *call;
	ipc_data_t saved_data;
	int saveddata = 0;
	int rc;

	/* Do not answer notification callids */
	if (callid & IPC_CALLID_NOTIFICATION)
		return 0;

	call = get_call(callid);
	if (!call)
		return ENOENT;

	if (answer_need_old(call)) {
		memcpy(&saved_data, &call->data, sizeof(call->data));
		saveddata = 1;
	}
	rc = copy_from_uspace(&call->data.args, &data->args, 
	    sizeof(call->data.args));
	if (rc != 0)
		return rc;

	rc = answer_preprocess(call, saveddata ? &saved_data : NULL);
	
	ipc_answer(&TASK->answerbox, call);

	return rc;
}

/** Hang up a phone.
 *
 * @param		Phone handle of the phone to be hung up.
 *
 * @return		Return 0 on success or an error code.
 */
unative_t sys_ipc_hangup(int phoneid)
{
	phone_t *phone;

	GET_CHECK_PHONE(phone, phoneid, return ENOENT);

	if (ipc_phone_hangup(phone))
		return -1;

	return 0;
}

/** Wait for an incoming IPC call or an answer.
 *
 * @param calldata	Pointer to buffer where the call/answer data is stored.
 * @param usec 		Timeout. See waitq_sleep_timeout() for explanation.
 * @param flags		Select mode of sleep operation. See waitq_sleep_timeout()
 *			for explanation.
 *
 * @return 		Hash of the call.
 *			If IPC_CALLID_NOTIFICATION bit is set in the hash, the
 *			call is a notification. IPC_CALLID_ANSWERED denotes an
 *			answer.
 */
unative_t sys_ipc_wait_for_call(ipc_data_t *calldata, uint32_t usec, int flags)
{
	call_t *call;

restart:	
	call = ipc_wait_for_call(&TASK->answerbox, usec,
	    flags | SYNCH_FLAGS_INTERRUPTIBLE);
	if (!call)
		return 0;

	if (call->flags & IPC_CALL_NOTIF) {
		ASSERT(! (call->flags & IPC_CALL_STATIC_ALLOC));

		/* Set in_phone_hash to the interrupt counter */
		call->data.phone = (void *) call->priv;
		
		STRUCT_TO_USPACE(calldata, &call->data);

		ipc_call_free(call);
		
		return ((unative_t) call) | IPC_CALLID_NOTIFICATION;
	}

	if (call->flags & IPC_CALL_ANSWERED) {
		process_answer(call);

		ASSERT(! (call->flags & IPC_CALL_STATIC_ALLOC));

		atomic_dec(&TASK->active_calls);

		if (call->flags & IPC_CALL_DISCARD_ANSWER) {
			ipc_call_free(call);
			goto restart;
		}

		STRUCT_TO_USPACE(&calldata->args, &call->data.args);
		ipc_call_free(call);

		return ((unative_t) call) | IPC_CALLID_ANSWERED;
	}

	if (process_request(&TASK->answerbox, call))
		goto restart;

	/* Include phone address('id') of the caller in the request,
	 * copy whole call->data, not only call->data.args */
	if (STRUCT_TO_USPACE(calldata, &call->data)) {
		return 0;
	}
	return (unative_t)call;
}

/** Connect an IRQ handler to a task.
 *
 * @param inr		IRQ number.
 * @param devno		Device number.
 * @param method	Method to be associated with the notification.
 * @param ucode		Uspace pointer to the top-half pseudocode.
 *
 * @return		EPERM or a return code returned by ipc_irq_register().
 */
unative_t sys_ipc_register_irq(inr_t inr, devno_t devno, unative_t method,
    irq_code_t *ucode)
{
	if (!(cap_get(TASK) & CAP_IRQ_REG))
		return EPERM;

	return ipc_irq_register(&TASK->answerbox, inr, devno, method, ucode);
}

/** Disconnect an IRQ handler from a task.
 *
 * @param inr		IRQ number.
 * @param devno		Device number.
 *
 * @return		Zero on success or EPERM on error..
 */
unative_t sys_ipc_unregister_irq(inr_t inr, devno_t devno)
{
	if (!(cap_get(TASK) & CAP_IRQ_REG))
		return EPERM;

	ipc_irq_unregister(&TASK->answerbox, inr, devno);

	return 0;
}

/** @}
 */
