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

#ifndef KERN_IPC_H_
#define KERN_IPC_H_

/* Length of data being transfered with IPC call */
/* - the uspace may not be able to utilize full length */
#define IPC_CALL_LEN    4

/** Maximum active async calls per thread */
#ifdef CONFIG_DEBUG
# define IPC_MAX_ASYNC_CALLS  4
#else
# define IPC_MAX_ASYNC_CALLS  4000
#endif

/* Flags for calls */
#define IPC_CALL_ANSWERED       (1<<0) /**< This is answer to a call */
#define IPC_CALL_STATIC_ALLOC   (1<<1) /**< This call will not be freed on error */
#define IPC_CALL_DISCARD_ANSWER (1<<2) /**< Answer will not be passed to userspace, will be discarded */
#define IPC_CALL_FORWARDED      (1<<3) /**< Call was forwarded */
#define IPC_CALL_CONN_ME_TO     (1<<4) /**< Identify connect_me_to answer */
#define IPC_CALL_NOTIF          (1<<5) /**< Interrupt notification */

/* Flags of callid (the addresses are aligned at least to 4, 
 * that is why we can use bottom 2 bits of the call address
 */
#define IPC_CALLID_ANSWERED       1 /**< Type of this msg is 'answer' */
#define IPC_CALLID_NOTIFICATION   2 /**< Type of this msg is 'notification' */

/* Return values from IPC_ASYNC */
#define IPC_CALLRET_FATAL         -1
#define IPC_CALLRET_TEMPORARY     -2


/* Macros for manipulating calling data */
#define IPC_SET_RETVAL(data, retval)   ((data).args[0] = (retval))
#define IPC_SET_METHOD(data, val)   ((data).args[0] = (val))
#define IPC_SET_ARG1(data, val)   ((data).args[1] = (val))
#define IPC_SET_ARG2(data, val)   ((data).args[2] = (val))
#define IPC_SET_ARG3(data, val)   ((data).args[3] = (val))

#define IPC_GET_METHOD(data)           ((data).args[0])
#define IPC_GET_RETVAL(data)           ((data).args[0])

#define IPC_GET_ARG1(data)              ((data).args[1])
#define IPC_GET_ARG2(data)              ((data).args[2])
#define IPC_GET_ARG3(data)              ((data).args[3])

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
 * - sys_connect_to_me - sends a message IPC_M_CONNECTTOME
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
#define IPC_M_CONNECT_TO_ME     1
/** Protocol for CONNECT - ME - TO
 *
 * Calling process asks the callee to create for him a new connection.
 * E.g. the caller wants a name server to connect him to print server.
 *
 * The protocol for negotiating is:
 * - sys_connect_me_to - send a synchronous message to name server
 *                       indicating that it wants to be connected to some
 *                       service
 *                     - arg1/2 are user specified, arg3 contains
 *                       address of the phone that should be connected
 *                       (TODO: it leaks to userspace)
 *   recipient         -  if ipc_answer == 0, then accept connection
 *                     -  otherwise connection refused
 *                     -  recepient may forward message. Forwarding
 *                        system message 
 *
 */
#define IPC_M_CONNECT_ME_TO     2
/** This message is sent to answerbox when the phone
 * is hung up
 */
#define IPC_M_PHONE_HUNGUP      3

/** Send as_area over IPC 
 * - ARG1 - src as_area base address
 * - ARG2 - size of src as_area (filled automatically by kernel)
 * - ARG3 - flags of the as_area being sent
 * 
 * on answer:
 * - ARG1 - dst as_area base adress
 */
#define IPC_M_AS_AREA_SEND      5

/** Get as_area over IPC
 * - ARG1 - dst as_area base address
 * - ARG2 - dst as_area size
 * - ARG3 - user defined argument
 * 
 * on answer, the server must set:
 *
 * - ARG1 - src as_area base address
 * - ARG2 - flags that will be used for sharing
 */
#define IPC_M_AS_AREA_RECV      6


/* Well-known methods */
#define IPC_M_LAST_SYSTEM     511
#define IPC_M_PING            512
/* User methods */
#define FIRST_USER_METHOD     1024

#ifdef KERNEL

#include <proc/task.h>

typedef struct {
	unative_t args[IPC_CALL_LEN];
	phone_t *phone;
} ipc_data_t;

typedef struct {
	link_t link;

	int flags;

	/* Identification of the caller */
	task_t *sender;
	/* The caller box is different from sender->answerbox
	 * for synchronous calls
	 */
	answerbox_t *callerbox;

	unative_t priv; /**< Private data to internal IPC */

	ipc_data_t data;  /**< Data passed from/to userspace */
} call_t;

extern void ipc_init(void);
extern call_t * ipc_wait_for_call(answerbox_t *box, uint32_t usec, int flags);
extern void ipc_answer(answerbox_t *box, call_t *request);
extern int ipc_call(phone_t *phone, call_t *call);
extern void ipc_call_sync(phone_t *phone, call_t *request);
extern void ipc_phone_init(phone_t *phone);
extern void ipc_phone_connect(phone_t *phone, answerbox_t *box);
extern void ipc_call_free(call_t *call);
extern call_t * ipc_call_alloc(int flags);
extern void ipc_answerbox_init(answerbox_t *box);
extern void ipc_call_static_init(call_t *call);
extern void task_print_list(void);
extern int ipc_forward(call_t *call, phone_t *newphone, answerbox_t *oldbox);
void ipc_cleanup(void);
int ipc_phone_hangup(phone_t *phone);
extern void ipc_backsend_err(phone_t *phone, call_t *call, unative_t err);
extern void ipc_print_task(task_id_t taskid);

extern answerbox_t *ipc_phone_0;

#endif

#endif

/** @}
 */
