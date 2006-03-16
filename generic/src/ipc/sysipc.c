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

/* TODO: multi-threaded connect-to-me can cause race condition
 * on phone, add counter + thread_kill??
 *
 * - don't allow userspace app to send system messages
 */

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
static int phone_alloc(answerbox_t *callee)
{
	int i;

	spinlock_lock(&TASK->lock);
	
	for (i=0; i < IPC_MAX_PHONES; i++) {
		if (!TASK->phones[i].callee) {
			TASK->phones[i].callee = callee;
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
	spinlock_lock(TASK->lock);

	ASSERT(TASK->phones[phoneid].callee);

	TASK->phones[phoneid].callee = NULL;
	spinlock_unlock(TASK->lock);
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
	return 0;
}

/** Interpret process answer as control information */
static inline void answer_preprocess(call_t *answer, call_t *call)
{
	int phoneid;

	if (IPC_GET_METHOD(call->data) == IPC_M_CONNECTTOME) {
		if (IPC_GET_RETVAL(answer->data)) {
			/* The connection was not accepted */
			/* TODO...race for multi-threaded process */
			phoneid = IPC_GET_ARG3(call->data);
			phone_dealloc(phoneid);
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
		phoneid = phone_alloc(&call->sender->answerbox);
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
		return IPC_CALLRET_FATAL;

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
		return IPC_CALLRET_FATAL;

	ipc_call_init(&call);
	copy_from_uspace(&call.data, question, sizeof(call.data));
	
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
	
	ipc_call(phone, call);

	return (__native) call;
}

/** Send IPC answer */
__native sys_ipc_answer_fast(__native callid, __native retval, 
			     __native arg1, __native arg2)
{
	call_t *call;
	call_t saved_call;
	int preprocess = 0;

	/* Check that the user is not sending us answer callid */
	ASSERT(! (callid & 1));
	/* TODO: Check that the callid is in the dispatch table */
	call = (call_t *) callid;

	if (answer_will_preprocess(call)) {
		memcpy(&saved_call.data, &call->data, sizeof(call->data));
		preprocess = 1;
	}

	IPC_SET_RETVAL(call->data, retval);
	IPC_SET_ARG1(call->data, arg1);
	IPC_SET_ARG2(call->data, arg2);
	if (preprocess)
		answer_preprocess(call, &saved_call);

	ipc_answer(&TASK->answerbox, call);
	return 0;
}

/** Send IPC answer */
inline __native sys_ipc_answer(__native callid, __native *data)
{
	call_t *call;
	call_t saved_call;
	int preprocess = 0;

	/* Check that the user is not sending us answer callid */
	ASSERT(! (callid & 1));
	/* TODO: Check that the callid is in the dispatch table */
	call = (call_t *) callid;

	if (answer_will_preprocess(call)) {
		memcpy(&saved_call.data, &call->data, sizeof(call->data));
		preprocess = 1;
	}
	copy_from_uspace(&call->data, data, sizeof(call->data));

	if (preprocess)
		answer_preprocess(call, &saved_call);
	
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
		return IPC_CALLRET_FATAL;

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

