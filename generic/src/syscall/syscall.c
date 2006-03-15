/*
 * Copyright (C) 2005 Martin Decky
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

#include <syscall/syscall.h>
#include <proc/thread.h>
#include <mm/as.h>
#include <print.h>
#include <putchar.h>
#include <ipc/ipc.h>
#include <errno.h>
#include <proc/task.h>
#include <arch.h>
#include <debug.h>

static __native sys_ctl(void) {
	printf("Thread finished\n");
	thread_exit();
	/* Unreachable */
	return 0;
}

static __native sys_io(int fd, const void * buf, size_t count) {
	
	// TODO: buf sanity checks and a lot of other stuff ...

	size_t i;
	
	for (i = 0; i < count; i++)
		putchar(((char *) buf)[i]);
	
	return count;
}

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

/** Send a call over IPC, wait for reply, return to user
 *
 * @return Call identification, returns -1 on fatal error, 
           -2 on 'Too many async request, handle answers first
 */
static __native sys_ipc_call_sync_fast(__native phoneid, __native method, 
				       __native arg1, __native *data)
{
	call_t call;
	phone_t *phone;
	/* Special answerbox for synchronous messages */

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
static __native sys_ipc_call_sync(__native phoneid, __native *question, 
				  __native *reply)
{
	call_t call;
	phone_t *phone;
	/* Special answerbox for synchronous messages */

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
	if (atomic_inc_post(&TASK->active_calls) > IPC_MAX_ASYNC_CALLS) {
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
static __native sys_ipc_call_async_fast(__native phoneid, __native method, 
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
static __native sys_ipc_call_async(__native phoneid, __native *data)
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
static __native sys_ipc_answer_fast(__native callid, __native retval, 
				    __native arg1, __native arg2)
{
	call_t *call;

	/* Check that the user is not sending us answer callid */
	ASSERT(! (callid & 1));
	/* TODO: Check that the callid is in the dispatch table */
	call = (call_t *) callid;

	IPC_SET_RETVAL(call->data, retval);
	IPC_SET_ARG1(call->data, arg1);
	IPC_SET_ARG2(call->data, arg2);

	ipc_answer(&TASK->answerbox, call);
	return 0;
}

/** Send IPC answer */
static __native sys_ipc_answer(__native callid, __native *data)
{
	call_t *call;

	/* Check that the user is not sending us answer callid */
	ASSERT(! (callid & 1));
	/* TODO: Check that the callid is in the dispatch table */
	call = (call_t *) callid;
	copy_from_uspace(&call->data, data, sizeof(call->data));
	ipc_answer(&TASK->answerbox, call);

	return 0;
}

/** Wait for incoming ipc call or answer
 *
 * @param result 
 * @param flags
 * @return Callid, if callid & 1, then the call is answer
 */
static __native sys_ipc_wait_for_call(__native *calldata, task_id_t *taskid,
				      __native flags)
{
	call_t *call;
	
	call = ipc_wait_for_call(&TASK->answerbox, flags);

	copy_to_uspace(calldata, &call->data, sizeof(call->data));

	if (call->flags & IPC_CALL_ANSWERED) {
		ASSERT(! (call->flags & IPC_CALL_STATIC_ALLOC));
		ipc_call_free(call);
		atomic_dec(&TASK->active_calls);
		return ((__native)call) | IPC_CALLID_ANSWERED;
	}
	copy_to_uspace(taskid, (void *)&TASK->taskid, sizeof(TASK->taskid));
	return (__native)call;
}

static __native sys_mremap(void *address, size_t size, unsigned long flags)
{
	return as_remap(AS, (__address) address, size, 0);
}

syshandler_t syscall_table[SYSCALL_END] = {
	sys_ctl,
	sys_io,
	sys_mremap,
	sys_ipc_call_sync_fast,
	sys_ipc_call_sync,
	sys_ipc_call_async_fast,
	sys_ipc_call_async,
	sys_ipc_answer_fast,
	sys_ipc_answer,
	sys_ipc_wait_for_call
};
