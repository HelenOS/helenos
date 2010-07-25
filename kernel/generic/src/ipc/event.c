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

/** Initialize kernel events. */
void event_init(void)
{
	unsigned int i;
	
	for (i = 0; i < EVENT_END; i++) {
		spinlock_initialize(&events[i].lock, "event.lock");
		events[i].answerbox = NULL;
		events[i].counter = 0;
		events[i].method = 0;
	}
}

static int event_subscribe(event_type_t evno, unative_t method,
    answerbox_t *answerbox)
{
	if (evno >= EVENT_END)
		return ELIMIT;
	
	spinlock_lock(&events[evno].lock);
	
	int res;
	
	if (events[evno].answerbox == NULL) {
		events[evno].answerbox = answerbox;
		events[evno].method = method;
		events[evno].counter = 0;
		res = EOK;
	} else
		res = EEXISTS;
	
	spinlock_unlock(&events[evno].lock);
	
	return res;
}

unative_t sys_event_subscribe(unative_t evno, unative_t method)
{
	return (unative_t) event_subscribe((event_type_t) evno, (unative_t)
	    method, &TASK->answerbox);
}

bool event_is_subscribed(event_type_t evno)
{
	bool res;
	
	ASSERT(evno < EVENT_END);
	
	spinlock_lock(&events[evno].lock);
	res = events[evno].answerbox != NULL;
	spinlock_unlock(&events[evno].lock);
	
	return res;
}


void event_cleanup_answerbox(answerbox_t *answerbox)
{
	unsigned int i;
	
	for (i = 0; i < EVENT_END; i++) {
		spinlock_lock(&events[i].lock);
		if (events[i].answerbox == answerbox) {
			events[i].answerbox = NULL;
			events[i].counter = 0;
			events[i].method = 0;
		}
		spinlock_unlock(&events[i].lock);
	}
}

void event_notify(event_type_t evno, unative_t a1, unative_t a2, unative_t a3,
    unative_t a4, unative_t a5)
{
	ASSERT(evno < EVENT_END);
	
	spinlock_lock(&events[evno].lock);
	if (events[evno].answerbox != NULL) {
		call_t *call = ipc_call_alloc(FRAME_ATOMIC);
		if (call) {
			call->flags |= IPC_CALL_NOTIF;
			call->priv = ++events[evno].counter;
			IPC_SET_METHOD(call->data, events[evno].method);
			IPC_SET_ARG1(call->data, a1);
			IPC_SET_ARG2(call->data, a2);
			IPC_SET_ARG3(call->data, a3);
			IPC_SET_ARG4(call->data, a4);
			IPC_SET_ARG5(call->data, a5);
			
			irq_spinlock_lock(&events[evno].answerbox->irq_lock, true);
			list_append(&call->link, &events[evno].answerbox->irq_notifs);
			irq_spinlock_unlock(&events[evno].answerbox->irq_lock, true);
			
			waitq_wakeup(&events[evno].answerbox->wq, WAKEUP_FIRST);
		}
	}
	spinlock_unlock(&events[evno].lock);
}

/** @}
 */
