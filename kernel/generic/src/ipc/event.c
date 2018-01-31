/*
 * Copyright (c) 2009 Jakub Jermar
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

/** @addtogroup generic
 * @{
 */
/**
 * @file
 * @brief Kernel event notifications.
 */

#include <assert.h>
#include <ipc/event.h>
#include <mm/slab.h>
#include <typedefs.h>
#include <synch/spinlock.h>
#include <console/console.h>
#include <proc/task.h>
#include <errno.h>
#include <arch.h>

/** The events array. */
static event_t events[EVENT_END];

static void event_initialize(event_t *event)
{
	spinlock_initialize(&event->lock, "event.lock");
	event->answerbox = NULL;
	event->counter = 0;
	event->imethod = 0;
	event->masked = false;
	event->unmask_callback = NULL;
}

static event_t *evno2event(int evno, task_t *task)
{
	assert(evno < EVENT_TASK_END);
	
	event_t *event;
	
	if (evno < EVENT_END)
		event = &events[(event_type_t) evno];
	else
		event = &task->events[(event_task_type_t) evno - EVENT_END];
	
	return event;
}

/** Initialize kernel events.
 *
 */
void event_init(void)
{
	for (unsigned int i = 0; i < EVENT_END; i++)
		event_initialize(evno2event(i, NULL));
}

void event_task_init(task_t *task)
{
	for (unsigned int i = EVENT_END; i < EVENT_TASK_END; i++)
		event_initialize(evno2event(i, task));
}

/** Unsubscribe kernel events associated with an answerbox
 *
 * @param answerbox Answerbox to be unsubscribed.
 *
 */
void event_cleanup_answerbox(answerbox_t *answerbox)
{
	for (unsigned int i = 0; i < EVENT_END; i++) {
		spinlock_lock(&events[i].lock);
		
		if (events[i].answerbox == answerbox) {
			events[i].answerbox = NULL;
			events[i].counter = 0;
			events[i].imethod = 0;
			events[i].masked = false;
		}
		
		spinlock_unlock(&events[i].lock);
	}
}

static void _event_set_unmask_callback(event_t *event, event_callback_t callback)
{
	spinlock_lock(&event->lock);
	event->unmask_callback = callback;
	spinlock_unlock(&event->lock);
}

/** Define a callback function for the event unmask event.
 *
 * @param evno     Event type.
 * @param callback Callback function to be called when
 *                 the event is unmasked.
 *
 */
void event_set_unmask_callback(event_type_t evno, event_callback_t callback)
{
	assert(evno < EVENT_END);
	
	_event_set_unmask_callback(evno2event(evno, NULL), callback);
}

void event_task_set_unmask_callback(task_t *task, event_task_type_t evno,
    event_callback_t callback)
{
	assert(evno >= (int) EVENT_END);
	assert(evno < EVENT_TASK_END);
		
	_event_set_unmask_callback(evno2event(evno, task), callback);
}

static errno_t event_enqueue(event_t *event, bool mask, sysarg_t a1, sysarg_t a2,
    sysarg_t a3, sysarg_t a4, sysarg_t a5)
{
	errno_t res;

	spinlock_lock(&event->lock);
	
	if (event->answerbox != NULL) {
		if (!event->masked) {
			call_t *call = ipc_call_alloc(FRAME_ATOMIC);
			
			if (call) {
				call->flags |= IPC_CALL_NOTIF;
				call->priv = ++event->counter;
				
				IPC_SET_IMETHOD(call->data, event->imethod);
				IPC_SET_ARG1(call->data, a1);
				IPC_SET_ARG2(call->data, a2);
				IPC_SET_ARG3(call->data, a3);
				IPC_SET_ARG4(call->data, a4);
				IPC_SET_ARG5(call->data, a5);
				
				call->data.task_id = TASK ? TASK->taskid : 0;
				
				irq_spinlock_lock(&event->answerbox->irq_lock,
				    true);
				list_append(&call->ab_link,
				    &event->answerbox->irq_notifs);
				irq_spinlock_unlock(&event->answerbox->irq_lock,
				    true);
				
				waitq_wakeup(&event->answerbox->wq,
				    WAKEUP_FIRST);
				
				if (mask)
					event->masked = true;
				
				res = EOK;
			} else
				res = ENOMEM;
		} else
			res = EBUSY;
	} else
		res = ENOENT;
	
	spinlock_unlock(&event->lock);
	return res;
}

/** Send kernel notification event
 *
 * @param evno Event type.
 * @param mask Mask further notifications after a successful
 *             sending.
 * @param a1   First argument.
 * @param a2   Second argument.
 * @param a3   Third argument.
 * @param a4   Fourth argument.
 * @param a5   Fifth argument.
 *
 * @return EOK if notification was successfully sent.
 * @return ENOMEM if the notification IPC message failed to allocate.
 * @return EBUSY if the notifications of the given type are
 *         currently masked.
 * @return ENOENT if the notifications of the given type are
 *         currently not subscribed.
 *
 */
errno_t event_notify(event_type_t evno, bool mask, sysarg_t a1, sysarg_t a2,
    sysarg_t a3, sysarg_t a4, sysarg_t a5)
{
	assert(evno < EVENT_END);
	
	return event_enqueue(evno2event(evno, NULL), mask, a1, a2, a3, a4, a5);
}

/** Send per-task kernel notification event
 *
 * @param task Destination task.
 * @param evno Event type.
 * @param mask Mask further notifications after a successful
 *             sending.
 * @param a1   First argument.
 * @param a2   Second argument.
 * @param a3   Third argument.
 * @param a4   Fourth argument.
 * @param a5   Fifth argument.
 *
 * @return EOK if notification was successfully sent.
 * @return ENOMEM if the notification IPC message failed to allocate.
 * @return EBUSY if the notifications of the given type are
 *         currently masked.
 * @return ENOENT if the notifications of the given type are
 *         currently not subscribed.
 *
 */
errno_t event_task_notify(task_t *task, event_task_type_t evno, bool mask,
    sysarg_t a1, sysarg_t a2, sysarg_t a3, sysarg_t a4, sysarg_t a5)
{
	assert(evno >= (int) EVENT_END);
	assert(evno < EVENT_TASK_END);
	
	return event_enqueue(evno2event(evno, task), mask, a1, a2, a3, a4, a5);
}

/** Subscribe event notifications
 *
 * @param evno      Event type.
 * @param imethod   IPC interface and method to be used for
 *                  the notifications.
 * @param answerbox Answerbox to send the notifications to.
 *
 * @return EOK if the subscription was successful.
 * @return EEXIST if the notifications of the given type are
 *         already subscribed.
 *
 */
static errno_t event_subscribe(event_t *event, sysarg_t imethod,
    answerbox_t *answerbox)
{
	errno_t res;
	
	spinlock_lock(&event->lock);
	
	if (event->answerbox == NULL) {
		event->answerbox = answerbox;
		event->imethod = imethod;
		event->counter = 0;
		event->masked = false;
		res = EOK;
	} else
		res = EEXIST;
	
	spinlock_unlock(&event->lock);
	
	return res;
}

/** Unsubscribe event notifications
 *
 * @param evno      Event type.
 * @param answerbox Answerbox used to send the notifications to.
 *
 * @return EOK if the subscription was successful.
 * @return EEXIST if the notifications of the given type are
 *         already subscribed.
 *
 */
static errno_t event_unsubscribe(event_t *event, answerbox_t *answerbox)
{
	errno_t res;
	
	spinlock_lock(&event->lock);
	
	if (event->answerbox == answerbox) {
		event->answerbox = NULL;
		event->counter = 0;
		event->imethod = 0;
		event->masked = false;
		res = EOK;
	} else
		res = ENOENT;
	
	spinlock_unlock(&event->lock);
	
	return res;
}

/** Unmask event notifications
 *
 * @param evno Event type to unmask.
 *
 */
static void event_unmask(event_t *event)
{
	spinlock_lock(&event->lock);
	event->masked = false;
	event_callback_t callback = event->unmask_callback;
	spinlock_unlock(&event->lock);
	
	/*
	 * Check if there is an unmask callback
	 * function defined for this event.
	 */
	if (callback != NULL)
		callback(event);
}

/** Event notification subscription syscall wrapper
 *
 * @param evno    Event type to subscribe.
 * @param imethod IPC interface and method to be used for
 *                the notifications.
 *
 * @return EOK on success.
 * @return ELIMIT on unknown event type.
 * @return EEXIST if the notifications of the given type are
 *         already subscribed.
 *
 */
sys_errno_t sys_ipc_event_subscribe(sysarg_t evno, sysarg_t imethod)
{
	if (evno >= EVENT_TASK_END)
		return ELIMIT;
	
	return (sys_errno_t) event_subscribe(evno2event(evno, TASK),
	    (sysarg_t) imethod, &TASK->answerbox);
}

/** Event notification unsubscription syscall wrapper
 *
 * @param evno    Event type to unsubscribe.
 *
 * @return EOK on success.
 * @return ELIMIT on unknown event type.
 * @return ENOENT if the notification of the given type is not
           subscribed.
 *
 */
sys_errno_t sys_ipc_event_unsubscribe(sysarg_t evno)
{
	if (evno >= EVENT_TASK_END)
		return ELIMIT;
	
	return (sys_errno_t) event_unsubscribe(evno2event(evno, TASK),
	    &TASK->answerbox);
}

/** Event notification unmask syscall wrapper
 *
 * Note that currently no tests are performed whether the calling
 * task is entitled to unmask the notifications. However, thanks
 * to the fact that notification masking is only a performance
 * optimization, this has probably no security implications.
 *
 * @param evno Event type to unmask.
 *
 * @return EOK on success.
 * @return ELIMIT on unknown event type.
 *
 */
sys_errno_t sys_ipc_event_unmask(sysarg_t evno)
{
	if (evno >= EVENT_TASK_END)
		return ELIMIT;
	
	event_unmask(evno2event(evno, TASK));

	return EOK;
}

/** @}
 */
