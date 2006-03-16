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

#include <errno.h>
#include <mm/page.h>
#include <memstr.h>
#include <debug.h>
#include <ipc/ipc.h>
#include <ipc/sysipc.h>


#include <print.h>
#include <arch.h>
#include <proc/thread.h>

/* TODO: multi-threaded connect-to-me can cause race condition
 * on phone, add counter + thread_kill??
 *
 */

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
	return 1;
}

/** Find call_t * in call table according to callid
 *
 * @return NULL on not found, otherwise pointer to call structure
 */
static inline call_t * get_call(__native callid)
{
	/* TODO: Traverse list of dispatched calls and find one */
	/* TODO: locking of call, ripping it from dispatched calls etc.  */
	return (call_t *) callid;
}

/** Return pointer to phone identified by phoneid or NULL if non-existent */
static phone_t * get_phone(__native phoneid)
{
	phone_t *phone;

	if (phoneid >= IPC_MAX_PHONES)
		return NULL;

	phone = &TASK->phones[phoneid];
	if (!phone->callee)
		return NULL;
	return phone;
}

/** Allocate new phone slot in current TASK structure */
static int phone_alloc(void)
{
	int i;

	spinlock_lock(&TASK->lock);
	
	for (i=0; i < IPC_MAX_PHONES; i++) {
		if (!TASK->phones[i].busy) {
			TASK->phones[i].busy = 1;
			break;
		}
	}
	spinlock_unlock(&TASK->lock);

	if (i >= IPC_MAX_PHONES)
		return -1;
	return i;
}

/** Disconnect phone */
static void phone_dealloc(int phoneid)
{
	spinlock_lock(&TASK->lock);

	ASSERT(TASK->phones[phoneid].busy);

	if (TASK->phones[phoneid].callee)
		ipc_phone_destroy(&TASK->phones[phoneid]);

	TASK->phones[phoneid].busy = 0;
	spinlock_unlock(&TASK->lock);
}

static void phone_connect(int phoneid, answerbox_t *box)
{
	phone_t *phone = &TASK->phones[phoneid];
	
	ipc_phone_connect(phone, box);
}

/****************************************************/
/* Functions that preprocess answer before sending 
 * it to the recepient
 */

/** Return true if the caller (ipc_answer) should save
 * the old call contents and call answer_preprocess
 */
static inline int answer_will_preprocess(call_t *call)
{
	if (IPC_GET_METHOD(call->data) == IPC_M_CONNECTTOME)
		return 1;
	if (IPC_GET_METHOD(call->data) == IPC_M_CONNECTMETO)
		return 1;
	return 0;
}

/** Interpret process answer as control information */
static inline void answer_preprocess(call_t *answer, ipc_data_t *olddata)
{
	int phoneid;

	if (IPC_GET_METHOD(*olddata) == IPC_M_CONNECTTOME) {
		phoneid = IPC_GET_ARG3(*olddata);
		if (IPC_GET_RETVAL(answer->data)) {
			/* The connection was not accepted */
			phone_dealloc(phoneid);
		} else {
			/* The connection was accepted */
			phone_connect(phoneid,&answer->sender->answerbox);
		}
	} else if (IPC_GET_METHOD(*olddata) == IPC_M_CONNECTMETO) {
		/* If the users accepted call, connect */
		if (!IPC_GET_RETVAL(answer->data)) {
			printf("Connecting Phone %P\n",IPC_GET_ARG3(*olddata));
			ipc_phone_connect((phone_t *)IPC_GET_ARG3(*olddata),
					  &TASK->answerbox);
		}
	}
}

/****************************************************/
/* Functions called to process received call/answer 
 * before passing to uspace
 */

/** Do basic kernel processing of received call answer */
static int process_answer(answerbox_t *box,call_t *call)
{
	return 0;
}

/** Do basic kernel processing of received call request
 *
 * @return 0 - the call should be passed to userspace, 1 - ignore call
 */
static int process_request(answerbox_t *box,call_t *call)
{
	int phoneid;

	if (IPC_GET_METHOD(call->data) == IPC_M_CONNECTTOME) {
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
				__native arg1, __native *data)
{
	call_t call;
	phone_t *phone;

	phone = get_phone(phoneid);
	if (!phone)
		return ENOENT;

	if (is_system_method(method))
		return EPERM;

	ipc_call_init(&call);
	IPC_SET_METHOD(call.data, method);
	IPC_SET_ARG1(call.data, arg1);
	
	ipc_call_sync(phone, &call);

	copy_to_uspace(data, &call.data, sizeof(call.data));

	return 0;
}

/** Synchronous IPC call allowing to send whole message */
__native sys_ipc_call_sync(__native phoneid, __native *question, 
			   __native *reply)
{
	call_t call;
	phone_t *phone;

	phone = get_phone(phoneid);
	if (!phone)
		return ENOENT;

	ipc_call_init(&call);
	copy_from_uspace(&call.data, question, sizeof(call.data));

	if (is_system_method(IPC_GET_METHOD(call.data)))
		return EPERM;
	
	ipc_call_sync(phone, &call);

	copy_to_uspace(reply, &call.data, sizeof(call.data));

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

	phone = get_phone(phoneid);
	if (!phone)
		return IPC_CALLRET_FATAL;

	if (is_system_method(method))
		return IPC_CALLRET_FATAL;

	if (check_call_limit())
		return IPC_CALLRET_TEMPORARY;

	call = ipc_call_alloc();
	IPC_SET_METHOD(call->data, method);
	IPC_SET_ARG1(call->data, arg1);
	IPC_SET_ARG2(call->data, arg2);

	ipc_call(phone, call);

	return (__native) call;
}

/** Synchronous IPC call allowing to send whole message
 *
 * @return The same as sys_ipc_call_async
 */
__native sys_ipc_call_async(__native phoneid, __native *data)
{
	call_t *call;
	phone_t *phone;

	phone = get_phone(phoneid);
	if (!phone)
		return IPC_CALLRET_FATAL;

	if (check_call_limit())
		return IPC_CALLRET_TEMPORARY;

	call = ipc_call_alloc();
	copy_from_uspace(&call->data, data, sizeof(call->data));

	if (is_system_method(IPC_GET_METHOD(call->data))) {
		ipc_call_free(call);
		return EPERM;
	}
	
	ipc_call(phone, call);

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

	phone = get_phone(phoneid);
	if (!phone) {
		IPC_SET_RETVAL(call->data, EFORWARD);
		ipc_answer(&TASK->answerbox, call);
		return ENOENT;
	}

	if (!is_forwardable(IPC_GET_METHOD(call->data))) {
		IPC_SET_RETVAL(call->data, EFORWARD);
		ipc_answer(&TASK->answerbox, call);
		return EPERM;
	}

	/* Userspace is not allowed to change method of system methods
	 * on forward, allow changing ARG1 and ARG2 by means of method and arg1
	 */
	if (is_system_method(IPC_GET_METHOD(call->data))) {
		IPC_SET_ARG1(call->data, method);
		IPC_SET_ARG2(call->data, arg1);
	} else {
		IPC_SET_METHOD(call->data, method);
		IPC_SET_ARG1(call->data, arg1);
	}

	ipc_forward(call, phone->callee, &TASK->answerbox);

	return 0;
}

/** Send IPC answer */
__native sys_ipc_answer_fast(__native callid, __native retval, 
			     __native arg1, __native arg2)
{
	call_t *call;
	ipc_data_t saved_data;
	int preprocess = 0;

	call = get_call(callid);
	if (!call)
		return ENOENT;

	if (answer_will_preprocess(call)) {
		memcpy(&saved_data, &call->data, sizeof(call->data));
		preprocess = 1;
	}

	IPC_SET_RETVAL(call->data, retval);
	IPC_SET_ARG1(call->data, arg1);
	IPC_SET_ARG2(call->data, arg2);

	if (preprocess)
		answer_preprocess(call, &saved_data);

	ipc_answer(&TASK->answerbox, call);
	return 0;
}

/** Send IPC answer */
inline __native sys_ipc_answer(__native callid, __native *data)
{
	call_t *call;
	ipc_data_t saved_data;
	int preprocess = 0;

	call = get_call(callid);
	if (!call)
		return ENOENT;

	if (answer_will_preprocess(call)) {
		memcpy(&saved_data, &call->data, sizeof(call->data));
		preprocess = 1;
	}
	copy_from_uspace(&call->data, data, sizeof(call->data));

	if (preprocess)
		answer_preprocess(call, &saved_data);
	
	ipc_answer(&TASK->answerbox, call);

	return 0;
}

/** Ask the other side of connection to do 'callback' connection
 *
 * @return 0 if no error, error otherwise 
 */
__native sys_ipc_connect_to_me(__native phoneid, __native arg1,
			       __native arg2, task_id_t *taskid)
{
	call_t call;
	phone_t *phone;

	phone = get_phone(phoneid);
	if (!phone)
		return ENOENT;

	ipc_call_init(&call);
	IPC_SET_METHOD(call.data, IPC_M_CONNECTTOME);
	IPC_SET_ARG1(call.data, arg1);
	IPC_SET_ARG2(call.data, arg2);
	
	ipc_call_sync(phone, &call);

	if (!IPC_GET_RETVAL(call.data) && taskid)
		copy_to_uspace(taskid, 
			       &phone->callee->task->taskid,
			       sizeof(TASK->taskid));

	return IPC_GET_RETVAL(call.data);
}

/** Ask target process to connect me somewhere
 *
 * @return phoneid - on success, error otherwise
 */
__native sys_ipc_connect_me_to(__native phoneid, __native arg1,
			       __native arg2)
{
	call_t call;
	phone_t *phone;
	int newphid;

	phone = get_phone(phoneid);
	if (!phone)
		return ENOENT;

	newphid = phone_alloc();
	if (newphid < 0)
		return ELIMIT;

	ipc_call_init(&call);
	IPC_SET_METHOD(call.data, IPC_M_CONNECTMETO);
	IPC_SET_ARG1(call.data, arg1);
	IPC_SET_ARG2(call.data, arg2);
	IPC_SET_ARG3(call.data, (__native)&TASK->phones[newphid]);

	ipc_call_sync(phone, &call);

	if (IPC_GET_RETVAL(call.data)) { /* Connection failed */
		phone_dealloc(newphid);
		return IPC_GET_RETVAL(call.data);
	}

	return newphid;
}

/** Wait for incoming ipc call or answer
 *
 * Generic function - can serve either as inkernel or userspace call
 * - inside kernel does probably unnecessary copying of data (TODO)
 *
 * @param result 
 * @param taskid 
 * @param flags
 * @return Callid, if callid & 1, then the call is answer
 */
inline __native sys_ipc_wait_for_call(ipc_data_t *calldata, 
				      task_id_t *taskid,
				      __native flags)
					     
{
	call_t *call;

restart:	
	call = ipc_wait_for_call(&TASK->answerbox, flags);

	if (call->flags & IPC_CALL_ANSWERED) {
		if (process_answer(&TASK->answerbox, call))
			goto restart;

		copy_to_uspace(calldata, &call->data, sizeof(call->data));
		atomic_dec(&TASK->active_calls);
		ASSERT(! (call->flags & IPC_CALL_STATIC_ALLOC));
		ipc_call_free(call);

		return ((__native)call) | IPC_CALLID_ANSWERED;
	}
	if (process_request(&TASK->answerbox, call))
		goto restart;
	copy_to_uspace(calldata, &call->data, sizeof(call->data));
	copy_to_uspace(taskid, (void *)&TASK->taskid, sizeof(TASK->taskid));
	return (__native)call;
}

