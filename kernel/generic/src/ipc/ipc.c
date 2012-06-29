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
 * First the answerbox, then the phone.
 */

#include <synch/spinlock.h>
#include <synch/mutex.h>
#include <synch/waitq.h>
#include <ipc/ipc.h>
#include <abi/ipc/methods.h>
#include <ipc/kbox.h>
#include <ipc/event.h>
#include <errno.h>
#include <mm/slab.h>
#include <arch.h>
#include <proc/task.h>
#include <memstr.h>
#include <debug.h>
#include <print.h>
#include <console/console.h>
#include <proc/thread.h>
#include <arch/interrupt.h>
#include <ipc/irq.h>

/** Open channel that is assigned automatically to new tasks */
answerbox_t *ipc_phone_0 = NULL;

static slab_cache_t *ipc_call_slab;
static slab_cache_t *ipc_answerbox_slab;

/** Initialize a call structure.
 *
 * @param call Call structure to be initialized.
 *
 */
static void _ipc_call_init(call_t *call)
{
	memsetb(call, sizeof(*call), 0);
	call->callerbox = &TASK->answerbox;
	call->sender = TASK;
	call->buffer = NULL;
}

/** Allocate and initialize a call structure.
 *
 * The call is initialized, so that the reply will be directed to
 * TASK->answerbox.
 *
 * @param flags Parameters for slab_alloc (e.g FRAME_ATOMIC).
 *
 * @return If flags permit it, return NULL, or initialized kernel
 *         call structure.
 *
 */
call_t *ipc_call_alloc(unsigned int flags)
{
	call_t *call = slab_alloc(ipc_call_slab, flags);
	if (call)
		_ipc_call_init(call);
	
	return call;
}

/** Deallocate a call structure.
 *
 * @param call Call structure to be freed.
 *
 */
void ipc_call_free(call_t *call)
{
	/* Check to see if we have data in the IPC_M_DATA_SEND buffer. */
	if (call->buffer)
		free(call->buffer);
	slab_free(ipc_call_slab, call);
}

/** Initialize an answerbox structure.
 *
 * @param box  Answerbox structure to be initialized.
 * @param task Task to which the answerbox belongs.
 *
 */
void ipc_answerbox_init(answerbox_t *box, task_t *task)
{
	irq_spinlock_initialize(&box->lock, "ipc.box.lock");
	irq_spinlock_initialize(&box->irq_lock, "ipc.box.irqlock");
	waitq_initialize(&box->wq);
	link_initialize(&box->sync_box_link);
	list_initialize(&box->connected_phones);
	list_initialize(&box->calls);
	list_initialize(&box->dispatched_calls);
	list_initialize(&box->answers);
	list_initialize(&box->irq_notifs);
	list_initialize(&box->irq_list);
	box->task = task;
}

/** Connect a phone to an answerbox.
 *
 * @param phone Initialized phone structure.
 * @param box   Initialized answerbox structure.
 *
 */
void ipc_phone_connect(phone_t *phone, answerbox_t *box)
{
	mutex_lock(&phone->lock);
	
	phone->state = IPC_PHONE_CONNECTED;
	phone->callee = box;
	
	irq_spinlock_lock(&box->lock, true);
	list_append(&phone->link, &box->connected_phones);
	irq_spinlock_unlock(&box->lock, true);
	
	mutex_unlock(&phone->lock);
}

/** Initialize a phone structure.
 *
 * @param phone Phone structure to be initialized.
 *
 */
void ipc_phone_init(phone_t *phone)
{
	mutex_initialize(&phone->lock, MUTEX_PASSIVE);
	phone->callee = NULL;
	phone->state = IPC_PHONE_FREE;
	atomic_set(&phone->active_calls, 0);
}

/** Helper function to facilitate synchronous calls.
 *
 * @param phone   Destination kernel phone structure.
 * @param request Call structure with request.
 *
 * @return EOK on success or EINTR if the sleep was interrupted.
 *
 */
int ipc_call_sync(phone_t *phone, call_t *request)
{
	answerbox_t *sync_box = slab_alloc(ipc_answerbox_slab, 0);
	ipc_answerbox_init(sync_box, TASK);
	
	/*
	 * Put the answerbox on the TASK's list of synchronous answerboxes so
	 * that it can be cleaned up if the call is interrupted.
	 */
	irq_spinlock_lock(&TASK->lock, true);
	list_append(&sync_box->sync_box_link, &TASK->sync_boxes);
	irq_spinlock_unlock(&TASK->lock, true);
	
	/* We will receive data in a special box. */
	request->callerbox = sync_box;
	
	ipc_call(phone, request);
	if (!ipc_wait_for_call(sync_box, SYNCH_NO_TIMEOUT,
	    SYNCH_FLAGS_INTERRUPTIBLE)) {
		/* The answerbox and the call will be freed by ipc_cleanup(). */
		return EINTR;
	}
	
	/*
	 * The answer arrived without interruption so we can remove the
	 * answerbox from the TASK's list of synchronous answerboxes.
	 */
	irq_spinlock_lock(&TASK->lock, true);
	list_remove(&sync_box->sync_box_link);
	irq_spinlock_unlock(&TASK->lock, true);
	
	slab_free(ipc_answerbox_slab, sync_box);
	return EOK;
}

/** Answer a message which was not dispatched and is not listed in any queue.
 *
 * @param call       Call structure to be answered.
 * @param selflocked If true, then TASK->answebox is locked.
 *
 */
static void _ipc_answer_free_call(call_t *call, bool selflocked)
{
	answerbox_t *callerbox = call->callerbox;
	bool do_lock = ((!selflocked) || callerbox != (&TASK->answerbox));
	
	/* Count sent answer */
	irq_spinlock_lock(&TASK->lock, true);
	TASK->ipc_info.answer_sent++;
	irq_spinlock_unlock(&TASK->lock, true);
	
	call->flags |= IPC_CALL_ANSWERED;
	
	if (call->flags & IPC_CALL_FORWARDED) {
		if (call->caller_phone) {
			/* Demasquerade the caller phone. */
			call->data.phone = call->caller_phone;
		}
	}

	call->data.task_id = TASK->taskid;
	
	if (do_lock)
		irq_spinlock_lock(&callerbox->lock, true);
	
	list_append(&call->link, &callerbox->answers);
	
	if (do_lock)
		irq_spinlock_unlock(&callerbox->lock, true);
	
	waitq_wakeup(&callerbox->wq, WAKEUP_FIRST);
}

/** Answer a message which is in a callee queue.
 *
 * @param box  Answerbox that is answering the message.
 * @param call Modified request that is being sent back.
 *
 */
void ipc_answer(answerbox_t *box, call_t *call)
{
	/* Remove from active box */
	irq_spinlock_lock(&box->lock, true);
	list_remove(&call->link);
	irq_spinlock_unlock(&box->lock, true);
	
	/* Send back answer */
	_ipc_answer_free_call(call, false);
}

/** Simulate sending back a message.
 *
 * Most errors are better handled by forming a normal backward
 * message and sending it as a normal answer.
 *
 * @param phone Phone structure the call should appear to come from.
 * @param call  Call structure to be answered.
 * @param err   Return value to be used for the answer.
 *
 */
void ipc_backsend_err(phone_t *phone, call_t *call, sysarg_t err)
{
	call->data.phone = phone;
	atomic_inc(&phone->active_calls);
	IPC_SET_RETVAL(call->data, err);
	_ipc_answer_free_call(call, false);
}

/** Unsafe unchecking version of ipc_call.
 *
 * @param phone Phone structure the call comes from.
 * @param box   Destination answerbox structure.
 * @param call  Call structure with request.
 *
 */
static void _ipc_call(phone_t *phone, answerbox_t *box, call_t *call)
{
	/* Count sent ipc call */
	irq_spinlock_lock(&TASK->lock, true);
	TASK->ipc_info.call_sent++;
	irq_spinlock_unlock(&TASK->lock, true);
	
	if (!(call->flags & IPC_CALL_FORWARDED)) {
		atomic_inc(&phone->active_calls);
		call->data.phone = phone;
		call->data.task_id = TASK->taskid;
	}
	
	irq_spinlock_lock(&box->lock, true);
	list_append(&call->link, &box->calls);
	irq_spinlock_unlock(&box->lock, true);
	
	waitq_wakeup(&box->wq, WAKEUP_FIRST);
}

/** Send an asynchronous request using a phone to an answerbox.
 *
 * @param phone Phone structure the call comes from and which is
 *              connected to the destination answerbox.
 * @param call  Call structure with request.
 *
 * @return Return 0 on success, ENOENT on error.
 *
 */
int ipc_call(phone_t *phone, call_t *call)
{
	mutex_lock(&phone->lock);
	if (phone->state != IPC_PHONE_CONNECTED) {
		mutex_unlock(&phone->lock);
		if (call->flags & IPC_CALL_FORWARDED) {
			IPC_SET_RETVAL(call->data, EFORWARD);
			_ipc_answer_free_call(call, false);
		} else {
			if (phone->state == IPC_PHONE_HUNGUP)
				ipc_backsend_err(phone, call, EHANGUP);
			else
				ipc_backsend_err(phone, call, ENOENT);
		}
		
		return ENOENT;
	}
	
	answerbox_t *box = phone->callee;
	_ipc_call(phone, box, call);
	
	mutex_unlock(&phone->lock);
	return 0;
}

/** Disconnect phone from answerbox.
 *
 * This call leaves the phone in the HUNGUP state. The change to 'free' is done
 * lazily later.
 *
 * @param phone Phone structure to be hung up.
 *
 * @return 0 if the phone is disconnected.
 * @return -1 if the phone was already disconnected.
 *
 */
int ipc_phone_hangup(phone_t *phone)
{
	mutex_lock(&phone->lock);
	if (phone->state == IPC_PHONE_FREE ||
	    phone->state == IPC_PHONE_HUNGUP ||
	    phone->state == IPC_PHONE_CONNECTING) {
		mutex_unlock(&phone->lock);
		return -1;
	}
	
	answerbox_t *box = phone->callee;
	if (phone->state != IPC_PHONE_SLAMMED) {
		/* Remove myself from answerbox */
		irq_spinlock_lock(&box->lock, true);
		list_remove(&phone->link);
		irq_spinlock_unlock(&box->lock, true);
		
		call_t *call = ipc_call_alloc(0);
		IPC_SET_IMETHOD(call->data, IPC_M_PHONE_HUNGUP);
		call->flags |= IPC_CALL_DISCARD_ANSWER;
		_ipc_call(phone, box, call);
	}
	
	phone->state = IPC_PHONE_HUNGUP;
	mutex_unlock(&phone->lock);
	
	return 0;
}

/** Forwards call from one answerbox to another one.
 *
 * @param call     Call structure to be redirected.
 * @param newphone Phone structure to target answerbox.
 * @param oldbox   Old answerbox structure.
 * @param mode     Flags that specify mode of the forward operation.
 *
 * @return 0 if forwarding succeeded or an error code if
 *         there was an error.
 *
 * The return value serves only as an information for the forwarder,
 * the original caller is notified automatically with EFORWARD.
 *
 */
int ipc_forward(call_t *call, phone_t *newphone, answerbox_t *oldbox,
    unsigned int mode)
{
	/* Count forwarded calls */
	irq_spinlock_lock(&TASK->lock, true);
	TASK->ipc_info.forwarded++;
	irq_spinlock_pass(&TASK->lock, &oldbox->lock);
	list_remove(&call->link);
	irq_spinlock_unlock(&oldbox->lock, true);
	
	if (mode & IPC_FF_ROUTE_FROM_ME) {
		if (!call->caller_phone)
			call->caller_phone = call->data.phone;
		call->data.phone = newphone;
		call->data.task_id = TASK->taskid;
	}
	
	return ipc_call(newphone, call);
}


/** Wait for a phone call.
 *
 * @param box   Answerbox expecting the call.
 * @param usec  Timeout in microseconds. See documentation for
 *              waitq_sleep_timeout() for decription of its special
 *              meaning.
 * @param flags Select mode of sleep operation. See documentation for
 *              waitq_sleep_timeout() for description of its special
 *              meaning.
 *
 * @return Recived call structure or NULL.
 *
 * To distinguish between a call and an answer, have a look at call->flags.
 *
 */
call_t *ipc_wait_for_call(answerbox_t *box, uint32_t usec, unsigned int flags)
{
	call_t *request;
	uint64_t irq_cnt = 0;
	uint64_t answer_cnt = 0;
	uint64_t call_cnt = 0;
	int rc;
	
restart:
	rc = waitq_sleep_timeout(&box->wq, usec, flags);
	if (SYNCH_FAILED(rc))
		return NULL;
	
	irq_spinlock_lock(&box->lock, true);
	if (!list_empty(&box->irq_notifs)) {
		/* Count received IRQ notification */
		irq_cnt++;
		
		irq_spinlock_lock(&box->irq_lock, false);
		
		request = list_get_instance(list_first(&box->irq_notifs),
		    call_t, link);
		list_remove(&request->link);
		
		irq_spinlock_unlock(&box->irq_lock, false);
	} else if (!list_empty(&box->answers)) {
		/* Count received answer */
		answer_cnt++;
		
		/* Handle asynchronous answers */
		request = list_get_instance(list_first(&box->answers),
		    call_t, link);
		list_remove(&request->link);
		atomic_dec(&request->data.phone->active_calls);
	} else if (!list_empty(&box->calls)) {
		/* Count received call */
		call_cnt++;
		
		/* Handle requests */
		request = list_get_instance(list_first(&box->calls),
		    call_t, link);
		list_remove(&request->link);
		
		/* Append request to dispatch queue */
		list_append(&request->link, &box->dispatched_calls);
	} else {
		/* This can happen regularly after ipc_cleanup */
		irq_spinlock_unlock(&box->lock, true);
		goto restart;
	}
	
	irq_spinlock_pass(&box->lock, &TASK->lock);
	
	TASK->ipc_info.irq_notif_received += irq_cnt;
	TASK->ipc_info.answer_received += answer_cnt;
	TASK->ipc_info.call_received += call_cnt;
	
	irq_spinlock_unlock(&TASK->lock, true);
	
	return request;
}

/** Answer all calls from list with EHANGUP answer.
 *
 * @param lst Head of the list to be cleaned up.
 *
 */
void ipc_cleanup_call_list(list_t *lst)
{
	while (!list_empty(lst)) {
		call_t *call = list_get_instance(list_first(lst), call_t, link);
		if (call->buffer)
			free(call->buffer);
		
		list_remove(&call->link);
		
		IPC_SET_RETVAL(call->data, EHANGUP);
		_ipc_answer_free_call(call, true);
	}
}

/** Disconnects all phones connected to an answerbox.
 *
 * @param box        Answerbox to disconnect phones from.
 * @param notify_box If true, the answerbox will get a hangup message for
 *                   each disconnected phone.
 *
 */
void ipc_answerbox_slam_phones(answerbox_t *box, bool notify_box)
{
	phone_t *phone;
	DEADLOCK_PROBE_INIT(p_phonelck);
	
	call_t *call = notify_box ? ipc_call_alloc(0) : NULL;
	
	/* Disconnect all phones connected to our answerbox */
restart_phones:
	irq_spinlock_lock(&box->lock, true);
	while (!list_empty(&box->connected_phones)) {
		phone = list_get_instance(list_first(&box->connected_phones),
		    phone_t, link);
		if (SYNCH_FAILED(mutex_trylock(&phone->lock))) {
			irq_spinlock_unlock(&box->lock, true);
			DEADLOCK_PROBE(p_phonelck, DEADLOCK_THRESHOLD);
			goto restart_phones;
		}
		
		/* Disconnect phone */
		ASSERT(phone->state == IPC_PHONE_CONNECTED);
		
		list_remove(&phone->link);
		phone->state = IPC_PHONE_SLAMMED;
		
		if (notify_box) {
			mutex_unlock(&phone->lock);
			irq_spinlock_unlock(&box->lock, true);
			
			/*
			 * Send one message to the answerbox for each
			 * phone. Used to make sure the kbox thread
			 * wakes up after the last phone has been
			 * disconnected.
			 */
			IPC_SET_IMETHOD(call->data, IPC_M_PHONE_HUNGUP);
			call->flags |= IPC_CALL_DISCARD_ANSWER;
			_ipc_call(phone, box, call);
			
			/* Allocate another call in advance */
			call = ipc_call_alloc(0);
			
			/* Must start again */
			goto restart_phones;
		}
		
		mutex_unlock(&phone->lock);
	}
	
	irq_spinlock_unlock(&box->lock, true);
	
	/* Free unused call */
	if (call)
		ipc_call_free(call);
}

/** Clean up all IPC communication of the current task.
 *
 * Note: ipc_hangup sets returning answerbox to TASK->answerbox, you
 * have to change it as well if you want to cleanup other tasks than TASK.
 *
 */
void ipc_cleanup(void)
{
	/* Disconnect all our phones ('ipc_phone_hangup') */
	size_t i;
	for (i = 0; i < IPC_MAX_PHONES; i++)
		ipc_phone_hangup(&TASK->phones[i]);
	
	/* Unsubscribe from any event notifications. */
	event_cleanup_answerbox(&TASK->answerbox);
	
	/* Disconnect all connected irqs */
	ipc_irq_cleanup(&TASK->answerbox);
	
	/* Disconnect all phones connected to our regular answerbox */
	ipc_answerbox_slam_phones(&TASK->answerbox, false);
	
#ifdef CONFIG_UDEBUG
	/* Clean up kbox thread and communications */
	ipc_kbox_cleanup();
#endif
	
	/* Answer all messages in 'calls' and 'dispatched_calls' queues */
	irq_spinlock_lock(&TASK->answerbox.lock, true);
	ipc_cleanup_call_list(&TASK->answerbox.dispatched_calls);
	ipc_cleanup_call_list(&TASK->answerbox.calls);
	irq_spinlock_unlock(&TASK->answerbox.lock, true);
	
	/* Wait for all answers to interrupted synchronous calls to arrive */
	ipl_t ipl = interrupts_disable();
	while (!list_empty(&TASK->sync_boxes)) {
		answerbox_t *box = list_get_instance(
		    list_first(&TASK->sync_boxes), answerbox_t, sync_box_link);
		
		list_remove(&box->sync_box_link);
		call_t *call = ipc_wait_for_call(box, SYNCH_NO_TIMEOUT,
		    SYNCH_FLAGS_NONE);
		ipc_call_free(call);
		slab_free(ipc_answerbox_slab, box);
	}
	interrupts_restore(ipl);
	
	/* Wait for all answers to asynchronous calls to arrive */
	while (true) {
		/*
		 * Go through all phones, until they are all FREE
		 * Locking is not needed, no one else should modify
		 * it when we are in cleanup
		 */
		for (i = 0; i < IPC_MAX_PHONES; i++) {
			if (TASK->phones[i].state == IPC_PHONE_HUNGUP &&
			    atomic_get(&TASK->phones[i].active_calls) == 0) {
				TASK->phones[i].state = IPC_PHONE_FREE;
				TASK->phones[i].callee = NULL;
			}
			
			/*
			 * Just for sure, we might have had some
			 * IPC_PHONE_CONNECTING phones
			 */
			if (TASK->phones[i].state == IPC_PHONE_CONNECTED)
				ipc_phone_hangup(&TASK->phones[i]);
			
			/*
			 * If the hangup succeeded, it has sent a HANGUP
			 * message, the IPC is now in HUNGUP state, we
			 * wait for the reply to come
			 */
			
			if (TASK->phones[i].state != IPC_PHONE_FREE)
				break;
		}
		
		/* Got into cleanup */
		if (i == IPC_MAX_PHONES)
			break;
		
		call_t *call = ipc_wait_for_call(&TASK->answerbox, SYNCH_NO_TIMEOUT,
		    SYNCH_FLAGS_NONE);
		ASSERT((call->flags & IPC_CALL_ANSWERED) ||
		    (call->flags & IPC_CALL_NOTIF));
		
		ipc_call_free(call);
	}
}

/** Initilize IPC subsystem
 *
 */
void ipc_init(void)
{
	ipc_call_slab = slab_cache_create("call_t", sizeof(call_t), 0, NULL,
	    NULL, 0);
	ipc_answerbox_slab = slab_cache_create("answerbox_t",
	    sizeof(answerbox_t), 0, NULL, NULL, 0);
}

/** List answerbox contents.
 *
 * @param taskid Task ID.
 *
 */
void ipc_print_task(task_id_t taskid)
{
	irq_spinlock_lock(&tasks_lock, true);
	task_t *task = task_find_by_id(taskid);
	
	if (!task) {
		irq_spinlock_unlock(&tasks_lock, true);
		return;
	}
	
	/* Hand-over-hand locking */
	irq_spinlock_exchange(&tasks_lock, &task->lock);
	
	printf("[phone id] [calls] [state\n");
	
	size_t i;
	for (i = 0; i < IPC_MAX_PHONES; i++) {
		if (SYNCH_FAILED(mutex_trylock(&task->phones[i].lock))) {
			printf("%-10zu (mutex busy)\n", i);
			continue;
		}
		
		if (task->phones[i].state != IPC_PHONE_FREE) {
			printf("%-10zu %7" PRIun " ", i,
			    atomic_get(&task->phones[i].active_calls));
			
			switch (task->phones[i].state) {
			case IPC_PHONE_CONNECTING:
				printf("connecting");
				break;
			case IPC_PHONE_CONNECTED:
				printf("connected to %" PRIu64 " (%s)",
				    task->phones[i].callee->task->taskid,
				    task->phones[i].callee->task->name);
				break;
			case IPC_PHONE_SLAMMED:
				printf("slammed by %p",
				    task->phones[i].callee);
				break;
			case IPC_PHONE_HUNGUP:
				printf("hung up by %p",
				    task->phones[i].callee);
				break;
			default:
				break;
			}
			
			printf("\n");
		}
		
		mutex_unlock(&task->phones[i].lock);
	}
	
	irq_spinlock_lock(&task->answerbox.lock, false);
	
#ifdef __32_BITS__
	printf("[call id ] [method] [arg1] [arg2] [arg3] [arg4] [arg5]"
	    " [flags] [sender\n");
#endif
	
#ifdef __64_BITS__
	printf("[call id         ] [method] [arg1] [arg2] [arg3] [arg4]"
	    " [arg5] [flags] [sender\n");
#endif
	
	printf(" --- incomming calls ---\n");
	list_foreach(task->answerbox.calls, cur) {
		call_t *call = list_get_instance(cur, call_t, link);
		
#ifdef __32_BITS__
		printf("%10p ", call);
#endif
		
#ifdef __64_BITS__
		printf("%18p ", call);
#endif
		
		printf("%-8" PRIun " %-6" PRIun " %-6" PRIun " %-6" PRIun
		    " %-6" PRIun " %-6" PRIun " %-7x %" PRIu64 " (%s)\n",
		    IPC_GET_IMETHOD(call->data), IPC_GET_ARG1(call->data),
		    IPC_GET_ARG2(call->data), IPC_GET_ARG3(call->data),
		    IPC_GET_ARG4(call->data), IPC_GET_ARG5(call->data),
		    call->flags, call->sender->taskid, call->sender->name);
	}
	
	printf(" --- dispatched calls ---\n");
	list_foreach(task->answerbox.dispatched_calls, cur) {
		call_t *call = list_get_instance(cur, call_t, link);
		
#ifdef __32_BITS__
		printf("%10p ", call);
#endif
		
#ifdef __64_BITS__
		printf("%18p ", call);
#endif
		
		printf("%-8" PRIun " %-6" PRIun " %-6" PRIun " %-6" PRIun
		    " %-6" PRIun " %-6" PRIun " %-7x %" PRIu64 " (%s)\n",
		    IPC_GET_IMETHOD(call->data), IPC_GET_ARG1(call->data),
		    IPC_GET_ARG2(call->data), IPC_GET_ARG3(call->data),
		    IPC_GET_ARG4(call->data), IPC_GET_ARG5(call->data),
		    call->flags, call->sender->taskid, call->sender->name);
	}
	
	printf(" --- incoming answers ---\n");
	list_foreach(task->answerbox.answers, cur) {
		call_t *call = list_get_instance(cur, call_t, link);
		
#ifdef __32_BITS__
		printf("%10p ", call);
#endif
		
#ifdef __64_BITS__
		printf("%18p ", call);
#endif
		
		printf("%-8" PRIun " %-6" PRIun " %-6" PRIun " %-6" PRIun
		    " %-6" PRIun " %-6" PRIun " %-7x %" PRIu64 " (%s)\n",
		    IPC_GET_IMETHOD(call->data), IPC_GET_ARG1(call->data),
		    IPC_GET_ARG2(call->data), IPC_GET_ARG3(call->data),
		    IPC_GET_ARG4(call->data), IPC_GET_ARG5(call->data),
		    call->flags, call->sender->taskid, call->sender->name);
	}
	
	irq_spinlock_unlock(&task->answerbox.lock, false);
	irq_spinlock_unlock(&task->lock, true);
}

/** @}
 */
