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

#define GET_CHECK_PHONE(phone,phoneid,err) { \
      if (phoneid > IPC_MAX_PHONES) { err; } \
      phone = &TASK->phones[phoneid]; \
}

#define STRUCT_TO_USPACE(dst,src) copy_to_uspace(dst,src,sizeof(*(src)))

/** Return true if the method is a system method */
static inline int is_system_method(__native method)
{
	if (method <= IPC_M_LAST_SYSTEM)
		return 1;
	return 0;
}

/** Return true if the message with this method is forwardable
 *
 * - some system messages may be forwarded, for some of them
 *   it is useless
 */
static inline int is_forwardable(__native method)
{
	if (method == IPC_M_PHONE_HUNGUP || method == IPC_M_AS_AREA_SEND \
	    || method == IPC_M_AS_AREA_RECV)
		return 0; /* This message is meant only for the receiver */
	return 1;
}

/****************************************************/
/* Functions that preprocess answer before sending 
 * it to the recepient
 */

/** Return true if the caller (ipc_answer) should save
 * the old call contents for answer_preprocess
 */
static inline int answer_need_old(call_t *call)
{
	if (IPC_GET_METHOD(call->data) == IPC_M_CONNECT_TO_ME)
		return 1;
	if (IPC_GET_METHOD(call->data) == IPC_M_CONNECT_ME_TO)
		return 1;
	if (IPC_GET_METHOD(call->data) == IPC_M_AS_AREA_SEND)
		return 1;
	if (IPC_GET_METHOD(call->data) == IPC_M_AS_AREA_RECV)
		return 1;
	return 0;
}

/** Interpret process answer as control information
 *
 * This function is called directly after sys_ipc_answer 
 */
static inline int answer_preprocess(call_t *answer, ipc_data_t *olddata)
{
	int phoneid;

	if (IPC_GET_RETVAL(answer->data) == EHANGUP) {
		/* In case of forward, hangup the forwared phone,
		 * not the originator
		 */
		spinlock_lock(&answer->data.phone->lock);
		spinlock_lock(&TASK->answerbox.lock);
		if (answer->data.phone->callee) {
			list_remove(&answer->data.phone->list);
			answer->data.phone->callee = 0;
		}
		spinlock_unlock(&TASK->answerbox.lock);
		spinlock_unlock(&answer->data.phone->lock);
	}

	if (!olddata)
		return 0;

	if (IPC_GET_METHOD(*olddata) == IPC_M_CONNECT_TO_ME) {
		phoneid = IPC_GET_ARG3(*olddata);
		if (IPC_GET_RETVAL(answer->data)) {
			/* The connection was not accepted */
			phone_dealloc(phoneid);
		} else {
			/* The connection was accepted */
			phone_connect(phoneid,&answer->sender->answerbox);
			/* Set 'phone identification' as arg3 of response */
			IPC_SET_ARG3(answer->data, (__native)&TASK->phones[phoneid]);
		}
	} else if (IPC_GET_METHOD(*olddata) == IPC_M_CONNECT_ME_TO) {
		/* If the users accepted call, connect */
		if (!IPC_GET_RETVAL(answer->data)) {
			ipc_phone_connect((phone_t *)IPC_GET_ARG3(*olddata),
					  &TASK->answerbox);
		}
	} else if (IPC_GET_METHOD(*olddata) == IPC_M_AS_AREA_SEND) {
		if (!IPC_GET_RETVAL(answer->data)) { /* Accepted, handle as_area receipt */
			ipl_t ipl;
			as_t *as;
			
			ipl = interrupts_disable();
			spinlock_lock(&answer->sender->lock);
			as = answer->sender->as;
			spinlock_unlock(&answer->sender->lock);
			interrupts_restore(ipl);
			
			return as_area_share(as, IPC_GET_ARG1(*olddata), IPC_GET_ARG2(*olddata),
					     AS, IPC_GET_ARG1(answer->data), IPC_GET_ARG3(*olddata));
		}
	} else if (IPC_GET_METHOD(*olddata) == IPC_M_AS_AREA_RECV) {
		if (!IPC_GET_RETVAL(answer->data)) { 
			ipl_t ipl;
			as_t *as;
			
			ipl = interrupts_disable();
			spinlock_lock(&answer->sender->lock);
			as = answer->sender->as;
			spinlock_unlock(&answer->sender->lock);
			interrupts_restore(ipl);
			
			return as_area_share(AS, IPC_GET_ARG1(answer->data), IPC_GET_ARG2(*olddata),
					     as, IPC_GET_ARG1(*olddata), IPC_GET_ARG3(*olddata));
		}
	}
	return 0;
}

/** Called before the request is sent
 *
 * @return 0 - no error, -1 - report error to user
 */
static int request_preprocess(call_t *call)
{
	int newphid;
	size_t size;

	switch (IPC_GET_METHOD(call->data)) {
	case IPC_M_CONNECT_ME_TO:
		newphid = phone_alloc();
		if (newphid < 0)
			return ELIMIT;
		/* Set arg3 for server */
		IPC_SET_ARG3(call->data, (__native)&TASK->phones[newphid]);
		call->flags |= IPC_CALL_CONN_ME_TO;
		call->private = newphid;
		break;
	case IPC_M_AS_AREA_SEND:
		size = as_get_size(IPC_GET_ARG1(call->data));
		if (!size) {
			return EPERM;
		}
		IPC_SET_ARG2(call->data, size);
		break;
	default:
		break;
	}
	return 0;
}

/****************************************************/
/* Functions called to process received call/answer 
 * before passing to uspace
 */

/** Do basic kernel processing of received call answer */
static void process_answer(call_t *call)
{
	if (IPC_GET_RETVAL(call->data) == EHANGUP && \
	    call->flags & IPC_CALL_FORWARDED)
		IPC_SET_RETVAL(call->data, EFORWARD);

	if (call->flags & IPC_CALL_CONN_ME_TO) {
		if (IPC_GET_RETVAL(call->data))
			phone_dealloc(call->private);
		else
			IPC_SET_ARG3(call->data, call->private);
	}
}

/** Do basic kernel processing of received call request
 *
 * @return 0 - the call should be passed to userspace, 1 - ignore call
 */
static int process_request(answerbox_t *box,call_t *call)
{
	int phoneid;

	if (IPC_GET_METHOD(call->data) == IPC_M_CONNECT_TO_ME) {
		phoneid = phone_alloc();
		if (phoneid < 0) { /* Failed to allocate phone */
			IPC_SET_RETVAL(call->data, ELIMIT);
			ipc_answer(box,call);
			return -1;
		}
		IPC_SET_ARG3(call->data, phoneid);
	} 
	return 0;
}

/** Send a call over IPC, wait for reply, return to user
 *
 * @return Call identification, returns -1 on fatal error, 
           -2 on 'Too many async request, handle answers first
 */
__native sys_ipc_call_sync_fast(__native phoneid, __native method, 
				__native arg1, ipc_data_t *data)
{
	call_t call;
	phone_t *phone;
	int res;

	GET_CHECK_PHONE(phone, phoneid, return ENOENT);

	ipc_call_static_init(&call);
	IPC_SET_METHOD(call.data, method);
	IPC_SET_ARG1(call.data, arg1);

	if (!(res=request_preprocess(&call))) {
		ipc_call_sync(phone, &call);
		process_answer(&call);
	} else 
		IPC_SET_RETVAL(call.data, res);
	STRUCT_TO_USPACE(&data->args, &call.data.args);

	return 0;
}

/** Synchronous IPC call allowing to send whole message */
__native sys_ipc_call_sync(__native phoneid, ipc_data_t *question, 
			   ipc_data_t *reply)
{
	call_t call;
	phone_t *phone;
	int res;
	int rc;

	ipc_call_static_init(&call);
	rc = copy_from_uspace(&call.data.args, &question->args, sizeof(call.data.args));
	if (rc != 0)
		return (__native) rc;

	GET_CHECK_PHONE(phone, phoneid, return ENOENT);

	if (!(res=request_preprocess(&call))) {
		ipc_call_sync(phone, &call);
		process_answer(&call);
	} else 
		IPC_SET_RETVAL(call.data, res);

	rc = STRUCT_TO_USPACE(&reply->args, &call.data.args);
	if (rc != 0)
		return rc;

	return 0;
}

/** Check that the task did not exceed allowed limit
 *
 * @return 0 - Limit OK,   -1 - limit exceeded
 */
static int check_call_limit(void)
{
	if (atomic_preinc(&TASK->active_calls) > IPC_MAX_ASYNC_CALLS) {
		atomic_dec(&TASK->active_calls);
		return -1;
	}
	return 0;
}

/** Send an asynchronous call over ipc
 *
 * @return Call identification, returns -1 on fatal error, 
           -2 on 'Too many async request, handle answers first
 */
__native sys_ipc_call_async_fast(__native phoneid, __native method, 
				 __native arg1, __native arg2)
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

	if (!(res=request_preprocess(call)))
		ipc_call(phone, call);
	else
		ipc_backsend_err(phone, call, res);

	return (__native) call;
}

/** Synchronous IPC call allowing to send whole message
 *
 * @return The same as sys_ipc_call_async
 */
__native sys_ipc_call_async(__native phoneid, ipc_data_t *data)
{
	call_t *call;
	phone_t *phone;
	int res;
	int rc;

	if (check_call_limit())
		return IPC_CALLRET_TEMPORARY;

	GET_CHECK_PHONE(phone, phoneid, return IPC_CALLRET_FATAL);

	call = ipc_call_alloc(0);
	rc = copy_from_uspace(&call->data.args, &data->args, sizeof(call->data.args));
	if (rc != 0) {
		ipc_call_free(call);
		return (__native) rc;
	}
	if (!(res=request_preprocess(call)))
		ipc_call(phone, call);
	else
		ipc_backsend_err(phone, call, res);

	return (__native) call;
}

/** Forward received call to another destination
 *
 * The arg1 and arg2 are changed in the forwarded message
 *
 * Warning: If implementing non-fast version, make sure that
 *          arg3 is not rewritten for certain system IPC
 */
__native sys_ipc_forward_fast(__native callid, __native phoneid,
			      __native method, __native arg1)
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

	if (!is_forwardable(IPC_GET_METHOD(call->data))) {
		IPC_SET_RETVAL(call->data, EFORWARD);
		ipc_answer(&TASK->answerbox, call);
		return EPERM;
	}

	/* Userspace is not allowed to change method of system methods
	 * on forward, allow changing ARG1 and ARG2 by means of method and arg1
	 */
	if (is_system_method(IPC_GET_METHOD(call->data))) {
		if (IPC_GET_METHOD(call->data) == IPC_M_CONNECT_TO_ME)
			phone_dealloc(IPC_GET_ARG3(call->data));

		IPC_SET_ARG1(call->data, method);
		IPC_SET_ARG2(call->data, arg1);
	} else {
		IPC_SET_METHOD(call->data, method);
		IPC_SET_ARG1(call->data, arg1);
	}

	return ipc_forward(call, phone, &TASK->answerbox);
}

/** Send IPC answer */
__native sys_ipc_answer_fast(__native callid, __native retval, 
			     __native arg1, __native arg2)
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
	rc = answer_preprocess(call, saveddata ? &saved_data : NULL);

	ipc_answer(&TASK->answerbox, call);
	return rc;
}

/** Send IPC answer */
__native sys_ipc_answer(__native callid, ipc_data_t *data)
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

/** Hang up the phone
 *
 */
__native sys_ipc_hangup(int phoneid)
{
	phone_t *phone;

	GET_CHECK_PHONE(phone, phoneid, return ENOENT);

	if (ipc_phone_hangup(phone))
		return -1;

	return 0;
}

/** Wait for incoming ipc call or answer
 *
 * @param calldata Pointer to buffer where the call/answer data is stored 
 * @param usec Timeout. See waitq_sleep_timeout() for explanation.
 * @param nonblocking See waitq_sleep_timeout() for explanation.
 *
 * @return Callid, if callid & 1, then the call is answer
 */
__native sys_ipc_wait_for_call(ipc_data_t *calldata, __u32 usec, int nonblocking)
{
	call_t *call;

restart:	
	call = ipc_wait_for_call(&TASK->answerbox, usec, nonblocking);
	if (!call)
		return 0;

	if (call->flags & IPC_CALL_NOTIF) {
		ASSERT(! (call->flags & IPC_CALL_STATIC_ALLOC));
		STRUCT_TO_USPACE(&calldata->args, &call->data.args);
		ipc_call_free(call);
		
		return ((__native)call) | IPC_CALLID_NOTIFICATION;
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

		return ((__native)call) | IPC_CALLID_ANSWERED;
	}

	if (process_request(&TASK->answerbox, call))
		goto restart;

	/* Include phone address('id') of the caller in the request,
	 * copy whole call->data, not only call->data.args */
	if (STRUCT_TO_USPACE(calldata, &call->data)) {
		return 0;
	}
	return (__native)call;
}

/** Connect irq handler to task */
__native sys_ipc_register_irq(__native irq, irq_code_t *ucode)
{
	if (!(cap_get(TASK) & CAP_IRQ_REG))
		return EPERM;

	if (irq >= IRQ_COUNT)
		return (__native) ELIMIT;

	irq_ipc_bind_arch(irq);

	return ipc_irq_register(&TASK->answerbox, irq, ucode);
}

/* Disconnect irq handler from task */
__native sys_ipc_unregister_irq(__native irq)
{
	if (!(cap_get(TASK) & CAP_IRQ_REG))
		return EPERM;

	if (irq >= IRQ_COUNT)
		return (__native) ELIMIT;

	ipc_irq_unregister(&TASK->answerbox, irq);

	return 0;
}
