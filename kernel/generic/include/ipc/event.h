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

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_EVENT_H_
#define KERN_EVENT_H_

#include <abi/ipc/event.h>
#include <typedefs.h>
#include <synch/spinlock.h>
#include <ipc/ipc.h>

struct task;

typedef void (*event_callback_t)(void *);

/** Event notification structure. */
typedef struct {
	SPINLOCK_DECLARE(lock);

	/** Answerbox for notifications. */
	answerbox_t *answerbox;
	/** Interface and method to be used for the notification. */
	sysarg_t imethod;
	/** Counter. */
	size_t counter;

	/** Masked flag. */
	bool masked;
	/** Unmask callback. */
	event_callback_t unmask_callback;
} event_t;

extern void event_init(void);
extern void event_task_init(struct task *);
extern void event_cleanup_answerbox(answerbox_t *);
extern void event_set_unmask_callback(event_type_t, event_callback_t);
extern void event_task_set_unmask_callback(struct task *, event_task_type_t,
    event_callback_t);

#define event_notify_0(e, m) \
	event_notify((e), (m), 0, 0, 0, 0, 0)
#define event_notify_1(e, m, a1) \
	event_notify((e), (m), (a1), 0, 0, 0, 0)
#define event_notify_2(e, m, a1, a2) \
	event_notify((e), (m), (a1), (a2), 0, 0, 0)
#define event_notify_3(e, m, a1, a2, a3) \
	event_notify((e), (m), (a1), (a2), (a3), 0, 0)
#define event_notify_4(e, m, a1, a2, a3, a4) \
	event_notify((e), (m), (a1), (a2), (a3), (a4), 0)
#define event_notify_5(e, m, a1, a2, a3, a4, a5) \
	event_notify((e), (m), (a1), (a2), (a3), (a4), (a5))

#define event_task_notify_0(t, e, m) \
	event_task_notify((t), (e), (m), 0, 0, 0, 0, 0)
#define event_task_notify_1(t, e, m, a1) \
	event_task_notify((t), (e), (m), (a1), 0, 0, 0, 0)
#define event_task_notify_2(t, e, m, a1, a2) \
	event_task_notify((t), (e), (m), (a1), (a2), 0, 0, 0)
#define event_task_notify_3(t, e, m, a1, a2, a3) \
	event_task_notify((t), (e), (m), (a1), (a2), (a3), 0, 0)
#define event_task_notify_4(t, e, m, a1, a2, a3, a4) \
	event_task_notify((t), (e), (m), (a1), (a2), (a3), (a4), 0)
#define event_task_notify_5(t, e, m, a1, a2, a3, a4, a5) \
	event_task_notify((t), (e), (m), (a1), (a2), (a3), (a4), (a5))

extern errno_t event_notify(event_type_t, bool, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t);
extern errno_t event_task_notify(struct task *, event_task_type_t, bool, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t, sysarg_t);

extern sys_errno_t sys_ipc_event_subscribe(sysarg_t, sysarg_t);
extern sys_errno_t sys_ipc_event_unsubscribe(sysarg_t);
extern sys_errno_t sys_ipc_event_unmask(sysarg_t);

#endif

/** @}
 */
