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

#include <ipc/event.h>
#include <ipc/event_types.h>
#include <mm/slab.h>
#include <typedefs.h>
#include <synch/spinlock.h>
#include <console/console.h>
#include <memstr.h>
#include <errno.h>
#include <arch.h>

/** The events array. */
static event_t events[EVENT_END];

/** Initialize kernel events.
 *
 */
void event_init(void)
{
	for (unsigned int i = 0; i < EVENT_END; i++) {
		spinlock_initialize(&events[i].lock, "event.lock");
		events[i].answerbox = NULL;
		events[i].counter = 0;
		events[i].imethod = 0;
		events[i].masked = false;
		events[i].unmask_callback = NULL;
	}
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

/** Define a callback function for the event unmask event.
 *
 * @param evno     Event type.
 * @param callback Callback function to be called when
 *                 the event is unmasked.
 *
 */
void event_set_unmask_callback(event_type_t evno, event_callback_t callback)
{
	ASSERT(evno < EVENT_END);
	
	spinlock_lock(&events[evno].lock);
	events[evno].unmask_callback = callback;
	spinlock_unlock(&events[evno].lock);
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
int event_notify(event_type_t evno, bool mask, sysarg_t a1, sysarg_t a2,
    sysarg_t a3, sysarg_t a4, sysarg_t a5)
{
	ASSERT(evno < EVENT_END);
	
	spinlock_lock(&events[evno].lock);
	
	int ret;
	
	if (events[evno].answerbox != NULL) {
		if (!events[evno].masked) {
			call_t *call = ipc_call_alloc(FRAME_ATOMIC);
			
			if (call) {
				call->flags |= IPC_CALL_NOTIF;
				call->priv = ++events[evno].counter;
				
				IPC_SET_IMETHOD(call->data, events[evno].imethod);
				IPC_SET_ARG1(call->data, a1);
				IPC_SET_ARG2(call->data, a2);
				IPC_SET_ARG3(call->data, a3);
				IPC_SET_ARG4(call->data, a4);
				IPC_SET_ARG5(call->data, a5);
				
				irq_spinlock_lock(&events[evno].answerbox->irq_lock, true);
				list_append(&call->link, &events[evno].answerbox->irq_notifs);
				irq_spinlock_unlock(&events[evno].answerbox->irq_lock, true);
				
				waitq_wakeup(&events[evno].answerbox->wq, WAKEUP_FIRST);
				
				if (mask)
					events[evno].masked = true;
				
				ret = EOK;
			} else
				ret = ENOMEM;
		} else
			ret = EBUSY;
	} else
		ret = ENOENT;
	
	spinlock_unlock(&events[evno].lock);
	
	return ret;
}

/** Subscribe event notifications
 *
 * @param evno      Event type.
 * @param imethod   IPC interface and method to be used for
 *                  the notifications.
 * @param answerbox Answerbox to send the notifications to.
 *
 * @return EOK if the subscription was successful.
 * @return EEXISTS if the notifications of the given type are
 *         already subscribed.
 *
 */
static int event_subscribe(event_type_t evno, sysarg_t imethod,
    answerbox_t *answerbox)
{
	ASSERT(evno < EVENT_END);
	
	spinlock_lock(&events[evno].lock);
	
	int res;
	
	if (events[evno].answerbox == NULL) {
		events[evno].answerbox = answerbox;
		events[evno].imethod = imethod;
		events[evno].counter = 0;
		events[evno].masked = false;
		res = EOK;
	} else
		res = EEXISTS;
	
	spinlock_unlock(&events[evno].lock);
	
	return res;
}

/** Unmask event notifications
 *
 * @param evno Event type to unmask.
 *
 */
static void event_unmask(event_type_t evno)
{
	ASSERT(evno < EVENT_END);
	
	spinlock_lock(&events[evno].lock);
	events[evno].masked = false;
	event_callback_t callback = events[evno].unmask_callback;
	spinlock_unlock(&events[evno].lock);
	
	/*
	 * Check if there is an unmask callback
	 * function defined for this event.
	 */
	if (callback != NULL)
		callback();
}

/** Event notification syscall wrapper
 *
 * @param evno    Event type to subscribe.
 * @param imethod IPC interface and method to be used for
 *                the notifications.
 *
 * @return EOK on success.
 * @return ELIMIT on unknown event type.
 * @return EEXISTS if the notifications of the given type are
 *         already subscribed.
 *
 */
sysarg_t sys_event_subscribe(sysarg_t evno, sysarg_t imethod)
{
	if (evno >= EVENT_END)
		return ELIMIT;
	
	return (sysarg_t) event_subscribe((event_type_t) evno, (sysarg_t)
	    imethod, &TASK->answerbox);
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
sysarg_t sys_event_unmask(sysarg_t evno)
{
	if (evno >= EVENT_END)
		return ELIMIT;
	
	event_unmask((event_type_t) evno);
	return EOK;
}

/** @}
 */
