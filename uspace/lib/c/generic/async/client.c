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

#define LIBC_ASYNC_C_
#include <ipc/ipc.h>
#include <async.h>
#include "../private/async.h"
#undef LIBC_ASYNC_C_

#include <ipc/irq.h>
#include <ipc/event.h>
#include <futex.h>
#include <fibril.h>
#include <adt/hash_table.h>
#include <adt/hash.h>
#include <adt/list.h>
#include <assert.h>
#include <errno.h>
#include <sys/time.h>
#include <libarch/barrier.h>
#include <stdbool.h>
#include <stdlib.h>
#include <mem.h>
#include <stdlib.h>
#include <macros.h>
#include <as.h>
#include <abi/mm/as.h>
#include "../private/libc.h"

/** Naming service session */
async_sess_t *session_ns;

/** Message data */
typedef struct {
	awaiter_t wdata;

	/** If reply was received. */
	bool done;

	/** If the message / reply should be discarded on arrival. */
	bool forget;

	/** If already destroyed. */
	bool destroyed;

	/** Pointer to where the answer data is stored. */
	ipc_call_t *dataptr;

	errno_t retval;
} amsg_t;

static void to_event_initialize(to_event_t *to)
{
	struct timeval tv = { 0, 0 };

	to->inlist = false;
	to->occurred = false;
	link_initialize(&to->link);
	to->expires = tv;
}

static void wu_event_initialize(wu_event_t *wu)
{
	wu->inlist = false;
	link_initialize(&wu->link);
}

void awaiter_initialize(awaiter_t *aw)
{
	aw->fid = 0;
	aw->active = false;
	to_event_initialize(&aw->to_event);
	wu_event_initialize(&aw->wu_event);
}

static amsg_t *amsg_create(void)
{
	amsg_t *msg = malloc(sizeof(amsg_t));
	if (msg) {
		msg->done = false;
		msg->forget = false;
		msg->destroyed = false;
		msg->dataptr = NULL;
		msg->retval = EINVAL;
		awaiter_initialize(&msg->wdata);
	}

	return msg;
}

static void amsg_destroy(amsg_t *msg)
{
	assert(!msg->destroyed);
	msg->destroyed = true;
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
	session_ns = (async_sess_t *) malloc(sizeof(async_sess_t));
	if (session_ns == NULL)
		abort();

	session_ns->iface = 0;
	session_ns->mgmt = EXCHANGE_ATOMIC;
	session_ns->phone = PHONE_NS;
	session_ns->arg1 = 0;
	session_ns->arg2 = 0;
	session_ns->arg3 = 0;

	fibril_mutex_initialize(&session_ns->remote_state_mtx);
	session_ns->remote_state_data = NULL;

	list_initialize(&session_ns->exch_list);
	fibril_mutex_initialize(&session_ns->mutex);
	atomic_set(&session_ns->refcnt, 0);
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
static void reply_received(void *arg, errno_t retval, ipc_call_t *data)
{
	assert(arg);

	futex_down(&async_futex);

	amsg_t *msg = (amsg_t *) arg;
	msg->retval = retval;

	/* Copy data after futex_down, just in case the call was detached */
	if ((msg->dataptr) && (data))
		*msg->dataptr = *data;

	write_barrier();

	/* Remove message from timeout list */
	if (msg->wdata.to_event.inlist)
		list_remove(&msg->wdata.to_event.link);

	msg->done = true;

	if (msg->forget) {
		assert(msg->wdata.active);
		amsg_destroy(msg);
	} else if (!msg->wdata.active) {
		msg->wdata.active = true;
		fibril_add_ready(msg->wdata.fid);
	}

	futex_up(&async_futex);
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
aid_t async_send_fast(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, ipc_call_t *dataptr)
{
	if (exch == NULL)
		return 0;

	amsg_t *msg = amsg_create();
	if (msg == NULL)
		return 0;

	msg->dataptr = dataptr;
	msg->wdata.active = true;

	ipc_call_async_4(exch->phone, imethod, arg1, arg2, arg3, arg4, msg,
	    reply_received);

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
aid_t async_send_slow(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5,
    ipc_call_t *dataptr)
{
	if (exch == NULL)
		return 0;

	amsg_t *msg = amsg_create();
	if (msg == NULL)
		return 0;

	msg->dataptr = dataptr;
	msg->wdata.active = true;

	ipc_call_async_5(exch->phone, imethod, arg1, arg2, arg3, arg4, arg5,
	    msg, reply_received);

	return (aid_t) msg;
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
	assert(amsgid);

	amsg_t *msg = (amsg_t *) amsgid;

	futex_down(&async_futex);

	assert(!msg->forget);
	assert(!msg->destroyed);

	if (msg->done) {
		futex_up(&async_futex);
		goto done;
	}

	msg->wdata.fid = fibril_get_id();
	msg->wdata.active = false;
	msg->wdata.to_event.inlist = false;

	/* Leave the async_futex locked when entering this function */
	fibril_switch(FIBRIL_TO_MANAGER);

	/* Futex is up automatically after fibril_switch */

done:
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
errno_t async_wait_timeout(aid_t amsgid, errno_t *retval, suseconds_t timeout)
{
	assert(amsgid);

	amsg_t *msg = (amsg_t *) amsgid;

	futex_down(&async_futex);

	assert(!msg->forget);
	assert(!msg->destroyed);

	if (msg->done) {
		futex_up(&async_futex);
		goto done;
	}

	/*
	 * Negative timeout is converted to zero timeout to avoid
	 * using tv_add with negative augmenter.
	 */
	if (timeout < 0)
		timeout = 0;

	getuptime(&msg->wdata.to_event.expires);
	tv_add_diff(&msg->wdata.to_event.expires, timeout);

	/*
	 * Current fibril is inserted as waiting regardless of the
	 * "size" of the timeout.
	 *
	 * Checking for msg->done and immediately bailing out when
	 * timeout == 0 would mean that the manager fibril would never
	 * run (consider single threaded program).
	 * Thus the IPC answer would be never retrieved from the kernel.
	 *
	 * Notice that the actual delay would be very small because we
	 * - switch to manager fibril
	 * - the manager sees expired timeout
	 * - and thus adds us back to ready queue
	 * - manager switches back to some ready fibril
	 *   (prior it, it checks for incoming IPC).
	 *
	 */
	msg->wdata.fid = fibril_get_id();
	msg->wdata.active = false;
	async_insert_timeout(&msg->wdata);

	/* Leave the async_futex locked when entering this function */
	fibril_switch(FIBRIL_TO_MANAGER);

	/* Futex is up automatically after fibril_switch */

	if (!msg->done)
		return ETIMEOUT;

done:
	if (retval)
		*retval = msg->retval;

	amsg_destroy(msg);

	return 0;
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
	amsg_t *msg = (amsg_t *) amsgid;

	assert(msg);
	assert(!msg->forget);
	assert(!msg->destroyed);

	futex_down(&async_futex);

	if (msg->done) {
		amsg_destroy(msg);
	} else {
		msg->dataptr = NULL;
		msg->forget = true;
	}

	futex_up(&async_futex);
}

/** Wait for specified time.
 *
 * The current fibril is suspended but the thread continues to execute.
 *
 * @param timeout Duration of the wait in microseconds.
 *
 */
void async_usleep(suseconds_t timeout)
{
	awaiter_t awaiter;
	awaiter_initialize(&awaiter);

	awaiter.fid = fibril_get_id();

	getuptime(&awaiter.to_event.expires);
	tv_add_diff(&awaiter.to_event.expires, timeout);

	futex_down(&async_futex);

	async_insert_timeout(&awaiter);

	/* Leave the async_futex locked when entering this function */
	fibril_switch(FIBRIL_TO_MANAGER);

	/* Futex is up automatically after fibril_switch() */
}

/** Delay execution for the specified number of seconds
 *
 * @param sec Number of seconds to sleep
 */
void async_sleep(unsigned int sec)
{
	/*
	 * Sleep in 1000 second steps to support
	 * full argument range
	 */

	while (sec > 0) {
		unsigned int period = (sec > 1000) ? 1000 : sec;

		async_usleep(period * 1000000);
		sec -= period;
	}
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
errno_t async_req_fast(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t *r1, sysarg_t *r2,
    sysarg_t *r3, sysarg_t *r4, sysarg_t *r5)
{
	if (exch == NULL)
		return ENOENT;

	ipc_call_t result;
	aid_t aid = async_send_4(exch, imethod, arg1, arg2, arg3, arg4,
	    &result);

	errno_t rc;
	async_wait_for(aid, &rc);

	if (r1)
		*r1 = IPC_GET_ARG1(result);

	if (r2)
		*r2 = IPC_GET_ARG2(result);

	if (r3)
		*r3 = IPC_GET_ARG3(result);

	if (r4)
		*r4 = IPC_GET_ARG4(result);

	if (r5)
		*r5 = IPC_GET_ARG5(result);

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
errno_t async_req_slow(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5, sysarg_t *r1,
    sysarg_t *r2, sysarg_t *r3, sysarg_t *r4, sysarg_t *r5)
{
	if (exch == NULL)
		return ENOENT;

	ipc_call_t result;
	aid_t aid = async_send_5(exch, imethod, arg1, arg2, arg3, arg4, arg5,
	    &result);

	errno_t rc;
	async_wait_for(aid, &rc);

	if (r1)
		*r1 = IPC_GET_ARG1(result);

	if (r2)
		*r2 = IPC_GET_ARG2(result);

	if (r3)
		*r3 = IPC_GET_ARG3(result);

	if (r4)
		*r4 = IPC_GET_ARG4(result);

	if (r5)
		*r5 = IPC_GET_ARG5(result);

	return rc;
}

void async_msg_0(async_exch_t *exch, sysarg_t imethod)
{
	if (exch != NULL)
		ipc_call_async_0(exch->phone, imethod, NULL, NULL);
}

void async_msg_1(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1)
{
	if (exch != NULL)
		ipc_call_async_1(exch->phone, imethod, arg1, NULL, NULL);
}

void async_msg_2(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2)
{
	if (exch != NULL)
		ipc_call_async_2(exch->phone, imethod, arg1, arg2, NULL, NULL);
}

void async_msg_3(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3)
{
	if (exch != NULL)
		ipc_call_async_3(exch->phone, imethod, arg1, arg2, arg3, NULL,
		    NULL);
}

void async_msg_4(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4)
{
	if (exch != NULL)
		ipc_call_async_4(exch->phone, imethod, arg1, arg2, arg3, arg4,
		    NULL, NULL);
}

void async_msg_5(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5)
{
	if (exch != NULL)
		ipc_call_async_5(exch->phone, imethod, arg1, arg2, arg3, arg4,
		    arg5, NULL, NULL);
}

static errno_t async_connect_me_to_internal(cap_phone_handle_t phone,
    sysarg_t arg1, sysarg_t arg2, sysarg_t arg3, sysarg_t arg4,
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
	msg->wdata.active = true;

	ipc_call_async_4(phone, IPC_M_CONNECT_ME_TO, arg1, arg2, arg3, arg4,
	    msg, reply_received);

	errno_t rc;
	async_wait_for((aid_t) msg, &rc);

	if (rc != EOK)
		return rc;

	*out_phone = (cap_phone_handle_t) IPC_GET_ARG5(result);
	return EOK;
}

/** Wrapper for making IPC_M_CONNECT_ME_TO calls using the async framework.
 *
 * Ask through for a new connection to some service.
 *
 * @param mgmt Exchange management style.
 * @param exch Exchange for sending the message.
 * @param arg1 User defined argument.
 * @param arg2 User defined argument.
 * @param arg3 User defined argument.
 *
 * @return New session on success or NULL on error.
 *
 */
async_sess_t *async_connect_me_to(exch_mgmt_t mgmt, async_exch_t *exch,
    sysarg_t arg1, sysarg_t arg2, sysarg_t arg3)
{
	if (exch == NULL) {
		errno = ENOENT;
		return NULL;
	}

	async_sess_t *sess = (async_sess_t *) malloc(sizeof(async_sess_t));
	if (sess == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	cap_phone_handle_t phone;
	errno_t rc = async_connect_me_to_internal(exch->phone, arg1, arg2, arg3,
	    0, &phone);
	if (rc != EOK) {
		errno = rc;
		free(sess);
		return NULL;
	}

	sess->iface = 0;
	sess->mgmt = mgmt;
	sess->phone = phone;
	sess->arg1 = arg1;
	sess->arg2 = arg2;
	sess->arg3 = arg3;

	fibril_mutex_initialize(&sess->remote_state_mtx);
	sess->remote_state_data = NULL;

	list_initialize(&sess->exch_list);
	fibril_mutex_initialize(&sess->mutex);
	atomic_set(&sess->refcnt, 0);

	return sess;
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
 *
 * @return New session on success or NULL on error.
 *
 */
async_sess_t *async_connect_me_to_iface(async_exch_t *exch, iface_t iface,
    sysarg_t arg2, sysarg_t arg3)
{
	if (exch == NULL) {
		errno = ENOENT;
		return NULL;
	}

	async_sess_t *sess = (async_sess_t *) malloc(sizeof(async_sess_t));
	if (sess == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	cap_phone_handle_t phone;
	errno_t rc = async_connect_me_to_internal(exch->phone, iface, arg2,
	    arg3, 0, &phone);
	if (rc != EOK) {
		errno = rc;
		free(sess);
		return NULL;
	}

	sess->iface = iface;
	sess->phone = phone;
	sess->arg1 = iface;
	sess->arg2 = arg2;
	sess->arg3 = arg3;

	fibril_mutex_initialize(&sess->remote_state_mtx);
	sess->remote_state_data = NULL;

	list_initialize(&sess->exch_list);
	fibril_mutex_initialize(&sess->mutex);
	atomic_set(&sess->refcnt, 0);

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
 */
void async_sess_args_set(async_sess_t *sess, sysarg_t arg1, sysarg_t arg2,
    sysarg_t arg3)
{
	sess->arg1 = arg1;
	sess->arg2 = arg2;
	sess->arg3 = arg3;
}

/** Wrapper for making IPC_M_CONNECT_ME_TO calls using the async framework.
 *
 * Ask through phone for a new connection to some service and block until
 * success.
 *
 * @param mgmt Exchange management style.
 * @param exch Exchange for sending the message.
 * @param arg1 User defined argument.
 * @param arg2 User defined argument.
 * @param arg3 User defined argument.
 *
 * @return New session on success or NULL on error.
 *
 */
async_sess_t *async_connect_me_to_blocking(exch_mgmt_t mgmt, async_exch_t *exch,
    sysarg_t arg1, sysarg_t arg2, sysarg_t arg3)
{
	if (exch == NULL) {
		errno = ENOENT;
		return NULL;
	}

	async_sess_t *sess = (async_sess_t *) malloc(sizeof(async_sess_t));
	if (sess == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	cap_phone_handle_t phone;
	errno_t rc = async_connect_me_to_internal(exch->phone, arg1, arg2, arg3,
	    IPC_FLAG_BLOCKING, &phone);

	if (rc != EOK) {
		errno = rc;
		free(sess);
		return NULL;
	}

	sess->iface = 0;
	sess->mgmt = mgmt;
	sess->phone = phone;
	sess->arg1 = arg1;
	sess->arg2 = arg2;
	sess->arg3 = arg3;

	fibril_mutex_initialize(&sess->remote_state_mtx);
	sess->remote_state_data = NULL;

	list_initialize(&sess->exch_list);
	fibril_mutex_initialize(&sess->mutex);
	atomic_set(&sess->refcnt, 0);

	return sess;
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
 *
 * @return New session on success or NULL on error.
 *
 */
async_sess_t *async_connect_me_to_blocking_iface(async_exch_t *exch, iface_t iface,
    sysarg_t arg2, sysarg_t arg3)
{
	if (exch == NULL) {
		errno = ENOENT;
		return NULL;
	}

	async_sess_t *sess = (async_sess_t *) malloc(sizeof(async_sess_t));
	if (sess == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	cap_phone_handle_t phone;
	errno_t rc = async_connect_me_to_internal(exch->phone, iface, arg2,
	    arg3, IPC_FLAG_BLOCKING, &phone);
	if (rc != EOK) {
		errno = rc;
		free(sess);
		return NULL;
	}

	sess->iface = iface;
	sess->phone = phone;
	sess->arg1 = iface;
	sess->arg2 = arg2;
	sess->arg3 = arg3;

	fibril_mutex_initialize(&sess->remote_state_mtx);
	sess->remote_state_data = NULL;

	list_initialize(&sess->exch_list);
	fibril_mutex_initialize(&sess->mutex);
	atomic_set(&sess->refcnt, 0);

	return sess;
}

/** Connect to a task specified by id.
 *
 */
async_sess_t *async_connect_kbox(task_id_t id)
{
	async_sess_t *sess = (async_sess_t *) malloc(sizeof(async_sess_t));
	if (sess == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	cap_phone_handle_t phone;
	errno_t rc = ipc_connect_kbox(id, &phone);
	if (rc != EOK) {
		errno = rc;
		free(sess);
		return NULL;
	}

	sess->iface = 0;
	sess->mgmt = EXCHANGE_ATOMIC;
	sess->phone = phone;
	sess->arg1 = 0;
	sess->arg2 = 0;
	sess->arg3 = 0;

	fibril_mutex_initialize(&sess->remote_state_mtx);
	sess->remote_state_data = NULL;

	list_initialize(&sess->exch_list);
	fibril_mutex_initialize(&sess->mutex);
	atomic_set(&sess->refcnt, 0);

	return sess;
}

static errno_t async_hangup_internal(cap_phone_handle_t phone)
{
	return ipc_hangup(phone);
}

/** Wrapper for ipc_hangup.
 *
 * @param sess Session to hung up.
 *
 * @return Zero on success or an error code.
 *
 */
errno_t async_hangup(async_sess_t *sess)
{
	async_exch_t *exch;

	assert(sess);

	if (atomic_get(&sess->refcnt) > 0)
		return EBUSY;

	fibril_mutex_lock(&async_sess_mutex);

	errno_t rc = async_hangup_internal(sess->phone);

	while (!list_empty(&sess->exch_list)) {
		exch = (async_exch_t *)
		    list_get_instance(list_first(&sess->exch_list),
		    async_exch_t, sess_link);

		list_remove(&exch->sess_link);
		list_remove(&exch->global_link);
		async_hangup_internal(exch->phone);
		free(exch);
	}

	free(sess);

	fibril_mutex_unlock(&async_sess_mutex);

	return rc;
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

	fibril_mutex_unlock(&async_sess_mutex);

	if (exch != NULL) {
		atomic_inc(&sess->refcnt);

		if (mgmt == EXCHANGE_SERIALIZE)
			fibril_mutex_lock(&sess->mutex);
	}

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

	atomic_dec(&sess->refcnt);

	if (mgmt == EXCHANGE_SERIALIZE)
		fibril_mutex_unlock(&sess->mutex);

	fibril_mutex_lock(&async_sess_mutex);

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
errno_t async_share_in_start(async_exch_t *exch, size_t size, sysarg_t arg,
    unsigned int *flags, void **dst)
{
	if (exch == NULL)
		return ENOENT;

	sysarg_t _flags = 0;
	sysarg_t _dst = (sysarg_t) -1;
	errno_t res = async_req_2_4(exch, IPC_M_SHARE_IN, (sysarg_t) size,
	    arg, NULL, &_flags, NULL, &_dst);

	if (flags)
		*flags = (unsigned int) _flags;

	*dst = (void *) _dst;
	return res;
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
	    arg1, arg2, arg3, 0, CAP_HANDLE_RAW(other_exch->phone));
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

void async_call_begin(async_call_t *call, async_sess_t *sess, sysarg_t imethod,
    sysarg_t arg1, sysarg_t arg2, sysarg_t arg3, sysarg_t arg4)
{
	memset(call, 0, sizeof(*call));

	call->exch = async_exchange_begin(sess);
	if (!call->exch) {
		call->rc = ENOMEM;
		return;
	}

	async_call_method(call, &call->initial, imethod,
	    arg1, arg2, arg3, arg4);
}

static suseconds_t time_until(const struct timeval *t)
{
	struct timeval tv;
	getuptime(&tv);
	if (tv_gteq(&tv, t))
		return 1;

	return tv_sub_diff(t, &tv);
}

static errno_t async_call_finish_internal(async_call_t *call, const struct timeval *expires)
{
	errno_t rc;

	/* Wait for all the fragments. */
	while (!list_empty(&call->fragments)) {
		link_t *tmp = list_first(&call->fragments);
		async_call_data_t *frag =
		    list_get_instance(tmp, async_call_data_t, link);

		if (expires) {
			errno_t trc = async_wait_timeout(frag->msgid, &rc,
			    time_until(expires));
			if (trc != EOK)
				return trc;
		} else {
			async_wait_for(frag->msgid, &rc);
		}

		list_remove(tmp);

		if (rc != EOK)
			return rc;

		if (frag->finalizer) {
			rc = frag->finalizer(frag);
			if (rc != EOK)
				return rc;
		}
	}

	return EOK;
}

errno_t async_call_finish_timeout(async_call_t *call, const struct timeval *expires)
{
	assert(call);

	if (call->rc)
		return call->rc;

	if (call->exch) {
		async_exchange_end(call->exch);
		call->exch = NULL;
	}

	errno_t rc = async_call_finish_internal(call, expires);
	if (rc == ETIMEOUT)
		return rc;

	/* If one fails, abort the call. */
	if (rc != EOK)
		async_call_abort(call);

	assert(list_empty(&call->fragments));
	call->rc = rc;
	return rc;
}

// Ends the call, and waits for all in-flight fragments to finish.
errno_t async_call_finish(async_call_t *call)
{
	return async_call_finish_timeout(call, NULL);
}

// Aborts the call. After this function returns, auxiliary structures
// and buffers are safe to deallocate.
extern void async_call_abort(async_call_t *call)
{
	// FIXME: Proper abort needs kernel support. A system call should
	//        clean up bookkeeping structures in the kernel and notify
	//        the server of the abort as well.

	//        Currently, we just wait, which is less than ideal,
	//        but at the same time, nothing in HelenOS currently
	//        benefits from timeouts.

	assert(call);

	if (call->exch) {
		async_exchange_end(call->exch);
		call->exch = NULL;
	}

	/* Wait for all the fragments. */
	while (!list_empty(&call->fragments)) {
		// TODO: abort instead of waiting
		(void) async_call_finish_internal(call, NULL);
	}

	assert(list_empty(&call->fragments));
}

// Waits for all in-flight fragments to finish, but doesn't end the call.
errno_t async_call_wait(async_call_t *call)
{
	return async_call_wait_timeout(call, NULL);
}

errno_t async_call_wait_timeout(async_call_t *call, const struct timeval *expires)
{
	assert(call);

	if (call->rc)
		return call->rc;

	/* Wait for all the fragments except the initial one. */
	assert(list_first(&call->fragments) == &call->initial.link);
	list_remove(&call->initial.link);

	errno_t rc = async_call_finish_internal(call, expires);
	list_prepend(&call->initial.link, &call->fragments);

	if (rc == ETIMEOUT)
		return rc;

	/* If one fails, abort the call. */
	if (rc != EOK)
		async_call_abort(call);

	call->rc = rc;
	return rc;
}

void async_call_method_with_finalizer(async_call_t *call,
    async_call_data_t *data, sysarg_t imethod, sysarg_t arg1, sysarg_t arg2,
    sysarg_t arg3, sysarg_t arg4, async_call_finalizer_t finalizer)
{
	assert(call);
	assert(data);
	data->finalizer = finalizer;

	if (!call->exch)
		call->rc = ENOENT;

	if (call->rc)
		return;

	data->msgid = async_send_fast(call->exch, imethod,
	    arg1, arg2, arg3, arg4, &data->answer);
	if (!data->msgid) {
		async_call_abort(call);
		call->rc = ENOMEM;
	}

	list_append(&data->link, &call->fragments);
}

void async_call_method(async_call_t *call, async_call_data_t *data,
    sysarg_t imethod, sysarg_t arg1, sysarg_t arg2, sysarg_t arg3, sysarg_t arg4)
{
	assert(call);
	assert(data);
	memset(data, 0, sizeof(*data));

	async_call_method_with_finalizer(call, data, imethod,
	    arg1, arg2, arg3, arg4, NULL);
}

static errno_t call_read_write_finalizer(async_call_data_t *data)
{
	size_t *sz = data->arg1;
	if (sz)
		*sz = IPC_GET_ARG2(data->answer);
	return EOK;
}

void async_call_read(async_call_t *call, async_call_data_t *data,
    void *dst, size_t size, size_t *nread)
{
	assert(call);
	assert(data);
	memset(data, 0, sizeof(*data));

	data->arg1 = nread;

	async_call_method_with_finalizer(call, data,
	    IPC_M_DATA_READ, (sysarg_t) dst, (sysarg_t) size, 0, 0,
	    call_read_write_finalizer);
}

/**
 * After the call is successfully finished,
 * `IPC_GET_ARG2(&data->answer)` holds the actual number of bytes written.
 */
void async_call_write(async_call_t *call, async_call_data_t *data,
    const void *src, size_t size, size_t *nwritten)
{
	assert(call);
	assert(data);
	memset(data, 0, sizeof(*data));

	data->arg1 = nwritten;

	async_call_method_with_finalizer(call, data,
	    IPC_M_DATA_WRITE, (sysarg_t) src, (sysarg_t) size, 0, 0,
	    call_read_write_finalizer);
}

static errno_t call_share_in_finalizer(async_call_data_t *data)
{
	unsigned int *flags = data->arg1;
	void **dst = data->arg2;

	if (flags)
		*flags = IPC_GET_ARG2(data->answer);

	if (dst)
		*dst = (void *) IPC_GET_ARG4(data->answer);

	return EOK;
}

void async_call_share_in(async_call_t *call, async_call_data_t *data,
    size_t size, sysarg_t arg, unsigned int *flags, void **dst)
{
	assert(call);
	assert(data);
	memset(data, 0, sizeof(*data));

	data->arg1 = flags;
	data->arg2 = dst;

	async_call_method_with_finalizer(call, data,
	    IPC_M_SHARE_IN, (sysarg_t) size, (sysarg_t) arg, 0, 0,
	    call_share_in_finalizer);
}

void async_call_share_out(async_call_t *call, async_call_data_t *data,
    void *src, unsigned int flags)
{
	async_call_method(call, data,
	    IPC_M_SHARE_OUT, (sysarg_t) src, 0, (sysarg_t) flags, 0);
}

// TODO: connect me to, connect to me, vfs handle, etc.

/** @}
 */
