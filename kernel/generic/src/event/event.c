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
 * @brief	Kernel event notifications.
 */

#include <event/event.h>
#include <event/event_types.h>
#include <mm/slab.h>
#include <arch/types.h>
#include <synch/spinlock.h>
#include <console/console.h>
#include <memstr.h>
#include <errno.h>
#include <arch.h>

/**
 * The events array.
 * Arranging the events in this two-dimensional array should decrease the
 * likelyhood of cacheline ping-pong.
 */
static event_t *events[EVENT_END];

/** Initialize kernel events. */
void event_init(void)
{
	int i;

	for (i = 0; i < EVENT_END; i++) {
		events[i] = (event_t *) malloc(sizeof(event_t), 0);
		spinlock_initialize(&events[i]->lock, "event.lock");
		events[i]->answerbox = NULL;
		events[i]->counter = 0;
		events[i]->method = 0;
	}
}

static int
event_subscribe(event_type_t e, unative_t method, answerbox_t *answerbox)
{
	if (e >= EVENT_END)
		return ELIMIT;

	int res = EEXISTS;
	event_t *event = events[e];

	spinlock_lock(&event->lock);
	if (!event->answerbox) {
		event->answerbox = answerbox;
		event->method = method;
		event->counter = 0;
		res = EOK;
	}
	spinlock_unlock(&event->lock);

	return res;
}

unative_t sys_event_subscribe(unative_t evno, unative_t method)
{
	return (unative_t) event_subscribe((event_type_t) evno, (unative_t)
	    method, &TASK->answerbox);
}

bool event_is_subscribed(event_type_t e)
{
	bool res;

	ASSERT(e < EVENT_END);
	spinlock_lock(&events[e]->lock);
	res = events[e]->answerbox != NULL;
	spinlock_unlock(&events[e]->lock);

	return res;
}


void event_cleanup_answerbox(answerbox_t *answerbox)
{
	int i;

	for (i = 0; i < EVENT_END; i++) {
		spinlock_lock(&events[i]->lock);
		if (events[i]->answerbox == answerbox) {
			events[i]->answerbox = NULL;
			events[i]->counter = 0;
			events[i]->method = 0;
		}
		spinlock_unlock(&events[i]->lock);
	}
}

void
event_notify(event_type_t e, unative_t a1, unative_t a2, unative_t a3,
    unative_t a4, unative_t a5)
{
	ASSERT(e < EVENT_END);
	event_t *event = events[e];
	spinlock_lock(&event->lock);
	if (event->answerbox) {
		call_t *call = ipc_call_alloc(FRAME_ATOMIC);
		if (call) {
			call->flags |= IPC_CALL_NOTIF;
			call->priv = ++event->counter;
			IPC_SET_METHOD(call->data, event->method);
			IPC_SET_ARG1(call->data, a1);
			IPC_SET_ARG2(call->data, a2);
			IPC_SET_ARG3(call->data, a3);
			IPC_SET_ARG4(call->data, a4);
			IPC_SET_ARG5(call->data, a5);

			spinlock_lock(&event->answerbox->irq_lock);
			list_append(&call->link, &event->answerbox->irq_notifs);
			spinlock_unlock(&event->answerbox->irq_lock);

			waitq_wakeup(&event->answerbox->wq, WAKEUP_FIRST);
		}
	}
	spinlock_unlock(&event->lock);
}

/** @}
 */
