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

/* Length of data being transfered with IPC call */
/* - the uspace may not be able to utilize full length */
#define IPC_CALL_LEN    4

/** Maximum active async calls per thread */
#define IPC_MAX_ASYNC_CALLS  4

/* Flags for calls */
#define IPC_CALL_ANSWERED      1 /**< This is answer to a call */
#define IPC_CALL_STATIC_ALLOC  2 /**< This call will not be freed on error */

/* Flags for ipc_wait_for_call */
#define IPC_WAIT_NONBLOCKING   1

/* Flags of callid */
#define IPC_CALLID_ANSWERED  1

/* Return values from IPC_ASYNC */
#define IPC_CALLRET_FATAL         -1
#define IPC_CALLRET_TEMPORARY     -2


/* Macros for manipulating calling data */
#define IPC_SET_RETVAL(data, retval)   ((data)[0] = (retval))
#define IPC_SET_METHOD(data, val)   ((data)[0] = (val))
#define IPC_SET_ARG1(data, val)   ((data)[1] = (val))
#define IPC_SET_ARG2(data, val)   ((data)[2] = (val))
#define IPC_SET_ARG3(data, val)   ((data)[3] = (val))

#define IPC_GET_METHOD(data)           ((data)[0])
#define IPC_GET_RETVAL(data)           ((data)[0])

#define IPC_GET_ARG1(data)              ((data)[1])
#define IPC_GET_ARG2(data)              ((data)[2])
#define IPC_GET_ARG3(data)              ((data)[3])

/* Well known phone descriptors */
#define PHONE_NS              0

/* System-specific methods - only through special syscalls
 * These methods have special behaviour
 */
/** Protocol for CONNECT - TO - ME 
 *
 * Calling process asks the callee to create a callback connection,
 * so that it can start initiating new messages.
 *
 * The protocol for negotiating is:
 * - sys_connecttome - sends a message IPC_M_CONNECTTOME
 * - sys_wait_for_call - upon receipt tries to allocate new phone
 *                       - if it fails, responds with ELIMIT
 *                     - passes call to userspace. If userspace
 *                       responds with error, phone is deallocated and
 *                       error is sent back to caller. Otherwise 
 *                       the call is accepted and the response is sent back.
 *                     - the allocated phoneid is passed to userspace 
 *                       (on the receiving sid) as ARG3 of the call.
 *                     - the caller obtains taskid of the called thread
 */
#define IPC_M_CONNECTTOME     1
/** Protocol for CONNECT - ME - TO
 *
 * Calling process asks the callee to create for him a new connection.
 * E.g. the caller wants a name server to connect him to print server.
 *
 * The protocol for negotiating is:
 * - sys_connect_me_to - send a synchronous message to name server
 *                       indicating that it wants to be connected to some
 *                       service
 *   recepient         -  if ipc_answer == 0, then accept connection
 *                     -  otherwise connection refused
 *                     -  recepient may forward message. Forwarding
 *                        system message 
 *
 */
#define IPC_M_CONNECTMETO     2
/* Control messages that the server sends to the processes 
 * about their connections.
 */


/* Well-known methods */
#define IPC_M_LAST_SYSTEM     511
#define IPC_M_PING            512
/* User methods */
#define FIRST_USER_METHOD     1024

#ifdef KERNEL

#include <synch/mutex.h>
#include <synch/condvar.h>
#include <adt/list.h>

#define IPC_MAX_PHONES  16

typedef struct answerbox answerbox_t;
typedef __native ipc_data_t[IPC_CALL_LEN];

typedef struct {
	link_t list;
	answerbox_t *callerbox;
	int flags;
	task_t *sender;
	ipc_data_t data;
} call_t;

struct answerbox {
	SPINLOCK_DECLARE(lock);

	task_t *task;

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
	int busy;
} phone_t;

extern void ipc_init(void);
extern call_t * ipc_wait_for_call(answerbox_t *box, int flags);
extern void ipc_answer(answerbox_t *box, call_t *request);
extern void ipc_call(phone_t *phone, call_t *request);
extern void ipc_call_sync(phone_t *phone, call_t *request);
extern void ipc_phone_destroy(phone_t *phone);
extern void ipc_phone_init(phone_t *phone);
extern void ipc_phone_connect(phone_t *phone, answerbox_t *box);
extern void ipc_call_free(call_t *call);
extern call_t * ipc_call_alloc(void);
extern void ipc_answerbox_init(answerbox_t *box);
extern void ipc_call_init(call_t *call);
extern void ipc_forward(call_t *call, answerbox_t *newbox,answerbox_t *oldbox);

extern answerbox_t *ipc_phone_0;

#endif

#endif
