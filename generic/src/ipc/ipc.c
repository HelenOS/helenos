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

/* Lock ordering
 *
 * First the answerbox, then the phone
 */

#include <synch/condvar.h>
#include <synch/mutex.h>
#include <ipc/ipc.h>
#include <errno.h>
#include <mm/slab.h>
#include <arch.h>
#include <proc/task.h>
#include <memstr.h>
#include <debug.h>

#include <print.h>
#include <proc/thread.h>

/* Open channel that is assigned automatically to new tasks */
answerbox_t *ipc_phone_0 = NULL;

static slab_cache_t *ipc_call_slab;

/** Allocate & initialize call structure
 * 
 * The call is initialized, so that the reply will be directed
 * to TASK->answerbox
 */
call_t * ipc_call_alloc(void)
{
	call_t *call;

	call = slab_alloc(ipc_call_slab, 0);
	memsetb((__address)call, sizeof(*call), 0);
	call->callerbox = &TASK->answerbox;

	return call;
}

/** Initialize allocated call */
void ipc_call_init(call_t *call)
{
	call->callerbox = &TASK->answerbox;
	call->flags = IPC_CALL_STATIC_ALLOC;
}

/** Deallocate call stracuture */
void ipc_call_free(call_t *call)
{
	slab_free(ipc_call_slab, call);
}

/** Initialize answerbox structure
 */
void ipc_answerbox_init(answerbox_t *box)
{
	mutex_initialize(&box->mutex);
	condvar_initialize(&box->cv);
	list_initialize(&box->connected_phones);
	list_initialize(&box->calls);
	list_initialize(&box->dispatched_calls);
	list_initialize(&box->answers);
}

/** Initialize phone structure and connect phone to naswerbox
 */
void ipc_phone_init(phone_t *phone, answerbox_t *box)
{
	spinlock_initialize(&phone->lock, "phone_lock");
	
	phone->callee = box;

	mutex_lock(&box->mutex);
	list_append(&phone->list, &box->connected_phones);
	mutex_unlock(&box->mutex);
}

/** Disconnect phone from answerbox */
void ipc_phone_destroy(phone_t *phone)
{
	answerbox_t *box = phone->callee;
	
	ASSERT(box);

	mutex_lock(&box->mutex);
	list_remove(&phone->list);
	mutex_unlock(&box->mutex);
}

/** Helper function to facilitate synchronous calls */
void ipc_call_sync(phone_t *phone, call_t *request)
{
	answerbox_t sync_box; 

	ipc_answerbox_init(&sync_box);

	/* We will receive data on special box */
	request->callerbox = &sync_box;

	ipc_call(phone, request);
	ipc_wait_for_call(&sync_box, 0);
}

/** Send a asynchronous request using phone to answerbox
 *
 * @param phone Phone connected to answerbox
 * @param request Request to be sent
 */
void ipc_call(phone_t *phone, call_t *request)
{
	answerbox_t *box = phone->callee;

	ASSERT(box);

	mutex_lock(&box->mutex);
	list_append(&request->list, &box->calls);
	mutex_unlock(&box->mutex);
	condvar_signal(&box->cv);
}

/** Answer message back to phone
 *
 * @param box Answerbox that is answering the message
 * @param request Modified request that is being sent back
 */
void ipc_answer(answerbox_t *box, call_t *request)
{
	answerbox_t *callerbox = request->callerbox;

	request->flags |= IPC_CALL_ANSWERED;

	mutex_lock(&box->mutex);
	list_remove(&request->list);
	mutex_unlock(&box->mutex);

	mutex_lock(&callerbox->mutex);
	list_append(&request->list, &callerbox->answers);
	mutex_unlock(&callerbox->mutex);
	condvar_signal(&callerbox->cv);
}

/** Wait for phone call 
 *
 * @return Recived message address
 * - to distinguish between call and answer, look at call->flags
 */
call_t * ipc_wait_for_call(answerbox_t *box, int flags)
{
	call_t *request;

	mutex_lock(&box->mutex);
	while (1) { 
		if (!list_empty(&box->answers)) {
			/* Handle asynchronous answers */
			request = list_get_instance(box->answers.next, call_t, list);
			list_remove(&request->list);
		} else if (!list_empty(&box->calls)) {
			/* Handle requests */
			request = list_get_instance(box->calls.next, call_t, list);
			list_remove(&request->list);
			/* Append request to dispatch queue */
			list_append(&request->list, &box->dispatched_calls);
		} else {
			if (!(flags & IPC_WAIT_NONBLOCKING)) {
				condvar_wait(&box->cv, &box->mutex);
				continue;
			}
			if (condvar_trywait(&box->cv, &box->mutex) != ESYNCH_WOULD_BLOCK)
				continue;
			request = NULL;
		}
		break;
	}
	mutex_unlock(&box->mutex);
	return request;
}

/** Initilize ipc subsystem */
void ipc_init(void)
{
	ipc_call_slab = slab_cache_create("ipc_call",
					  sizeof(call_t),
					  0,
					  NULL, NULL, 0);
}
