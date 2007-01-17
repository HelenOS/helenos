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

/* Lock ordering
 *
 * First the answerbox, then the phone
 */

#include <synch/spinlock.h>
#include <synch/waitq.h>
#include <synch/synch.h>
#include <ipc/ipc.h>
#include <errno.h>
#include <mm/slab.h>
#include <arch.h>
#include <proc/task.h>
#include <memstr.h>
#include <debug.h>

#include <print.h>
#include <proc/thread.h>
#include <arch/interrupt.h>
#include <ipc/irq.h>

/* Open channel that is assigned automatically to new tasks */
answerbox_t *ipc_phone_0 = NULL;

static slab_cache_t *ipc_call_slab;

/* Initialize new call */
static void _ipc_call_init(call_t *call)
{
	memsetb((uintptr_t)call, sizeof(*call), 0);
	call->callerbox = &TASK->answerbox;
	call->sender = TASK;
}

/** Allocate & initialize call structure
 * 
 * The call is initialized, so that the reply will be directed
 * to TASK->answerbox
 *
 * @param flags Parameters for slab_alloc (ATOMIC, etc.)
 */
call_t * ipc_call_alloc(int flags)
{
	call_t *call;

	call = slab_alloc(ipc_call_slab, flags);
	_ipc_call_init(call);

	return call;
}

/** Initialize allocated call */
void ipc_call_static_init(call_t *call)
{
	_ipc_call_init(call);
	call->flags |= IPC_CALL_STATIC_ALLOC;
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
	spinlock_initialize(&box->lock, "ipc_box_lock");
	spinlock_initialize(&box->irq_lock, "ipc_box_irqlock");
	waitq_initialize(&box->wq);
	list_initialize(&box->connected_phones);
	list_initialize(&box->calls);
	list_initialize(&box->dispatched_calls);
	list_initialize(&box->answers);
	list_initialize(&box->irq_notifs);
	list_initialize(&box->irq_head);
	box->task = TASK;
}

/** Connect phone to answerbox */
void ipc_phone_connect(phone_t *phone, answerbox_t *box)
{
	spinlock_lock(&phone->lock);

	phone->state = IPC_PHONE_CONNECTED;
	phone->callee = box;

	spinlock_lock(&box->lock);
	list_append(&phone->link, &box->connected_phones);
	spinlock_unlock(&box->lock);

	spinlock_unlock(&phone->lock);
}

/** Initialize phone structure and connect phone to answerbox
 */
void ipc_phone_init(phone_t *phone)
{
	spinlock_initialize(&phone->lock, "phone_lock");
	phone->callee = NULL;
	phone->state = IPC_PHONE_FREE;
	atomic_set(&phone->active_calls, 0);
}

/** Helper function to facilitate synchronous calls */
void ipc_call_sync(phone_t *phone, call_t *request)
{
	answerbox_t sync_box; 

	ipc_answerbox_init(&sync_box);

	/* We will receive data on special box */
	request->callerbox = &sync_box;

	ipc_call(phone, request);
	ipc_wait_for_call(&sync_box, SYNCH_NO_TIMEOUT, SYNCH_FLAGS_NONE);
}

/** Answer message that was not dispatched and is not entered in
 * any queue
 */
static void _ipc_answer_free_call(call_t *call)
{
	answerbox_t *callerbox = call->callerbox;

	call->flags |= IPC_CALL_ANSWERED;

	spinlock_lock(&callerbox->lock);
	list_append(&call->link, &callerbox->answers);
	spinlock_unlock(&callerbox->lock);
	waitq_wakeup(&callerbox->wq, 0);
}

/** Answer message, that is in callee queue
 *
 * @param box Answerbox that is answering the message
 * @param call Modified request that is being sent back
 */
void ipc_answer(answerbox_t *box, call_t *call)
{
	/* Remove from active box */
	spinlock_lock(&box->lock);
	list_remove(&call->link);
	spinlock_unlock(&box->lock);
	/* Send back answer */
	_ipc_answer_free_call(call);
}

/** Simulate sending back a message
 *
 * Most errors are better handled by forming a normal backward
 * message and sending it as a normal answer.
 */
void ipc_backsend_err(phone_t *phone, call_t *call, unative_t err)
{
	call->data.phone = phone;
	atomic_inc(&phone->active_calls);
	IPC_SET_RETVAL(call->data, err);
	_ipc_answer_free_call(call);
}

/* Unsafe unchecking ipc_call */
static void _ipc_call(phone_t *phone, answerbox_t *box, call_t *call)
{
	if (! (call->flags & IPC_CALL_FORWARDED)) {
		atomic_inc(&phone->active_calls);
		call->data.phone = phone;
	}

	spinlock_lock(&box->lock);
	list_append(&call->link, &box->calls);
	spinlock_unlock(&box->lock);
	waitq_wakeup(&box->wq, 0);
}

/** Send a asynchronous request using phone to answerbox
 *
 * @param phone Phone connected to answerbox.
 * @param call Structure representing the call.
 */
int ipc_call(phone_t *phone, call_t *call)
{
	answerbox_t *box;

	spinlock_lock(&phone->lock);
	if (phone->state != IPC_PHONE_CONNECTED) {
		spinlock_unlock(&phone->lock);
		if (call->flags & IPC_CALL_FORWARDED) {
			IPC_SET_RETVAL(call->data, EFORWARD);
			_ipc_answer_free_call(call);
		} else {
			if (phone->state == IPC_PHONE_HUNGUP)
				ipc_backsend_err(phone, call, EHANGUP);
			else
				ipc_backsend_err(phone, call, ENOENT);
		}
		return ENOENT;
	}
	box = phone->callee;
	_ipc_call(phone, box, call);
	
	spinlock_unlock(&phone->lock);
	return 0;
}

/** Disconnect phone from answerbox
 *
 * This call leaves the phone in HUNGUP state. The change to 'free' is done
 * lazily later.
 *
 * @param phone Phone to be hung up
 *              
 * @return 0 - phone disconnected, -1 - the phone was already disconnected
 */
int ipc_phone_hangup(phone_t *phone)
{
	answerbox_t *box;
	call_t *call;
	
	spinlock_lock(&phone->lock);
	if (phone->state == IPC_PHONE_FREE || phone->state ==IPC_PHONE_HUNGUP \
	    || phone->state == IPC_PHONE_CONNECTING) {
		spinlock_unlock(&phone->lock);
		return -1;
	}
	box = phone->callee;
	if (phone->state != IPC_PHONE_SLAMMED) {
		/* Remove myself from answerbox */
		spinlock_lock(&box->lock);
		list_remove(&phone->link);
		spinlock_unlock(&box->lock);

		if (phone->state != IPC_PHONE_SLAMMED) {
			call = ipc_call_alloc(0);
			IPC_SET_METHOD(call->data, IPC_M_PHONE_HUNGUP);
			call->flags |= IPC_CALL_DISCARD_ANSWER;
			_ipc_call(phone, box, call);
		}
	}

	phone->state = IPC_PHONE_HUNGUP;
	spinlock_unlock(&phone->lock);

	return 0;
}

/** Forwards call from one answerbox to a new one
 *
 * @param call Call to be redirected.
 * @param newphone Phone to target answerbox.
 * @param oldbox Old answerbox
 * @return 0 on forward ok, error code, if there was error
 * 
 * - the return value serves only as an information for the forwarder,
 *   the original caller is notified automatically with EFORWARD
 */
int ipc_forward(call_t *call, phone_t *newphone, answerbox_t *oldbox)
{
	spinlock_lock(&oldbox->lock);
	list_remove(&call->link);
	spinlock_unlock(&oldbox->lock);

	return ipc_call(newphone, call);
}


/** Wait for phone call 
 *
 * @param box Answerbox expecting the call.
 * @param usec Timeout in microseconds. See documentation for waitq_sleep_timeout() for
 *	       decription of its special meaning.
 * @param flags Select mode of sleep operation. See documentation for waitq_sleep_timeout()i
 * 		for description of its special meaning.
 * @return Recived message address
 * - to distinguish between call and answer, look at call->flags
 */
call_t * ipc_wait_for_call(answerbox_t *box, uint32_t usec, int flags)
{
	call_t *request;
	ipl_t ipl;
	int rc;

restart:
	rc = waitq_sleep_timeout(&box->wq, usec, flags);
	if (SYNCH_FAILED(rc))
		return NULL;
	
	spinlock_lock(&box->lock);
	if (!list_empty(&box->irq_notifs)) {
		ipl = interrupts_disable();
		spinlock_lock(&box->irq_lock);

		request = list_get_instance(box->irq_notifs.next, call_t, link);
		list_remove(&request->link);

		spinlock_unlock(&box->irq_lock);
		interrupts_restore(ipl);
	} else if (!list_empty(&box->answers)) {
		/* Handle asynchronous answers */
		request = list_get_instance(box->answers.next, call_t, link);
		list_remove(&request->link);
		atomic_dec(&request->data.phone->active_calls);
	} else if (!list_empty(&box->calls)) {
		/* Handle requests */
		request = list_get_instance(box->calls.next, call_t, link);
		list_remove(&request->link);
		/* Append request to dispatch queue */
		list_append(&request->link, &box->dispatched_calls);
	} else {
		/* This can happen regularly after ipc_cleanup */
		spinlock_unlock(&box->lock);
		goto restart;
	}
	spinlock_unlock(&box->lock);
	return request;
}

/** Answer all calls from list with EHANGUP msg */
static void ipc_cleanup_call_list(link_t *lst)
{
	call_t *call;

	while (!list_empty(lst)) {
		call = list_get_instance(lst->next, call_t, link);
		list_remove(&call->link);

		IPC_SET_RETVAL(call->data, EHANGUP);
		_ipc_answer_free_call(call);
	}
}

/** Cleans up all IPC communication of the current task
 *
 * Note: ipc_hangup sets returning answerbox to TASK->answerbox, you
 * have to change it as well if you want to cleanup other current then current.
 */
void ipc_cleanup(void)
{
	int i;
	call_t *call;
	phone_t *phone;

	/* Disconnect all our phones ('ipc_phone_hangup') */
	for (i=0;i < IPC_MAX_PHONES; i++)
		ipc_phone_hangup(&TASK->phones[i]);

	/* Disconnect all connected irqs */
	ipc_irq_cleanup(&TASK->answerbox);

	/* Disconnect all phones connected to our answerbox */
restart_phones:
	spinlock_lock(&TASK->answerbox.lock);
	while (!list_empty(&TASK->answerbox.connected_phones)) {
		phone = list_get_instance(TASK->answerbox.connected_phones.next,
					  phone_t, link);
		if (! spinlock_trylock(&phone->lock)) {
			spinlock_unlock(&TASK->answerbox.lock);
			goto restart_phones;
		}
		
		/* Disconnect phone */
		ASSERT(phone->state == IPC_PHONE_CONNECTED);
		phone->state = IPC_PHONE_SLAMMED;
		list_remove(&phone->link);

		spinlock_unlock(&phone->lock);
	}

	/* Answer all messages in 'calls' and 'dispatched_calls' queues */
	ipc_cleanup_call_list(&TASK->answerbox.dispatched_calls);
	ipc_cleanup_call_list(&TASK->answerbox.calls);
	spinlock_unlock(&TASK->answerbox.lock);
	
	/* Wait for all async answers to arrive */
	while (1) {
		/* Go through all phones, until all are FREE... */
		/* Locking not needed, no one else should modify
		 * it, when we are in cleanup */
		for (i=0;i < IPC_MAX_PHONES; i++) {
			if (TASK->phones[i].state == IPC_PHONE_HUNGUP && \
			    atomic_get(&TASK->phones[i].active_calls) == 0)
				TASK->phones[i].state = IPC_PHONE_FREE;
			
			/* Just for sure, we might have had some 
			 * IPC_PHONE_CONNECTING phones */
			if (TASK->phones[i].state == IPC_PHONE_CONNECTED)
				ipc_phone_hangup(&TASK->phones[i]);
			/* If the hangup succeeded, it has sent a HANGUP 
			 * message, the IPC is now in HUNGUP state, we
			 * wait for the reply to come */
			
			if (TASK->phones[i].state != IPC_PHONE_FREE)
				break;
		}
		/* Voila, got into cleanup */
		if (i == IPC_MAX_PHONES)
			break;
		
		call = ipc_wait_for_call(&TASK->answerbox, SYNCH_NO_TIMEOUT, SYNCH_FLAGS_NONE);
		ASSERT((call->flags & IPC_CALL_ANSWERED) || (call->flags & IPC_CALL_NOTIF));
		ASSERT(! (call->flags & IPC_CALL_STATIC_ALLOC));
		
		atomic_dec(&TASK->active_calls);
		ipc_call_free(call);
	}
}


/** Initilize IPC subsystem */
void ipc_init(void)
{
	ipc_call_slab = slab_cache_create("ipc_call", sizeof(call_t), 0, NULL, NULL, 0);
}


/** Kconsole - list answerbox contents */
void ipc_print_task(task_id_t taskid)
{
	task_t *task;
	int i;
	call_t *call;
	link_t *tmp;
	
	spinlock_lock(&tasks_lock);
	task = task_find_by_id(taskid);
	if (task) 
		spinlock_lock(&task->lock);
	spinlock_unlock(&tasks_lock);
	if (!task)
		return;

	/* Print opened phones & details */
	printf("PHONE:\n");
	for (i=0; i < IPC_MAX_PHONES;i++) {
		spinlock_lock(&task->phones[i].lock);
		if (task->phones[i].state != IPC_PHONE_FREE) {
			printf("%d: ",i);
			switch (task->phones[i].state) {
			case IPC_PHONE_CONNECTING:
				printf("connecting ");
				break;
			case IPC_PHONE_CONNECTED:
				printf("connected to: %p ", 
				       task->phones[i].callee);
				break;
			case IPC_PHONE_SLAMMED:
				printf("slammed by: %p ", 
				       task->phones[i].callee);
				break;
			case IPC_PHONE_HUNGUP:
				printf("hung up - was: %p ", 
				       task->phones[i].callee);
				break;
			default:
				break;
			}
			printf("active: %d\n", atomic_get(&task->phones[i].active_calls));
		}
		spinlock_unlock(&task->phones[i].lock);
	}


	/* Print answerbox - calls */
	spinlock_lock(&task->answerbox.lock);
	printf("ABOX - CALLS:\n");
	for (tmp=task->answerbox.calls.next; tmp != &task->answerbox.calls;tmp = tmp->next) {
		call = list_get_instance(tmp, call_t, link);
		printf("Callid: %p Srctask:%lld M:%d A1:%d A2:%d A3:%d Flags:%x\n",call,
		       call->sender->taskid, IPC_GET_METHOD(call->data), IPC_GET_ARG1(call->data),
		       IPC_GET_ARG2(call->data), IPC_GET_ARG3(call->data), call->flags);
	}
	/* Print answerbox - calls */
	printf("ABOX - DISPATCHED CALLS:\n");
	for (tmp=task->answerbox.dispatched_calls.next; 
	     tmp != &task->answerbox.dispatched_calls; 
	     tmp = tmp->next) {
		call = list_get_instance(tmp, call_t, link);
		printf("Callid: %p Srctask:%lld M:%d A1:%d A2:%d A3:%d Flags:%x\n",call,
		       call->sender->taskid, IPC_GET_METHOD(call->data), IPC_GET_ARG1(call->data),
		       IPC_GET_ARG2(call->data), IPC_GET_ARG3(call->data), call->flags);
	}
	/* Print answerbox - calls */
	printf("ABOX - ANSWERS:\n");
	for (tmp=task->answerbox.answers.next; tmp != &task->answerbox.answers; tmp = tmp->next) {
		call = list_get_instance(tmp, call_t, link);
		printf("Callid:%p M:%d A1:%d A2:%d A3:%d Flags:%x\n",call,
		       IPC_GET_METHOD(call->data), IPC_GET_ARG1(call->data),
		       IPC_GET_ARG2(call->data), IPC_GET_ARG3(call->data), call->flags);
	}

	spinlock_unlock(&task->answerbox.lock);
	spinlock_unlock(&task->lock);
}

/** @}
 */
