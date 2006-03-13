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

#include <synch/waitq.h>
#include <ipc/ipc.h>
#include <errno.h>
#include <mm/slab.h>
#include <arch.h>
#include <proc/task.h>
#include <memstr.h>
#include <debug.h>

#include <print.h>
#include <proc/thread.h>

answerbox_t *ipc_central_box;

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

/** Deallocate call stracuture */
void ipc_call_free(call_t *call)
{
	slab_free(ipc_call_slab, call);
}

/** Initialize answerbox structure
 */
void ipc_answerbox_init(answerbox_t *box)
{
	spinlock_initialize(&box->lock, "abox_lock");
	waitq_initialize(&box->wq);
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
	spinlock_lock(&box->lock);
	list_append(&phone->list, &box->connected_phones);
	spinlock_unlock(&box->lock);
}

/** Disconnect phone from answerbox */
void ipc_phone_destroy(phone_t *phone)
{
	answerbox_t *box = phone->callee;
	
	ASSERT(box);

	spinlock_lock(&box->lock);
	list_remove(&phone->list);
	spinlock_unlock(&box->lock);
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

	spinlock_lock(&box->lock);
	list_append(&request->list, &box->calls);
	spinlock_unlock(&box->lock);
	waitq_wakeup(&box->wq, 0);
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

	spinlock_lock(&box->lock);
	spinlock_lock(&callerbox->lock);

	list_remove(&request->list);
	list_append(&request->list, &callerbox->answers);
	waitq_wakeup(&callerbox->wq, 0);

	spinlock_unlock(&callerbox->lock);
	spinlock_unlock(&box->lock);
}

/** Wait for phone call 
 *
 * @return Recived message address
 * - to distinguish between call and answer, look at call->flags
 */
call_t * ipc_wait_for_call(answerbox_t *box, int flags)
{
	call_t *request;

	if ((flags & IPC_WAIT_NONBLOCKING)) {
		if (waitq_sleep_timeout(&box->wq,SYNCH_NO_TIMEOUT,SYNCH_NON_BLOCKING) == ESYNCH_WOULD_BLOCK)
			return 0;
	} else {
		waitq_sleep(&box->wq);
	}


	// TODO - might need condition variable+mutex if we want to support
	// removing of requests from queue before dispatch
	spinlock_lock(&box->lock);
	/* Handle answers first */
	if (!list_empty(&box->answers)) {
		request = list_get_instance(box->answers.next, call_t, list);
		list_remove(&request->list);
	} else {
		ASSERT (! list_empty(&box->calls));
		request = list_get_instance(box->calls.next, call_t, list);
		list_remove(&request->list);
		/* Append request to dispatch queue */
		list_append(&request->list, &box->dispatched_calls);
	}
	spinlock_unlock(&box->lock);

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

static void ipc_phonecompany_thread(void *data)
{
	call_t *call;

	printf("Phone company started.\n");
	while (1) {
		call = ipc_wait_for_call(&TASK->answerbox, 0);
		printf("Received phone call - %P %P\n",
		       call->data[0], call->data[1]);
		call->data[0] = 0xbabaaaee;;
		call->data[1] = 0xaaaaeeee;
		ipc_answer(&TASK->answerbox, call);
		printf("Call answered.\n");
	}
}

void ipc_create_phonecompany(void)
{
	thread_t *t;
	
	if ((t = thread_create(ipc_phonecompany_thread, "phonecompany", 
			       TASK, 0)))
		thread_ready(t);
	else
		panic("thread_create/phonecompany");

	ipc_central_box = &TASK->answerbox;
}
