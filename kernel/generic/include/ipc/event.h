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
/** @file
 */

#ifndef KERN_EVENT_H_
#define KERN_EVENT_H_

#include <ipc/event_types.h>
#include <arch/types.h>
#include <synch/spinlock.h>
#include <ipc/ipc.h>

/** Event notification structure. */
typedef struct {
	SPINLOCK_DECLARE(lock);
	
	/** Answerbox for notifications. */
	answerbox_t *answerbox;
	/** Method to be used for the notification. */
	unative_t method;
	/** Counter. */
	size_t counter;
} event_t;

extern void event_init(void);
extern unative_t sys_event_subscribe(unative_t, unative_t);
extern bool event_is_subscribed(event_type_t);
extern void event_cleanup_answerbox(answerbox_t *);

#define event_notify_0(e) \
	event_notify((e), 0, 0, 0, 0, 0)
#define event_notify_1(e, a1) \
	event_notify((e), (a1), 0, 0, 0, 0)
#define event_notify_2(e, a1, a2) \
	event_notify((e), (a1), (a2), 0, 0, 0)
#define event_notify_3(e, a1, a2, a3) \
	event_notify((e), (a1), (a2), (a3), 0, 0)
#define event_notify_4(e, a1, a2, a3, a4) \
	event_notify((e), (a1), (a2), (a3), (a4), 0)
#define event_notify_5(e, a1, a2, a3, a4, a5) \
	event_notify((e), (a1), (a2), (a3), (a4), (a5))

extern void event_notify(event_type_t, unative_t, unative_t, unative_t,
    unative_t, unative_t);

#endif

/** @}
 */
