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

/** @addtogroup libc
 * @{
 */
/** @file
 */

/**
 * Asynchronous library
 *
 * The aim of this library is to provide a facility for writing programs which
 * utilize the asynchronous nature of HelenOS IPC, yet using a normal way of
 * programming.
 *
 * You should be able to write very simple multithreaded programs. The async
 * framework will automatically take care of most of the synchronization
 * problems.
 *
 * Example of use (pseudo C):
 *
 * 1) Multithreaded client application
 *
 *   fibril_create(fibril1, ...);
 *   fibril_create(fibril2, ...);
 *   ...
 *
 *   int fibril1(void *arg)
 *   {
 *     conn = async_connect_me_to(...);
 *
 *     exch = async_exchange_begin(conn);
 *     c1 = async_send(exch);
 *     async_exchange_end(exch);
 *
 *     exch = async_exchange_begin(conn);
 *     c2 = async_send(exch);
 *     async_exchange_end(exch);
 *
 *     async_wait_for(c1);
 *     async_wait_for(c2);
 *     ...
 *   }
 *
 *
 * 2) Multithreaded server application
 *
 *   main()
 *   {
 *     async_manager();
 *   }
 *
 *   port_handler(ichandle, *icall)
 *   {
 *     if (want_refuse) {
 *       async_answer_0(ichandle, ELIMIT);
 *       return;
 *     }
 *     async_answer_0(ichandle, EOK);
 *
 *     chandle = async_get_call(&call);
 *     somehow_handle_the_call(chandle, call);
 *     async_answer_2(chandle, 1, 2, 3);
 *
 *     chandle = async_get_call(&call);
 *     ...
 *   }
 *
 */

#define _LIBC_ASYNC_C_
#include <ipc/ipc.h>
#include <async.h>
#include "../private/async.h"
#include "../private/ns.h"
#undef _LIBC_ASYNC_C_

#include <ipc/irq.h>
#include <ipc/event.h>
#include <fibril.h>
#include <adt/hash_table.h>
#include <adt/hash.h>
#include <adt/list.h>
#include <assert.h>
#include <errno.h>
#include <time.h>
#include <barrier.h>
#include <stdbool.h>
#include <stdlib.h>
#include <mem.h>
#include <stdlib.h>
#include <macros.h>
#include <as.h>
#include <abi/mm/as.h>
#include "../private/libc.h"
#include "../private/fibril.h"

static fibril_rmutex_t message_mutex;

/** Naming service session */
async_sess_t session_ns;

/** Message data */
typedef struct {
	fibril_event_t received;

	/** If reply was received. */
	bool done;

	/** If the message / reply should be discarded on arrival. */
	bool forget;

	/** Pointer to where the answer data is stored. */
	ipc_call_t *dataptr;

	errno_t retval;
} amsg_t;

static amsg_t *amsg_create(void)
{
	return calloc(1, sizeof(amsg_t));
}

static void amsg_destroy(amsg_t *msg)
{
	free(msg);
}

/** Mutex protecting inactive_exch_list and avail_phone_cv.
 *
 */
static FIBRIL_MUTEX_INITIALIZE(async_sess_mutex);

/** List of all currently inactive exchanges.
 *
 */
static LIST_INITIALIZE(inactive_exch_list);

/** Condition variable to wait for a phone to become available.
 *
 */
static FIBRIL_CONDVAR_INITIALIZE(avail_phone_cv);

/** Initialize the async framework.
 *
 */
void __async_client_init(void)
{
	if (fibril_rmutex_initialize(&message_mutex) != EOK)
		abort();

	session_ns.iface = 0;
	session_ns.mgmt = EXCHANGE_ATOMIC;
	session_ns.phone = PHONE_NS;
	session_ns.arg1 = 0;
	session_ns.arg2 = 0;
	session_ns.arg3 = 0;

	fibril_mutex_initialize(&session_ns.remote_state_mtx);
	session_ns.remote_state_data = NULL;

	list_initialize(&session_ns.exch_list);
	fibril_mutex_initialize(&session_ns.mutex);
	session_ns.exchanges = 0;
}

void __async_client_fini(void)
{
	fibril_rmutex_destroy(&message_mutex);
}

/** Reply received callback.
 *
 * This function is called whenever a reply for an asynchronous message sent out
 * by the asynchronous framework is received.
 *
 * Notify the fibril which is waiting for this message that it has arrived.
 *
 * @param arg    Pointer to the asynchronous message record.
 * @param retval Value returned in the answer.
 * @param data   Call data of the answer.
 *
 */
void async_reply_received(ipc_call_t *data)
{
	amsg_t *msg = (amsg_t *) data->answer_label;
	if (!msg)
		return;

	fibril_rmutex_lock(&message_mutex);

	msg->retval = ipc_get_retval(data);

	/* Copy data inside lock, just in case the call was detached */
	if ((msg->dataptr) && (data))
		*msg->dataptr = *data;

	msg->done = true;

	if (msg->forget) {
		amsg_destroy(msg);
	} else {
		fibril_notify(&msg->received);
	}

	fibril_rmutex_unlock(&message_mutex);
}

/** Send message and return id of the sent message.
 *
 * The return value can be used as input for async_wait() to wait for
 * completion.
 *
 * @param exch    Exchange for sending the message.
 * @param imethod Service-defined interface and method.
 * @param arg1    Service-defined payload argument.
 * @param arg2    Service-defined payload argument.
 * @param arg3    Service-defined payload argument.
 * @param arg4    Service-defined payload argument.
 * @param dataptr If non-NULL, storage where the reply data will be stored.
 *
 * @return Hash of the sent message or 0 on error.
 *
 */
static aid_t async_send_fast(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, ipc_call_t *dataptr)
{
	if (exch == NULL)
		return 0;

	amsg_t *msg = amsg_create();
	if (msg == NULL)
		return 0;

	msg->dataptr = dataptr;

	errno_t rc = ipc_call_async_4(exch->phone, imethod, arg1, arg2, arg3,
	    arg4, msg);
	if (rc != EOK) {
		msg->retval = rc;
		msg->done = true;
	}

	return (aid_t) msg;
}

/** Send message and return id of the sent message
 *
 * The return value can be used as input for async_wait() to wait for
 * completion.
 *
 * @param exch    Exchange for sending the message.
 * @param imethod Service-defined interface and method.
 * @param arg1    Service-defined payload argument.
 * @param arg2    Service-defined payload argument.
 * @param arg3    Service-defined payload argument.
 * @param arg4    Service-defined payload argument.
 * @param arg5    Service-defined payload argument.
 * @param dataptr If non-NULL, storage where the reply data will be
 *                stored.
 *
 * @return Hash of the sent message or 0 on error.
 *
 */
static aid_t async_send_slow(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5,
    ipc_call_t *dataptr)
{
	if (exch == NULL)
		return 0;

	amsg_t *msg = amsg_create();
	if (msg == NULL)
		return 0;

	msg->dataptr = dataptr;

	errno_t rc = ipc_call_async_5(exch->phone, imethod, arg1, arg2, arg3,
	    arg4, arg5, msg);
	if (rc != EOK) {
		msg->retval = rc;
		msg->done = true;
	}

	return (aid_t) msg;
}

aid_t async_send_0(async_exch_t *exch, sysarg_t imethod, ipc_call_t *dataptr)
{
	return async_send_fast(exch, imethod, 0, 0, 0, 0, dataptr);
}

aid_t async_send_1(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    ipc_call_t *dataptr)
{
	return async_send_fast(exch, imethod, arg1, 0, 0, 0, dataptr);
}

aid_t async_send_2(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, ipc_call_t *dataptr)
{
	return async_send_fast(exch, imethod, arg1, arg2, 0, 0, dataptr);
}

aid_t async_send_3(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, ipc_call_t *dataptr)
{
	return async_send_fast(exch, imethod, arg1, arg2, arg3, 0, dataptr);
}

aid_t async_send_4(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, ipc_call_t *dataptr)
{
	return async_send_fast(exch, imethod, arg1, arg2, arg3, arg4, dataptr);
}

aid_t async_send_5(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5,
    ipc_call_t *dataptr)
{
	return async_send_slow(exch, imethod, arg1, arg2, arg3, arg4, arg5,
	    dataptr);
}

/** Wait for a message sent by the async framework.
 *
 * @param amsgid Hash of the message to wait for.
 * @param retval Pointer to storage where the retval of the answer will
 *               be stored.
 *
 */
void async_wait_for(aid_t amsgid, errno_t *retval)
{
	if (amsgid == 0) {
		if (retval)
			*retval = ENOMEM;
		return;
	}

	amsg_t *msg = (amsg_t *) amsgid;
	fibril_wait_for(&msg->received);

	if (retval)
		*retval = msg->retval;

	amsg_destroy(msg);
}

/** Wait for a message sent by the async framework, timeout variant.
 *
 * If the wait times out, the caller may choose to either wait again by calling
 * async_wait_for() or async_wait_timeout(), or forget the message via
 * async_forget().
 *
 * @param amsgid  Hash of the message to wait for.
 * @param retval  Pointer to storage where the retval of the answer will
 *                be stored.
 * @param timeout Timeout in microseconds.
 *
 * @return Zero on success, ETIMEOUT if the timeout has expired.
 *
 */
errno_t async_wait_timeout(aid_t amsgid, errno_t *retval, usec_t timeout)
{
	if (amsgid == 0) {
		if (retval)
			*retval = ENOMEM;
		return EOK;
	}

	amsg_t *msg = (amsg_t *) amsgid;

	/*
	 * Negative timeout is converted to zero timeout to avoid
	 * using tv_add with negative augmenter.
	 */
	if (timeout < 0)
		timeout = 0;

	struct timespec expires;
	getuptime(&expires);
	ts_add_diff(&expires, USEC2NSEC(timeout));

	errno_t rc = fibril_wait_timeout(&msg->received, &expires);
	if (rc != EOK)
		return rc;

	if (retval)
		*retval = msg->retval;

	amsg_destroy(msg);

	return EOK;
}

/** Discard the message / reply on arrival.
 *
 * The message will be marked to be discarded once the reply arrives in
 * reply_received(). It is not allowed to call async_wait_for() or
 * async_wait_timeout() on this message after a call to this function.
 *
 * @param amsgid  Hash of the message to forget.
 */
void async_forget(aid_t amsgid)
{
	if (amsgid == 0)
		return;

	amsg_t *msg = (amsg_t *) amsgid;

	assert(!msg->forget);

	fibril_rmutex_lock(&message_mutex);

	if (msg->done) {
		amsg_destroy(msg);
	} else {
		msg->dataptr = NULL;
		msg->forget = true;
	}

	fibril_rmutex_unlock(&message_mutex);
}

/** Pseudo-synchronous message sending - fast version.
 *
 * Send message asynchronously and return only after the reply arrives.
 *
 * This function can only transfer 4 register payload arguments. For
 * transferring more arguments, see the slower async_req_slow().
 *
 * @param exch    Exchange for sending the message.
 * @param imethod Interface and method of the call.
 * @param arg1    Service-defined payload argument.
 * @param arg2    Service-defined payload argument.
 * @param arg3    Service-defined payload argument.
 * @param arg4    Service-defined payload argument.
 * @param r1      If non-NULL, storage for the 1st reply argument.
 * @param r2      If non-NULL, storage for the 2nd reply argument.
 * @param r3      If non-NULL, storage for the 3rd reply argument.
 * @param r4      If non-NULL, storage for the 4th reply argument.
 * @param r5      If non-NULL, storage for the 5th reply argument.
 *
 * @return Return code of the reply or an error code.
 *
 */
static errno_t async_req_fast(async_exch_t *exch, sysarg_t imethod,
    sysarg_t arg1, sysarg_t arg2, sysarg_t arg3, sysarg_t arg4,
    sysarg_t *r1, sysarg_t *r2, sysarg_t *r3, sysarg_t *r4, sysarg_t *r5)
{
	if (exch == NULL)
		return ENOENT;

	ipc_call_t result;
	aid_t aid = async_send_4(exch, imethod, arg1, arg2, arg3, arg4,
	    &result);

	errno_t rc;
	async_wait_for(aid, &rc);

	if (r1)
		*r1 = ipc_get_arg1(&result);

	if (r2)
		*r2 = ipc_get_arg2(&result);

	if (r3)
		*r3 = ipc_get_arg3(&result);

	if (r4)
		*r4 = ipc_get_arg4(&result);

	if (r5)
		*r5 = ipc_get_arg5(&result);

	return rc;
}

/** Pseudo-synchronous message sending - slow version.
 *
 * Send message asynchronously and return only after the reply arrives.
 *
 * @param exch    Exchange for sending the message.
 * @param imethod Interface and method of the call.
 * @param arg1    Service-defined payload argument.
 * @param arg2    Service-defined payload argument.
 * @param arg3    Service-defined payload argument.
 * @param arg4    Service-defined payload argument.
 * @param arg5    Service-defined payload argument.
 * @param r1      If non-NULL, storage for the 1st reply argument.
 * @param r2      If non-NULL, storage for the 2nd reply argument.
 * @param r3      If non-NULL, storage for the 3rd reply argument.
 * @param r4      If non-NULL, storage for the 4th reply argument.
 * @param r5      If non-NULL, storage for the 5th reply argument.
 *
 * @return Return code of the reply or an error code.
 *
 */
static errno_t async_req_slow(async_exch_t *exch, sysarg_t imethod,
    sysarg_t arg1, sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5,
    sysarg_t *r1, sysarg_t *r2, sysarg_t *r3, sysarg_t *r4, sysarg_t *r5)
{
	if (exch == NULL)
		return ENOENT;

	ipc_call_t result;
	aid_t aid = async_send_5(exch, imethod, arg1, arg2, arg3, arg4, arg5,
	    &result);

	errno_t rc;
	async_wait_for(aid, &rc);

	if (r1)
		*r1 = ipc_get_arg1(&result);

	if (r2)
		*r2 = ipc_get_arg2(&result);

	if (r3)
		*r3 = ipc_get_arg3(&result);

	if (r4)
		*r4 = ipc_get_arg4(&result);

	if (r5)
		*r5 = ipc_get_arg5(&result);

	return rc;
}

errno_t async_req_0_0(async_exch_t *exch, sysarg_t imethod)
{
	return async_req_fast(exch, imethod, 0, 0, 0, 0, NULL, NULL, NULL, NULL,
	    NULL);
}

errno_t async_req_0_1(async_exch_t *exch, sysarg_t imethod, sysarg_t *r1)
{
	return async_req_fast(exch, imethod, 0, 0, 0, 0, r1, NULL, NULL, NULL,
	    NULL);
}

errno_t async_req_0_2(async_exch_t *exch, sysarg_t imethod, sysarg_t *r1,
    sysarg_t *r2)
{
	return async_req_fast(exch, imethod, 0, 0, 0, 0, r1, r2, NULL, NULL, NULL);
}

errno_t async_req_0_3(async_exch_t *exch, sysarg_t imethod, sysarg_t *r1,
    sysarg_t *r2, sysarg_t *r3)
{
	return async_req_fast(exch, imethod, 0, 0, 0, 0, r1, r2, r3, NULL, NULL);
}

errno_t async_req_0_4(async_exch_t *exch, sysarg_t imethod, sysarg_t *r1,
    sysarg_t *r2, sysarg_t *r3, sysarg_t *r4)
{
	return async_req_fast(exch, imethod, 0, 0, 0, 0, r1, r2, r3, r4, NULL);
}

errno_t async_req_0_5(async_exch_t *exch, sysarg_t imethod, sysarg_t *r1,
    sysarg_t *r2, sysarg_t *r3, sysarg_t *r4, sysarg_t *r5)
{
	return async_req_fast(exch, imethod, 0, 0, 0, 0, r1, r2, r3, r4, r5);
}

errno_t async_req_1_0(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1)
{
	return async_req_fast(exch, imethod, arg1, 0, 0, 0, NULL, NULL, NULL, NULL,
	    NULL);
}

errno_t async_req_1_1(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t *r1)
{
	return async_req_fast(exch, imethod, arg1, 0, 0, 0, r1, NULL, NULL, NULL,
	    NULL);
}

errno_t async_req_1_2(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t *r1, sysarg_t *r2)
{
	return async_req_fast(exch, imethod, arg1, 0, 0, 0, r1, r2, NULL, NULL,
	    NULL);
}

errno_t async_req_1_3(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t *r1, sysarg_t *r2, sysarg_t *r3)
{
	return async_req_fast(exch, imethod, arg1, 0, 0, 0, r1, r2, r3, NULL,
	    NULL);
}

errno_t async_req_1_4(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t *r1, sysarg_t *r2, sysarg_t *r3, sysarg_t *r4)
{
	return async_req_fast(exch, imethod, arg1, 0, 0, 0, r1, r2, r3, r4, NULL);
}

errno_t async_req_1_5(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t *r1, sysarg_t *r2, sysarg_t *r3, sysarg_t *r4, sysarg_t *r5)
{
	return async_req_fast(exch, imethod, arg1, 0, 0, 0, r1, r2, r3, r4, r5);
}

errno_t async_req_2_0(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2)
{
	return async_req_fast(exch, imethod, arg1, arg2, 0, 0, NULL, NULL, NULL,
	    NULL, NULL);
}

errno_t async_req_2_1(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t *r1)
{
	return async_req_fast(exch, imethod, arg1, arg2, 0, 0, r1, NULL, NULL,
	    NULL, NULL);
}

errno_t async_req_2_2(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t *r1, sysarg_t *r2)
{
	return async_req_fast(exch, imethod, arg1, arg2, 0, 0, r1, r2, NULL,
	    NULL, NULL);
}

errno_t async_req_2_3(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t *r1, sysarg_t *r2, sysarg_t *r3)
{
	return async_req_fast(exch, imethod, arg1, arg2, 0, 0, r1, r2, r3, NULL,
	    NULL);
}

errno_t async_req_2_4(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t *r1, sysarg_t *r2, sysarg_t *r3, sysarg_t *r4)
{
	return async_req_fast(exch, imethod, arg1, arg2, 0, 0, r1, r2, r3, r4,
	    NULL);
}

errno_t async_req_2_5(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t *r1, sysarg_t *r2, sysarg_t *r3, sysarg_t *r4, sysarg_t *r5)
{
	return async_req_fast(exch, imethod, arg1, arg2, 0, 0, r1, r2, r3, r4, r5);
}

errno_t async_req_3_0(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3)
{
	return async_req_fast(exch, imethod, arg1, arg2, arg3, 0, NULL, NULL, NULL,
	    NULL, NULL);
}

errno_t async_req_3_1(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t *r1)
{
	return async_req_fast(exch, imethod, arg1, arg2, arg3, 0, r1, NULL, NULL,
	    NULL, NULL);
}

errno_t async_req_3_2(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t *r1, sysarg_t *r2)
{
	return async_req_fast(exch, imethod, arg1, arg2, arg3, 0, r1, r2, NULL,
	    NULL, NULL);
}

errno_t async_req_3_3(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t *r1, sysarg_t *r2, sysarg_t *r3)
{
	return async_req_fast(exch, imethod, arg1, arg2, arg3, 0, r1, r2, r3, NULL,
	    NULL);
}

errno_t async_req_3_4(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t *r1, sysarg_t *r2, sysarg_t *r3,
    sysarg_t *r4)
{
	return async_req_fast(exch, imethod, arg1, arg2, arg3, 0, r1, r2, r3, r4,
	    NULL);
}

errno_t async_req_3_5(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t *r1, sysarg_t *r2, sysarg_t *r3,
    sysarg_t *r4, sysarg_t *r5)
{
	return async_req_fast(exch, imethod, arg1, arg2, arg3, 0, r1, r2, r3, r4,
	    r5);
}

errno_t async_req_4_0(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4)
{
	return async_req_fast(exch, imethod, arg1, arg2, arg3, arg4, NULL, NULL,
	    NULL, NULL, NULL);
}

errno_t async_req_4_1(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t *r1)
{
	return async_req_fast(exch, imethod, arg1, arg2, arg3, arg4, r1, NULL,
	    NULL, NULL, NULL);
}

errno_t async_req_4_2(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t *r1, sysarg_t *r2)
{
	return async_req_fast(exch, imethod, arg1, arg2, arg3, arg4, r1, r2, NULL,
	    NULL, NULL);
}

errno_t async_req_4_3(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t *r1, sysarg_t *r2,
    sysarg_t *r3)
{
	return async_req_fast(exch, imethod, arg1, arg2, arg3, arg4, r1, r2, r3,
	    NULL, NULL);
}

errno_t async_req_4_4(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t *r1, sysarg_t *r2,
    sysarg_t *r3, sysarg_t *r4)
{
	return async_req_fast(exch, imethod, arg1, arg2, arg3, arg4, r1, r2, r3,
	    r4, NULL);
}

errno_t async_req_4_5(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t *r1, sysarg_t *r2,
    sysarg_t *r3, sysarg_t *r4, sysarg_t *r5)
{
	return async_req_fast(exch, imethod, arg1, arg2, arg3, arg4, r1, r2, r3,
	    r4, r5);
}

errno_t async_req_5_0(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5)
{
	return async_req_slow(exch, imethod, arg1, arg2, arg3, arg4, arg5, NULL,
	    NULL, NULL, NULL, NULL);
}

errno_t async_req_5_1(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5, sysarg_t *r1)
{
	return async_req_slow(exch, imethod, arg1, arg2, arg3, arg4, arg5, r1,
	    NULL, NULL, NULL, NULL);
}

errno_t async_req_5_2(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5, sysarg_t *r1,
    sysarg_t *r2)
{
	return async_req_slow(exch, imethod, arg1, arg2, arg3, arg4, arg5, r1, r2,
	    NULL, NULL, NULL);
}

errno_t async_req_5_3(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5, sysarg_t *r1,
    sysarg_t *r2, sysarg_t *r3)
{
	return async_req_slow(exch, imethod, arg1, arg2, arg3, arg4, arg5, r1, r2,
	    r3, NULL, NULL);
}

errno_t async_req_5_4(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5, sysarg_t *r1,
    sysarg_t *r2, sysarg_t *r3, sysarg_t *r4)
{
	return async_req_slow(exch, imethod, arg1, arg2, arg3, arg4, arg5, r1, r2,
	    r3, r4, NULL);
}

errno_t async_req_5_5(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5, sysarg_t *r1,
    sysarg_t *r2, sysarg_t *r3, sysarg_t *r4, sysarg_t *r5)
{
	return async_req_slow(exch, imethod, arg1, arg2, arg3, arg4, arg5, r1, r2,
	    r3, r4, r5);
}

void async_msg_0(async_exch_t *exch, sysarg_t imethod)
{
	if (exch != NULL)
		ipc_call_async_0(exch->phone, imethod, NULL);
}

void async_msg_1(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1)
{
	if (exch != NULL)
		ipc_call_async_1(exch->phone, imethod, arg1, NULL);
}

void async_msg_2(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2)
{
	if (exch != NULL)
		ipc_call_async_2(exch->phone, imethod, arg1, arg2, NULL);
}

void async_msg_3(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3)
{
	if (exch != NULL)
		ipc_call_async_3(exch->phone, imethod, arg1, arg2, arg3, NULL);
}

void async_msg_4(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4)
{
	if (exch != NULL)
		ipc_call_async_4(exch->phone, imethod, arg1, arg2, arg3, arg4,
		    NULL);
}

void async_msg_5(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5)
{
	if (exch != NULL)
		ipc_call_async_5(exch->phone, imethod, arg1, arg2, arg3, arg4,
		    arg5, NULL);
}

static errno_t async_connect_me_to_internal(cap_phone_handle_t phone,
    iface_t iface, sysarg_t arg2, sysarg_t arg3, sysarg_t flags,
    cap_phone_handle_t *out_phone)
{
	ipc_call_t result;

	// XXX: Workaround for GCC's inability to infer association between
	// rc == EOK and *out_phone being assigned.
	*out_phone = CAP_NIL;

	amsg_t *msg = amsg_create();
	if (!msg)
		return ENOENT;

	msg->dataptr = &result;

	errno_t rc = ipc_call_async_4(phone, IPC_M_CONNECT_ME_TO,
	    (sysarg_t) iface, arg2, arg3, flags, msg);
	if (rc != EOK) {
		msg->retval = rc;
		msg->done = true;
	}

	async_wait_for((aid_t) msg, &rc);

	if (rc != EOK)
		return rc;

	*out_phone = (cap_phone_handle_t) ipc_get_arg5(&result);
	return EOK;
}

/** Wrapper for making IPC_M_CONNECT_ME_TO calls using the async framework.
 *
 * Ask through phone for a new connection to some service and block until
 * success.
 *
 * @param exch  Exchange for sending the message.
 * @param iface Connection interface.
 * @param arg2  User defined argument.
 * @param arg3  User defined argument.
 * @param rc    Placeholder for return code. Unused if NULL.
 *
 * @return New session on success or NULL on error.
 *
 */
async_sess_t *async_connect_me_to(async_exch_t *exch, iface_t iface,
    sysarg_t arg2, sysarg_t arg3, errno_t *rc)
{
	if (exch == NULL) {
		if (rc != NULL)
			*rc = ENOENT;

		return NULL;
	}

	async_sess_t *sess = calloc(1, sizeof(async_sess_t));
	if (sess == NULL) {
		if (rc != NULL)
			*rc = ENOMEM;

		return NULL;
	}

	cap_phone_handle_t phone;
	errno_t ret = async_connect_me_to_internal(exch->phone, iface, arg2,
	    arg3, 0, &phone);
	if (ret != EOK) {
		if (rc != NULL)
			*rc = ret;

		free(sess);
		return NULL;
	}

	sess->iface = iface;
	sess->phone = phone;
	sess->arg1 = iface;
	sess->arg2 = arg2;
	sess->arg3 = arg3;

	fibril_mutex_initialize(&sess->remote_state_mtx);
	list_initialize(&sess->exch_list);
	fibril_mutex_initialize(&sess->mutex);

	return sess;
}

/** Set arguments for new connections.
 *
 * FIXME This is an ugly hack to work around the problem that parallel
 * exchanges are implemented using parallel connections. When we create
 * a callback session, the framework does not know arguments for the new
 * connections.
 *
 * The proper solution seems to be to implement parallel exchanges using
 * tagging.
 *
 */
void async_sess_args_set(async_sess_t *sess, iface_t iface, sysarg_t arg2,
    sysarg_t arg3)
{
	sess->arg1 = iface;
	sess->arg2 = arg2;
	sess->arg3 = arg3;
}

/** Wrapper for making IPC_M_CONNECT_ME_TO calls using the async framework.
 *
 * Ask through phone for a new connection to some service and block until
 * success.
 *
 * @param exch  Exchange for sending the message.
 * @param iface Connection interface.
 * @param arg2  User defined argument.
 * @param arg3  User defined argument.
 * @param rc    Placeholder for return code. Unused if NULL.
 *
 * @return New session on success or NULL on error.
 *
 */
async_sess_t *async_connect_me_to_blocking(async_exch_t *exch, iface_t iface,
    sysarg_t arg2, sysarg_t arg3, errno_t *rc)
{
	if (exch == NULL) {
		if (rc != NULL)
			*rc = ENOENT;

		return NULL;
	}

	async_sess_t *sess = calloc(1, sizeof(async_sess_t));
	if (sess == NULL) {
		if (rc != NULL)
			*rc = ENOMEM;

		return NULL;
	}

	cap_phone_handle_t phone;
	errno_t ret = async_connect_me_to_internal(exch->phone, iface, arg2,
	    arg3, IPC_FLAG_BLOCKING, &phone);
	if (ret != EOK) {
		if (rc != NULL)
			*rc = ret;

		free(sess);
		return NULL;
	}

	sess->iface = iface;
	sess->phone = phone;
	sess->arg1 = iface;
	sess->arg2 = arg2;
	sess->arg3 = arg3;

	fibril_mutex_initialize(&sess->remote_state_mtx);
	list_initialize(&sess->exch_list);
	fibril_mutex_initialize(&sess->mutex);

	return sess;
}

/** Connect to a task specified by id.
 *
 * @param id Task to which to connect.
 * @param rc Placeholder for return code. Unused if NULL.
 *
 * @return New session on success or NULL on error.
 *
 */
async_sess_t *async_connect_kbox(task_id_t id, errno_t *rc)
{
	async_sess_t *sess = calloc(1, sizeof(async_sess_t));
	if (sess == NULL) {
		if (rc != NULL)
			*rc = ENOMEM;

		return NULL;
	}

	cap_phone_handle_t phone;
	errno_t ret = ipc_connect_kbox(id, &phone);
	if (ret != EOK) {
		if (rc != NULL)
			*rc = ret;

		free(sess);
		return NULL;
	}

	sess->iface = 0;
	sess->mgmt = EXCHANGE_ATOMIC;
	sess->phone = phone;

	fibril_mutex_initialize(&sess->remote_state_mtx);
	list_initialize(&sess->exch_list);
	fibril_mutex_initialize(&sess->mutex);

	return sess;
}

static void async_hangup_internal(cap_phone_handle_t phone)
{
	errno_t rc;

	rc = ipc_hangup(phone);
	assert(rc == EOK);
	(void) rc;
}

/** Wrapper for ipc_hangup.
 *
 * @param sess Session to hung up.
 */
void async_hangup(async_sess_t *sess)
{
	async_exch_t *exch;

	assert(sess);

	fibril_mutex_lock(&async_sess_mutex);
	assert(sess->exchanges == 0);

	async_hangup_internal(sess->phone);

	while (!list_empty(&sess->exch_list)) {
		exch = (async_exch_t *)
		    list_get_instance(list_first(&sess->exch_list),
		    async_exch_t, sess_link);

		list_remove(&exch->sess_link);
		list_remove(&exch->global_link);
		if (sess->mgmt != EXCHANGE_ATOMIC &&
		    sess->mgmt != EXCHANGE_SERIALIZE)
			async_hangup_internal(exch->phone);
		free(exch);
	}

	free(sess);

	fibril_mutex_unlock(&async_sess_mutex);
}

/** Start new exchange in a session.
 *
 * @param session Session.
 *
 * @return New exchange or NULL on error.
 *
 */
async_exch_t *async_exchange_begin(async_sess_t *sess)
{
	if (sess == NULL)
		return NULL;

	exch_mgmt_t mgmt = sess->mgmt;
	if (sess->iface != 0)
		mgmt = sess->iface & IFACE_EXCHANGE_MASK;

	async_exch_t *exch = NULL;

	fibril_mutex_lock(&async_sess_mutex);

	if (!list_empty(&sess->exch_list)) {
		/*
		 * There are inactive exchanges in the session.
		 */
		exch = (async_exch_t *)
		    list_get_instance(list_first(&sess->exch_list),
		    async_exch_t, sess_link);

		list_remove(&exch->sess_link);
		list_remove(&exch->global_link);
	} else {
		/*
		 * There are no available exchanges in the session.
		 */

		if ((mgmt == EXCHANGE_ATOMIC) ||
		    (mgmt == EXCHANGE_SERIALIZE)) {
			exch = (async_exch_t *) malloc(sizeof(async_exch_t));
			if (exch != NULL) {
				link_initialize(&exch->sess_link);
				link_initialize(&exch->global_link);
				exch->sess = sess;
				exch->phone = sess->phone;
			}
		} else if (mgmt == EXCHANGE_PARALLEL) {
			cap_phone_handle_t phone;
			errno_t rc;

		retry:
			/*
			 * Make a one-time attempt to connect a new data phone.
			 */
			rc = async_connect_me_to_internal(sess->phone, sess->arg1,
			    sess->arg2, sess->arg3, 0, &phone);
			if (rc == EOK) {
				exch = (async_exch_t *) malloc(sizeof(async_exch_t));
				if (exch != NULL) {
					link_initialize(&exch->sess_link);
					link_initialize(&exch->global_link);
					exch->sess = sess;
					exch->phone = phone;
				} else
					async_hangup_internal(phone);
			} else if (!list_empty(&inactive_exch_list)) {
				/*
				 * We did not manage to connect a new phone. But we
				 * can try to close some of the currently inactive
				 * connections in other sessions and try again.
				 */
				exch = (async_exch_t *)
				    list_get_instance(list_first(&inactive_exch_list),
				    async_exch_t, global_link);

				list_remove(&exch->sess_link);
				list_remove(&exch->global_link);
				async_hangup_internal(exch->phone);
				free(exch);
				goto retry;
			} else {
				/*
				 * Wait for a phone to become available.
				 */
				fibril_condvar_wait(&avail_phone_cv, &async_sess_mutex);
				goto retry;
			}
		}
	}

	if (exch != NULL)
		sess->exchanges++;

	fibril_mutex_unlock(&async_sess_mutex);

	if (exch != NULL && mgmt == EXCHANGE_SERIALIZE)
		fibril_mutex_lock(&sess->mutex);

	return exch;
}

/** Finish an exchange.
 *
 * @param exch Exchange to finish.
 *
 */
void async_exchange_end(async_exch_t *exch)
{
	if (exch == NULL)
		return;

	async_sess_t *sess = exch->sess;
	assert(sess != NULL);

	exch_mgmt_t mgmt = sess->mgmt;
	if (sess->iface != 0)
		mgmt = sess->iface & IFACE_EXCHANGE_MASK;

	if (mgmt == EXCHANGE_SERIALIZE)
		fibril_mutex_unlock(&sess->mutex);

	fibril_mutex_lock(&async_sess_mutex);

	sess->exchanges--;

	list_append(&exch->sess_link, &sess->exch_list);
	list_append(&exch->global_link, &inactive_exch_list);
	fibril_condvar_signal(&avail_phone_cv);

	fibril_mutex_unlock(&async_sess_mutex);
}

/** Wrapper for IPC_M_SHARE_IN calls using the async framework.
 *
 * @param exch  Exchange for sending the message.
 * @param size  Size of the destination address space area.
 * @param arg   User defined argument.
 * @param flags Storage for the received flags. Can be NULL.
 * @param dst   Address of the storage for the destination address space area
 *              base address. Cannot be NULL.
 *
 * @return Zero on success or an error code from errno.h.
 *
 */
static errno_t async_share_in_start(async_exch_t *exch, size_t size,
    sysarg_t arg, unsigned int *flags, void **dst)
{
	if (exch == NULL)
		return ENOENT;

	sysarg_t _flags = 0;
	sysarg_t _dst = (sysarg_t) -1;
	errno_t res = async_req_3_5(exch, IPC_M_SHARE_IN, (sysarg_t) size,
	    (sysarg_t) __progsymbols.end, arg, NULL, &_flags, NULL, NULL,
	    &_dst);

	if (flags)
		*flags = (unsigned int) _flags;

	*dst = (void *) _dst;
	return res;
}

errno_t async_share_in_start_0_0(async_exch_t *exch, size_t size, void **dst)
{
	return async_share_in_start(exch, size, 0, NULL, dst);
}

errno_t async_share_in_start_0_1(async_exch_t *exch, size_t size,
    unsigned int *flags, void **dst)
{
	return async_share_in_start(exch, size, 0, flags, dst);
}

errno_t async_share_in_start_1_0(async_exch_t *exch, size_t size, sysarg_t arg,
    void **dst)
{
	return async_share_in_start(exch, size, arg, NULL, dst);
}

errno_t async_share_in_start_1_1(async_exch_t *exch, size_t size, sysarg_t arg,
    unsigned int *flags, void **dst)
{
	return async_share_in_start(exch, size, arg, flags, dst);
}

/** Wrapper for IPC_M_SHARE_OUT calls using the async framework.
 *
 * @param exch  Exchange for sending the message.
 * @param src   Source address space area base address.
 * @param flags Flags to be used for sharing. Bits can be only cleared.
 *
 * @return Zero on success or an error code from errno.h.
 *
 */
errno_t async_share_out_start(async_exch_t *exch, void *src, unsigned int flags)
{
	if (exch == NULL)
		return ENOENT;

	return async_req_3_0(exch, IPC_M_SHARE_OUT, (sysarg_t) src, 0,
	    (sysarg_t) flags);
}

/** Start IPC_M_DATA_READ using the async framework.
 *
 * @param exch    Exchange for sending the message.
 * @param dst     Address of the beginning of the destination buffer.
 * @param size    Size of the destination buffer (in bytes).
 * @param dataptr Storage of call data (arg 2 holds actual data size).
 *
 * @return Hash of the sent message or 0 on error.
 *
 */
aid_t async_data_read(async_exch_t *exch, void *dst, size_t size,
    ipc_call_t *dataptr)
{
	return async_send_2(exch, IPC_M_DATA_READ, (sysarg_t) dst,
	    (sysarg_t) size, dataptr);
}

/** Wrapper for IPC_M_DATA_READ calls using the async framework.
 *
 * @param exch Exchange for sending the message.
 * @param dst  Address of the beginning of the destination buffer.
 * @param size Size of the destination buffer.
 *
 * @return Zero on success or an error code from errno.h.
 *
 */
errno_t async_data_read_start(async_exch_t *exch, void *dst, size_t size)
{
	if (exch == NULL)
		return ENOENT;

	return async_req_2_0(exch, IPC_M_DATA_READ, (sysarg_t) dst,
	    (sysarg_t) size);
}

/** Wrapper for IPC_M_DATA_WRITE calls using the async framework.
 *
 * @param exch Exchange for sending the message.
 * @param src  Address of the beginning of the source buffer.
 * @param size Size of the source buffer.
 *
 * @return Zero on success or an error code from errno.h.
 *
 */
errno_t async_data_write_start(async_exch_t *exch, const void *src, size_t size)
{
	if (exch == NULL)
		return ENOENT;

	return async_req_2_0(exch, IPC_M_DATA_WRITE, (sysarg_t) src,
	    (sysarg_t) size);
}

errno_t async_state_change_start(async_exch_t *exch, sysarg_t arg1, sysarg_t arg2,
    sysarg_t arg3, async_exch_t *other_exch)
{
	return async_req_5_0(exch, IPC_M_STATE_CHANGE_AUTHORIZE,
	    arg1, arg2, arg3, 0, cap_handle_raw(other_exch->phone));
}

/** Lock and get session remote state
 *
 * Lock and get the local replica of the remote state
 * in stateful sessions. The call should be paired
 * with async_remote_state_release*().
 *
 * @param[in] sess Stateful session.
 *
 * @return Local replica of the remote state.
 *
 */
void *async_remote_state_acquire(async_sess_t *sess)
{
	fibril_mutex_lock(&sess->remote_state_mtx);
	return sess->remote_state_data;
}

/** Update the session remote state
 *
 * Update the local replica of the remote state
 * in stateful sessions. The remote state must
 * be already locked.
 *
 * @param[in] sess  Stateful session.
 * @param[in] state New local replica of the remote state.
 *
 */
void async_remote_state_update(async_sess_t *sess, void *state)
{
	assert(fibril_mutex_is_locked(&sess->remote_state_mtx));
	sess->remote_state_data = state;
}

/** Release the session remote state
 *
 * Unlock the local replica of the remote state
 * in stateful sessions.
 *
 * @param[in] sess Stateful session.
 *
 */
void async_remote_state_release(async_sess_t *sess)
{
	assert(fibril_mutex_is_locked(&sess->remote_state_mtx));

	fibril_mutex_unlock(&sess->remote_state_mtx);
}

/** Release the session remote state and end an exchange
 *
 * Unlock the local replica of the remote state
 * in stateful sessions. This is convenience function
 * which gets the session pointer from the exchange
 * and also ends the exchange.
 *
 * @param[in] exch Stateful session's exchange.
 *
 */
void async_remote_state_release_exchange(async_exch_t *exch)
{
	if (exch == NULL)
		return;

	async_sess_t *sess = exch->sess;
	assert(fibril_mutex_is_locked(&sess->remote_state_mtx));

	async_exchange_end(exch);
	fibril_mutex_unlock(&sess->remote_state_mtx);
}

void *async_as_area_create(void *base, size_t size, unsigned int flags,
    async_sess_t *pager, sysarg_t id1, sysarg_t id2, sysarg_t id3)
{
	as_area_pager_info_t pager_info = {
		.pager = pager->phone,
		.id1 = id1,
		.id2 = id2,
		.id3 = id3
	};
	return as_area_create(base, size, flags, &pager_info);
}

/** @}
 */
