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

#ifndef __IPC_H__
#define __IPC_H__

#define IPC_CALL_LEN    2

/* Flags for calls */
#define IPC_CALL_ANSWERED    1
/* Flags for ipc_wait_for_call */
#define IPC_WAIT_NONBLOCKING 1

#ifdef KERNEL

#include <synch/waitq.h>
#include <synch/spinlock.h>
#include <adt/list.h>

#define IPC_MAX_PHONES  16


typedef struct answerbox answerbox_t;

typedef struct {
	link_t list;
	answerbox_t *callerbox;
	int flags;
	__native data[IPC_CALL_LEN];
} call_t;

struct answerbox {
	SPINLOCK_DECLARE(lock);

	waitq_t wq;

	link_t connected_phones; /**< Phones connected to this answerbox */
	link_t calls;            /**< Received calls */
	link_t dispatched_calls; /* Should be hash table in the future */

	link_t answers;          /**< Answered calls */
};

typedef struct {
	SPINLOCK_DECLARE(lock);
	link_t list;
	answerbox_t *callee;
} phone_t;

extern void ipc_create_phonecompany(void);
extern void ipc_init(void);
extern call_t * ipc_wait_for_call(answerbox_t *box, int flags);
extern void ipc_answer(answerbox_t *box, call_t *request);
extern void ipc_call(phone_t *phone, call_t *request);
extern void ipc_phone_destroy(phone_t *phone);
extern void ipc_phone_init(phone_t *phone, answerbox_t *box);
extern void ipc_call_free(call_t *call);
extern call_t * ipc_call_alloc(void);
void ipc_answerbox_init(answerbox_t *box);
void ipc_phone_init(phone_t *phone, answerbox_t *box);

extern answerbox_t *ipc_central_box;

#endif

#endif
