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
 *   my_client_connection(icallid, *icall)
 *   {
 *     if (want_refuse) {
 *       async_answer_0(icallid, ELIMIT);
 *       return;
 *     }
 *     async_answer_0(icallid, EOK);
 *
 *     callid = async_get_call(&call);
 *     somehow_handle_the_call(callid, call);
 *     async_answer_2(callid, 1, 2, 3);
 *
 *     callid = async_get_call(&call);
 *     ...
 *   }
 *
 */

#define LIBC_ASYNC_C_
#include <ipc/ipc.h>
#include <async.h>
#include "private/async.h"
#undef LIBC_ASYNC_C_

#include <ipc/irq.h>
#include <ipc/event.h>
#include <futex.h>
#include <fibril.h>
#include <adt/hash_table.h>
#include <adt/list.h>
#include <assert.h>
#include <errno.h>
#include <sys/time.h>
#include <libarch/barrier.h>
#include <stdbool.h>
#include <malloc.h>
#include <mem.h>
#include <stdlib.h>
#include <macros.h>
#include "private/libc.h"

/** Session data */
struct async_sess {
	/** List of inactive exchanges */
	list_t exch_list;
	
	/** Exchange management style */
	exch_mgmt_t mgmt;
	
	/** Session identification */
	int phone;
	
	/** First clone connection argument */
	sysarg_t arg1;
	
	/** Second clone connection argument */
	sysarg_t arg2;
	
	/** Third clone connection argument */
	sysarg_t arg3;
	
	/** Exchange mutex */
	fibril_mutex_t mutex;
	
	/** Number of opened exchanges */
	atomic_t refcnt;
	
	/** Mutex for stateful connections */
	fibril_mutex_t remote_state_mtx;
	
	/** Data for stateful connections */
	void *remote_state_data;
};

/** Exchange data */
struct async_exch {
	/** Link into list of inactive exchanges */
	link_t sess_link;
	
	/** Link into global list of inactive exchanges */
	link_t global_link;
	
	/** Session pointer */
	async_sess_t *sess;
	
	/** Exchange identification */
	int phone;
};

/** Async framework global futex */
atomic_t async_futex = FUTEX_INITIALIZER;

/** Number of threads waiting for IPC in the kernel. */
atomic_t threads_in_ipc_wait = { 0 };

/** Naming service session */
async_sess_t *session_ns;

/** Call data */
typedef struct {
	link_t link;
	
	ipc_callid_t callid;
	ipc_call_t call;
} msg_t;

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
	
	sysarg_t retval;
} amsg_t;

/* Client connection data */
typedef struct {
	ht_link_t link;
	
	task_id_t in_task_id;
	atomic_t refcnt;
	void *data;
} client_t;

/* Server connection data */
typedef struct {
	awaiter_t wdata;
	
	/** Hash table link. */
	ht_link_t link;
	
	/** Incoming client task ID. */
	task_id_t in_task_id;
	
	/** Incoming phone hash. */
	sysarg_t in_phone_hash;
	
	/** Link to the client tracking structure. */
	client_t *client;
	
	/** Messages that should be delivered to this fibril. */
	list_t msg_queue;
	
	/** Identification of the opening call. */
	ipc_callid_t callid;
	/** Call data of the opening call. */
	ipc_call_t call;
	/** Local argument or NULL if none. */
	void *carg;
	
	/** Identification of the closing call. */
	ipc_callid_t close_callid;
	
	/** Fibril function that will be used to handle the connection. */
	async_client_conn_t cfibril;
} connection_t;

/* Notification data */
typedef struct {
	ht_link_t link;
	
	/** Notification method */
	sysarg_t imethod;
	
	/** Notification handler */
	async_notification_handler_t handler;
	
	/** Notification data */
	void *data;
} notification_t;

/** Identifier of the incoming connection handled by the current fibril. */
static fibril_local connection_t *fibril_connection;

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
	amsg_t *msg;

	msg = malloc(sizeof(amsg_t));
	if (msg) {
		msg->done = false;
		msg->forget = false;
		msg->destroyed = false;
		msg->dataptr = NULL;
		msg->retval = (sysarg_t) EINVAL;
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

static void *default_client_data_constructor(void)
{
	return NULL;
}

static void default_client_data_destructor(void *data)
{
}

static async_client_data_ctor_t async_client_data_create =
    default_client_data_constructor;
static async_client_data_dtor_t async_client_data_destroy =
    default_client_data_destructor;

void async_set_client_data_constructor(async_client_data_ctor_t ctor)
{
	assert(async_client_data_create == default_client_data_constructor);
	async_client_data_create = ctor;
}

void async_set_client_data_destructor(async_client_data_dtor_t dtor)
{
	assert(async_client_data_destroy == default_client_data_destructor);
	async_client_data_destroy = dtor;
}

/** Default fibril function that gets called to handle new connection.
 *
 * This function is defined as a weak symbol - to be redefined in user code.
 *
 * @param callid Hash of the incoming call.
 * @param call   Data of the incoming call.
 * @param arg    Local argument
 *
 */
static void default_client_connection(ipc_callid_t callid, ipc_call_t *call,
    void *arg)
{
	ipc_answer_0(callid, ENOENT);
}

static async_client_conn_t client_connection = default_client_connection;
static size_t notification_handler_stksz = FIBRIL_DFLT_STK_SIZE;

/** Setter for client_connection function pointer.
 *
 * @param conn Function that will implement a new connection fibril.
 *
 */
void async_set_client_connection(async_client_conn_t conn)
{
	assert(client_connection == default_client_connection);
	client_connection = conn;
}

/** Set the stack size for the notification handler notification fibrils.
 *
 * @param size Stack size in bytes.
 */
void async_set_notification_handler_stack_size(size_t size)
{
	notification_handler_stksz = size;
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

static hash_table_t client_hash_table;
static hash_table_t conn_hash_table;
static hash_table_t notification_hash_table;
static LIST_INITIALIZE(timeout_list);

static sysarg_t notification_avail = 0;

static size_t client_key_hash(void *key)
{
	task_id_t in_task_id = *(task_id_t *) key;
	return in_task_id;
}

static size_t client_hash(const ht_link_t *item)
{
	client_t *client = hash_table_get_inst(item, client_t, link);
	return client_key_hash(&client->in_task_id);
}

static bool client_key_equal(void *key, const ht_link_t *item)
{
	task_id_t in_task_id = *(task_id_t *) key;
	client_t *client = hash_table_get_inst(item, client_t, link);
	return in_task_id == client->in_task_id;
}

/** Operations for the client hash table. */
static hash_table_ops_t client_hash_table_ops = {
	.hash = client_hash,
	.key_hash = client_key_hash,
	.key_equal = client_key_equal,
	.equal = NULL,
	.remove_callback = NULL
};

/** Compute hash into the connection hash table based on the source phone hash.
 *
 * @param key Pointer to source phone hash.
 *
 * @return Index into the connection hash table.
 *
 */
static size_t conn_key_hash(void *key)
{
	sysarg_t in_phone_hash = *(sysarg_t *) key;
	return in_phone_hash;
}

static size_t conn_hash(const ht_link_t *item)
{
	connection_t *conn = hash_table_get_inst(item, connection_t, link);
	return conn_key_hash(&conn->in_phone_hash);
}

static bool conn_key_equal(void *key, const ht_link_t *item)
{
	sysarg_t in_phone_hash = *(sysarg_t *) key;
	connection_t *conn = hash_table_get_inst(item, connection_t, link);
	return (in_phone_hash == conn->in_phone_hash);
}

/** Operations for the connection hash table. */
static hash_table_ops_t conn_hash_table_ops = {
	.hash = conn_hash,
	.key_hash = conn_key_hash,
	.key_equal = conn_key_equal,
	.equal = NULL,
	.remove_callback = NULL
};

static size_t notification_key_hash(void *key)
{
	sysarg_t id = *(sysarg_t *) key;
	return id;
}

static size_t notification_hash(const ht_link_t *item)
{
	notification_t *notification =
	    hash_table_get_inst(item, notification_t, link);
	return notification_key_hash(&notification->imethod);
}

static bool notification_key_equal(void *key, const ht_link_t *item)
{
	sysarg_t id = *(sysarg_t *) key;
	notification_t *notification =
	    hash_table_get_inst(item, notification_t, link);
	return id == notification->imethod;
}

/** Operations for the notification hash table. */
static hash_table_ops_t notification_hash_table_ops = {
	.hash = notification_hash,
	.key_hash = notification_key_hash,
	.key_equal = notification_key_equal,
	.equal = NULL,
	.remove_callback = NULL
};

/** Sort in current fibril's timeout request.
 *
 * @param wd Wait data of the current fibril.
 *
 */
void async_insert_timeout(awaiter_t *wd)
{
	assert(wd);
	
	wd->to_event.occurred = false;
	wd->to_event.inlist = true;
	
	link_t *tmp = timeout_list.head.next;
	while (tmp != &timeout_list.head) {
		awaiter_t *cur
		    = list_get_instance(tmp, awaiter_t, to_event.link);
		
		if (tv_gteq(&cur->to_event.expires, &wd->to_event.expires))
			break;
		
		tmp = tmp->next;
	}
	
	list_insert_before(&wd->to_event.link, tmp);
}

/** Try to route a call to an appropriate connection fibril.
 *
 * If the proper connection fibril is found, a message with the call is added to
 * its message queue. If the fibril was not active, it is activated and all
 * timeouts are unregistered.
 *
 * @param callid Hash of the incoming call.
 * @param call   Data of the incoming call.
 *
 * @return False if the call doesn't match any connection.
 * @return True if the call was passed to the respective connection fibril.
 *
 */
static bool route_call(ipc_callid_t callid, ipc_call_t *call)
{
	assert(call);
	
	futex_down(&async_futex);
	
	ht_link_t *link = hash_table_find(&conn_hash_table, &call->in_phone_hash);
	if (!link) {
		futex_up(&async_futex);
		return false;
	}
	
	connection_t *conn = hash_table_get_inst(link, connection_t, link);
	
	msg_t *msg = malloc(sizeof(*msg));
	if (!msg) {
		futex_up(&async_futex);
		return false;
	}
	
	msg->callid = callid;
	msg->call = *call;
	list_append(&msg->link, &conn->msg_queue);
	
	if (IPC_GET_IMETHOD(*call) == IPC_M_PHONE_HUNGUP)
		conn->close_callid = callid;
	
	/* If the connection fibril is waiting for an event, activate it */
	if (!conn->wdata.active) {
		
		/* If in timeout list, remove it */
		if (conn->wdata.to_event.inlist) {
			conn->wdata.to_event.inlist = false;
			list_remove(&conn->wdata.to_event.link);
		}
		
		conn->wdata.active = true;
		fibril_add_ready(conn->wdata.fid);
	}
	
	futex_up(&async_futex);
	return true;
}

/** Notification fibril.
 *
 * When a notification arrives, a fibril with this implementing function is
 * created. It calls the corresponding notification handler and does the final
 * cleanup.
 *
 * @param arg Message structure pointer.
 *
 * @return Always zero.
 *
 */
static int notification_fibril(void *arg)
{
	assert(arg);
	
	msg_t *msg = (msg_t *) arg;
	async_notification_handler_t handler = NULL;
	void *data = NULL;
	
	futex_down(&async_futex);
	
	ht_link_t *link = hash_table_find(&notification_hash_table,
	    &IPC_GET_IMETHOD(msg->call));
	if (link) {
		notification_t *notification =
		    hash_table_get_inst(link, notification_t, link);
		handler = notification->handler;
		data = notification->data;
	}
	
	futex_up(&async_futex);
	
	if (handler)
		handler(msg->callid, &msg->call, data);
	
	free(msg);
	return 0;
}

/** Process notification.
 *
 * A new fibril is created which would process the notification.
 *
 * @param callid Hash of the incoming call.
 * @param call   Data of the incoming call.
 *
 * @return False if an error occured.
 *         True if the call was passed to the notification fibril.
 *
 */
static bool process_notification(ipc_callid_t callid, ipc_call_t *call)
{
	assert(call);
	
	futex_down(&async_futex);
	
	msg_t *msg = malloc(sizeof(*msg));
	if (!msg) {
		futex_up(&async_futex);
		return false;
	}
	
	msg->callid = callid;
	msg->call = *call;
	
	fid_t fid = fibril_create_generic(notification_fibril, msg,
	    notification_handler_stksz);
	if (fid == 0) {
		free(msg);
		futex_up(&async_futex);
		return false;
	}
	
	fibril_add_ready(fid);
	
	futex_up(&async_futex);
	return true;
}

/** Subscribe to IRQ notification.
 *
 * @param inr     IRQ number.
 * @param devno   Device number of the device generating inr.
 * @param handler Notification handler.
 * @param data    Notification handler client data.
 * @param ucode   Top-half pseudocode handler.
 *
 * @return Zero on success or a negative error code.
 *
 */
int async_irq_subscribe(int inr, int devno,
    async_notification_handler_t handler, void *data, const irq_code_t *ucode)
{
	notification_t *notification =
	    (notification_t *) malloc(sizeof(notification_t));
	if (!notification)
		return ENOMEM;
	
	futex_down(&async_futex);
	
	sysarg_t imethod = notification_avail;
	notification_avail++;
	
	notification->imethod = imethod;
	notification->handler = handler;
	notification->data = data;
	
	hash_table_insert(&notification_hash_table, &notification->link);
	
	futex_up(&async_futex);
	
	return ipc_irq_subscribe(inr, devno, imethod, ucode);
}

/** Unsubscribe from IRQ notification.
 *
 * @param inr     IRQ number.
 * @param devno   Device number of the device generating inr.
 *
 * @return Zero on success or a negative error code.
 *
 */
int async_irq_unsubscribe(int inr, int devno)
{
	// TODO: Remove entry from hash table
	//       to avoid memory leak
	
	return ipc_irq_unsubscribe(inr, devno);
}

/** Subscribe to event notifications.
 *
 * @param evno    Event type to subscribe.
 * @param handler Notification handler.
 * @param data    Notification handler client data.
 *
 * @return Zero on success or a negative error code.
 *
 */
int async_event_subscribe(event_type_t evno,
    async_notification_handler_t handler, void *data)
{
	notification_t *notification =
	    (notification_t *) malloc(sizeof(notification_t));
	if (!notification)
		return ENOMEM;
	
	futex_down(&async_futex);
	
	sysarg_t imethod = notification_avail;
	notification_avail++;
	
	notification->imethod = imethod;
	notification->handler = handler;
	notification->data = data;
	
	hash_table_insert(&notification_hash_table, &notification->link);
	
	futex_up(&async_futex);
	
	return ipc_event_subscribe(evno, imethod);
}

/** Subscribe to task event notifications.
 *
 * @param evno    Event type to subscribe.
 * @param handler Notification handler.
 * @param data    Notification handler client data.
 *
 * @return Zero on success or a negative error code.
 *
 */
int async_event_task_subscribe(event_task_type_t evno,
    async_notification_handler_t handler, void *data)
{
	notification_t *notification =
	    (notification_t *) malloc(sizeof(notification_t));
	if (!notification)
		return ENOMEM;
	
	futex_down(&async_futex);
	
	sysarg_t imethod = notification_avail;
	notification_avail++;
	
	notification->imethod = imethod;
	notification->handler = handler;
	notification->data = data;
	
	hash_table_insert(&notification_hash_table, &notification->link);
	
	futex_up(&async_futex);
	
	return ipc_event_task_subscribe(evno, imethod);
}

/** Unmask event notifications.
 *
 * @param evno Event type to unmask.
 *
 * @return Value returned by the kernel.
 *
 */
int async_event_unmask(event_type_t evno)
{
	return ipc_event_unmask(evno);
}

/** Unmask task event notifications.
 *
 * @param evno Event type to unmask.
 *
 * @return Value returned by the kernel.
 *
 */
int async_event_task_unmask(event_task_type_t evno)
{
	return ipc_event_task_unmask(evno);
}

/** Return new incoming message for the current (fibril-local) connection.
 *
 * @param call  Storage where the incoming call data will be stored.
 * @param usecs Timeout in microseconds. Zero denotes no timeout.
 *
 * @return If no timeout was specified, then a hash of the
 *         incoming call is returned. If a timeout is specified,
 *         then a hash of the incoming call is returned unless
 *         the timeout expires prior to receiving a message. In
 *         that case zero is returned.
 *
 */
ipc_callid_t async_get_call_timeout(ipc_call_t *call, suseconds_t usecs)
{
	assert(call);
	assert(fibril_connection);
	
	/* Why doing this?
	 * GCC 4.1.0 coughs on fibril_connection-> dereference.
	 * GCC 4.1.1 happilly puts the rdhwr instruction in delay slot.
	 *           I would never expect to find so many errors in
	 *           a compiler.
	 */
	connection_t *conn = fibril_connection;
	
	futex_down(&async_futex);
	
	if (usecs) {
		getuptime(&conn->wdata.to_event.expires);
		tv_add(&conn->wdata.to_event.expires, usecs);
	} else
		conn->wdata.to_event.inlist = false;
	
	/* If nothing in queue, wait until something arrives */
	while (list_empty(&conn->msg_queue)) {
		if (conn->close_callid) {
			/*
			 * Handle the case when the connection was already
			 * closed by the client but the server did not notice
			 * the first IPC_M_PHONE_HUNGUP call and continues to
			 * call async_get_call_timeout(). Repeat
			 * IPC_M_PHONE_HUNGUP until the caller notices.
			 */
			memset(call, 0, sizeof(ipc_call_t));
			IPC_SET_IMETHOD(*call, IPC_M_PHONE_HUNGUP);
			futex_up(&async_futex);
			return conn->close_callid;
		}
		
		if (usecs)
			async_insert_timeout(&conn->wdata);
		
		conn->wdata.active = false;
		
		/*
		 * Note: the current fibril will be rescheduled either due to a
		 * timeout or due to an arriving message destined to it. In the
		 * former case, handle_expired_timeouts() and, in the latter
		 * case, route_call() will perform the wakeup.
		 */
		fibril_switch(FIBRIL_TO_MANAGER);
		
		/*
		 * Futex is up after getting back from async_manager.
		 * Get it again.
		 */
		futex_down(&async_futex);
		if ((usecs) && (conn->wdata.to_event.occurred)
		    && (list_empty(&conn->msg_queue))) {
			/* If we timed out -> exit */
			futex_up(&async_futex);
			return 0;
		}
	}
	
	msg_t *msg = list_get_instance(list_first(&conn->msg_queue), msg_t, link);
	list_remove(&msg->link);
	
	ipc_callid_t callid = msg->callid;
	*call = msg->call;
	free(msg);
	
	futex_up(&async_futex);
	return callid;
}

static client_t *async_client_get(task_id_t client_id, bool create)
{
	client_t *client = NULL;

	futex_down(&async_futex);
	ht_link_t *link = hash_table_find(&client_hash_table, &client_id);
	if (link) {
		client = hash_table_get_inst(link, client_t, link);
		atomic_inc(&client->refcnt);
	} else if (create) {
		client = malloc(sizeof(client_t));
		if (client) {
			client->in_task_id = client_id;
			client->data = async_client_data_create();
		
			atomic_set(&client->refcnt, 1);
			hash_table_insert(&client_hash_table, &client->link);
		}
	}

	futex_up(&async_futex);
	return client;
}

static void async_client_put(client_t *client)
{
	bool destroy;

	futex_down(&async_futex);
	
	if (atomic_predec(&client->refcnt) == 0) {
		hash_table_remove(&client_hash_table, &client->in_task_id);
		destroy = true;
	} else
		destroy = false;
	
	futex_up(&async_futex);
	
	if (destroy) {
		if (client->data)
			async_client_data_destroy(client->data);
		
		free(client);
	}
}

void *async_get_client_data(void)
{
	assert(fibril_connection);
	return fibril_connection->client->data;
}

void *async_get_client_data_by_id(task_id_t client_id)
{
	client_t *client = async_client_get(client_id, false);
	if (!client)
		return NULL;
	if (!client->data) {
		async_client_put(client);
		return NULL;
	}

	return client->data;
}

void async_put_client_data_by_id(task_id_t client_id)
{
	client_t *client = async_client_get(client_id, false);

	assert(client);
	assert(client->data);

	/* Drop the reference we got in async_get_client_data_by_hash(). */
	async_client_put(client);

	/* Drop our own reference we got at the beginning of this function. */
	async_client_put(client);
}

/** Wrapper for client connection fibril.
 *
 * When a new connection arrives, a fibril with this implementing function is
 * created. It calls client_connection() and does the final cleanup.
 *
 * @param arg Connection structure pointer.
 *
 * @return Always zero.
 *
 */
static int connection_fibril(void *arg)
{
	assert(arg);
	
	/*
	 * Setup fibril-local connection pointer.
	 */
	fibril_connection = (connection_t *) arg;
	
	/*
	 * Add our reference for the current connection in the client task
	 * tracking structure. If this is the first reference, create and
	 * hash in a new tracking structure.
	 */

	client_t *client = async_client_get(fibril_connection->in_task_id, true);
	if (!client) {
		ipc_answer_0(fibril_connection->callid, ENOMEM);
		return 0;
	}

	fibril_connection->client = client;
	
	/*
	 * Call the connection handler function.
	 */
	fibril_connection->cfibril(fibril_connection->callid,
	    &fibril_connection->call, fibril_connection->carg);
	
	/*
	 * Remove the reference for this client task connection.
	 */
	async_client_put(client);
	
	/*
	 * Remove myself from the connection hash table.
	 */
	futex_down(&async_futex);
	hash_table_remove(&conn_hash_table, &fibril_connection->in_phone_hash);
	futex_up(&async_futex);
	
	/*
	 * Answer all remaining messages with EHANGUP.
	 */
	while (!list_empty(&fibril_connection->msg_queue)) {
		msg_t *msg =
		    list_get_instance(list_first(&fibril_connection->msg_queue),
		    msg_t, link);
		
		list_remove(&msg->link);
		ipc_answer_0(msg->callid, EHANGUP);
		free(msg);
	}
	
	/*
	 * If the connection was hung-up, answer the last call,
	 * i.e. IPC_M_PHONE_HUNGUP.
	 */
	if (fibril_connection->close_callid)
		ipc_answer_0(fibril_connection->close_callid, EOK);
	
	free(fibril_connection);
	return 0;
}

/** Create a new fibril for a new connection.
 *
 * Create new fibril for connection, fill in connection structures and insert
 * it into the hash table, so that later we can easily do routing of messages to
 * particular fibrils.
 *
 * @param in_task_id    Identification of the incoming connection.
 * @param in_phone_hash Identification of the incoming connection.
 * @param callid        Hash of the opening IPC_M_CONNECT_ME_TO call.
 *                      If callid is zero, the connection was opened by
 *                      accepting the IPC_M_CONNECT_TO_ME call and this function
 *                      is called directly by the server.
 * @param call          Call data of the opening call.
 * @param cfibril       Fibril function that should be called upon opening the
 *                      connection.
 * @param carg          Extra argument to pass to the connection fibril
 *
 * @return New fibril id or NULL on failure.
 *
 */
fid_t async_new_connection(task_id_t in_task_id, sysarg_t in_phone_hash,
    ipc_callid_t callid, ipc_call_t *call,
    async_client_conn_t cfibril, void *carg)
{
	connection_t *conn = malloc(sizeof(*conn));
	if (!conn) {
		if (callid)
			ipc_answer_0(callid, ENOMEM);
		
		return (uintptr_t) NULL;
	}
	
	conn->in_task_id = in_task_id;
	conn->in_phone_hash = in_phone_hash;
	list_initialize(&conn->msg_queue);
	conn->callid = callid;
	conn->close_callid = 0;
	conn->carg = carg;
	
	if (call)
		conn->call = *call;
	
	/* We will activate the fibril ASAP */
	conn->wdata.active = true;
	conn->cfibril = cfibril;
	conn->wdata.fid = fibril_create(connection_fibril, conn);
	
	if (conn->wdata.fid == 0) {
		free(conn);
		
		if (callid)
			ipc_answer_0(callid, ENOMEM);
		
		return (uintptr_t) NULL;
	}
	
	/* Add connection to the connection hash table */
	
	futex_down(&async_futex);
	hash_table_insert(&conn_hash_table, &conn->link);
	futex_up(&async_futex);
	
	fibril_add_ready(conn->wdata.fid);
	
	return conn->wdata.fid;
}

/** Handle a call that was received.
 *
 * If the call has the IPC_M_CONNECT_ME_TO method, a new connection is created.
 * Otherwise the call is routed to its connection fibril.
 *
 * @param callid Hash of the incoming call.
 * @param call   Data of the incoming call.
 *
 */
static void handle_call(ipc_callid_t callid, ipc_call_t *call)
{
	assert(call);
	
	/* Unrouted call - take some default action */
	if ((callid & IPC_CALLID_NOTIFICATION)) {
		process_notification(callid, call);
		return;
	}
	
	switch (IPC_GET_IMETHOD(*call)) {
	case IPC_M_CLONE_ESTABLISH:
	case IPC_M_CONNECT_ME_TO:
		/* Open new connection with fibril, etc. */
		async_new_connection(call->in_task_id, IPC_GET_ARG5(*call),
		    callid, call, client_connection, NULL);
		return;
	}
	
	/* Try to route the call through the connection hash table */
	if (route_call(callid, call))
		return;
	
	/* Unknown call from unknown phone - hang it up */
	ipc_answer_0(callid, EHANGUP);
}

/** Fire all timeouts that expired. */
static void handle_expired_timeouts(void)
{
	struct timeval tv;
	getuptime(&tv);
	
	futex_down(&async_futex);
	
	link_t *cur = list_first(&timeout_list);
	while (cur != NULL) {
		awaiter_t *waiter =
		    list_get_instance(cur, awaiter_t, to_event.link);
		
		if (tv_gt(&waiter->to_event.expires, &tv))
			break;
		
		list_remove(&waiter->to_event.link);
		waiter->to_event.inlist = false;
		waiter->to_event.occurred = true;
		
		/*
		 * Redundant condition?
		 * The fibril should not be active when it gets here.
		 */
		if (!waiter->active) {
			waiter->active = true;
			fibril_add_ready(waiter->fid);
		}
		
		cur = list_first(&timeout_list);
	}
	
	futex_up(&async_futex);
}

/** Endless loop dispatching incoming calls and answers.
 *
 * @return Never returns.
 *
 */
static int async_manager_worker(void)
{
	while (true) {
		if (fibril_switch(FIBRIL_FROM_MANAGER)) {
			futex_up(&async_futex);
			/*
			 * async_futex is always held when entering a manager
			 * fibril.
			 */
			continue;
		}
		
		futex_down(&async_futex);
		
		suseconds_t timeout;
		unsigned int flags = SYNCH_FLAGS_NONE;
		if (!list_empty(&timeout_list)) {
			awaiter_t *waiter = list_get_instance(
			    list_first(&timeout_list), awaiter_t, to_event.link);
			
			struct timeval tv;
			getuptime(&tv);
			
			if (tv_gteq(&tv, &waiter->to_event.expires)) {
				futex_up(&async_futex);
				handle_expired_timeouts();
				/*
				 * Notice that even if the event(s) already
				 * expired (and thus the other fibril was
				 * supposed to be running already),
				 * we check for incoming IPC.
				 *
				 * Otherwise, a fibril that continuously
				 * creates (almost) expired events could
				 * prevent IPC retrieval from the kernel.
				 */
				timeout = 0;
				flags = SYNCH_FLAGS_NON_BLOCKING;

			} else {
				timeout = tv_sub(&waiter->to_event.expires, &tv);
				futex_up(&async_futex);
			}
		} else {
			futex_up(&async_futex);
			timeout = SYNCH_NO_TIMEOUT;
		}
		
		atomic_inc(&threads_in_ipc_wait);
		
		ipc_call_t call;
		ipc_callid_t callid = ipc_wait_cycle(&call, timeout, flags);
		
		atomic_dec(&threads_in_ipc_wait);
		
		if (!callid) {
			handle_expired_timeouts();
			continue;
		}
		
		if (callid & IPC_CALLID_ANSWERED)
			continue;
		
		handle_call(callid, &call);
	}
	
	return 0;
}

/** Function to start async_manager as a standalone fibril.
 *
 * When more kernel threads are used, one async manager should exist per thread.
 *
 * @param arg Unused.
 * @return Never returns.
 *
 */
static int async_manager_fibril(void *arg)
{
	futex_up(&async_futex);
	
	/*
	 * async_futex is always locked when entering manager
	 */
	async_manager_worker();
	
	return 0;
}

/** Add one manager to manager list. */
void async_create_manager(void)
{
	fid_t fid = fibril_create(async_manager_fibril, NULL);
	if (fid != 0)
		fibril_add_manager(fid);
}

/** Remove one manager from manager list */
void async_destroy_manager(void)
{
	fibril_remove_manager();
}

/** Initialize the async framework.
 *
 */
void __async_init(void)
{
	if (!hash_table_create(&client_hash_table, 0, 0, &client_hash_table_ops))
		abort();
	
	if (!hash_table_create(&conn_hash_table, 0, 0, &conn_hash_table_ops))
		abort();
	
	if (!hash_table_create(&notification_hash_table, 0, 0,
	    &notification_hash_table_ops))
		abort();
	
	session_ns = (async_sess_t *) malloc(sizeof(async_sess_t));
	if (session_ns == NULL)
		abort();
	
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
void reply_received(void *arg, int retval, ipc_call_t *data)
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
 * @param dataptr If non-NULL, storage where the reply data will be
 *                stored.
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
	    reply_received, true);
	
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
	    msg, reply_received, true);
	
	return (aid_t) msg;
}

/** Wait for a message sent by the async framework.
 *
 * @param amsgid Hash of the message to wait for.
 * @param retval Pointer to storage where the retval of the answer will
 *               be stored.
 *
 */
void async_wait_for(aid_t amsgid, sysarg_t *retval)
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
int async_wait_timeout(aid_t amsgid, sysarg_t *retval, suseconds_t timeout)
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
	tv_add(&msg->wdata.to_event.expires, timeout);
	
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
	amsg_t *msg = amsg_create();
	if (!msg)
		return;
	
	msg->wdata.fid = fibril_get_id();
	
	getuptime(&msg->wdata.to_event.expires);
	tv_add(&msg->wdata.to_event.expires, timeout);
	
	futex_down(&async_futex);
	
	async_insert_timeout(&msg->wdata);
	
	/* Leave the async_futex locked when entering this function */
	fibril_switch(FIBRIL_TO_MANAGER);
	
	/* Futex is up automatically after fibril_switch() */
	
	amsg_destroy(msg);
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
 * @return Return code of the reply or a negative error code.
 *
 */
sysarg_t async_req_fast(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t *r1, sysarg_t *r2,
    sysarg_t *r3, sysarg_t *r4, sysarg_t *r5)
{
	if (exch == NULL)
		return ENOENT;
	
	ipc_call_t result;
	aid_t aid = async_send_4(exch, imethod, arg1, arg2, arg3, arg4,
	    &result);
	
	sysarg_t rc;
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
 * @return Return code of the reply or a negative error code.
 *
 */
sysarg_t async_req_slow(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5, sysarg_t *r1,
    sysarg_t *r2, sysarg_t *r3, sysarg_t *r4, sysarg_t *r5)
{
	if (exch == NULL)
		return ENOENT;
	
	ipc_call_t result;
	aid_t aid = async_send_5(exch, imethod, arg1, arg2, arg3, arg4, arg5,
	    &result);
	
	sysarg_t rc;
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
		ipc_call_async_0(exch->phone, imethod, NULL, NULL, true);
}

void async_msg_1(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1)
{
	if (exch != NULL)
		ipc_call_async_1(exch->phone, imethod, arg1, NULL, NULL, true);
}

void async_msg_2(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2)
{
	if (exch != NULL)
		ipc_call_async_2(exch->phone, imethod, arg1, arg2, NULL, NULL,
		    true);
}

void async_msg_3(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3)
{
	if (exch != NULL)
		ipc_call_async_3(exch->phone, imethod, arg1, arg2, arg3, NULL,
		    NULL, true);
}

void async_msg_4(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4)
{
	if (exch != NULL)
		ipc_call_async_4(exch->phone, imethod, arg1, arg2, arg3, arg4,
		    NULL, NULL, true);
}

void async_msg_5(async_exch_t *exch, sysarg_t imethod, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5)
{
	if (exch != NULL)
		ipc_call_async_5(exch->phone, imethod, arg1, arg2, arg3, arg4,
		    arg5, NULL, NULL, true);
}

sysarg_t async_answer_0(ipc_callid_t callid, sysarg_t retval)
{
	return ipc_answer_0(callid, retval);
}

sysarg_t async_answer_1(ipc_callid_t callid, sysarg_t retval, sysarg_t arg1)
{
	return ipc_answer_1(callid, retval, arg1);
}

sysarg_t async_answer_2(ipc_callid_t callid, sysarg_t retval, sysarg_t arg1,
    sysarg_t arg2)
{
	return ipc_answer_2(callid, retval, arg1, arg2);
}

sysarg_t async_answer_3(ipc_callid_t callid, sysarg_t retval, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3)
{
	return ipc_answer_3(callid, retval, arg1, arg2, arg3);
}

sysarg_t async_answer_4(ipc_callid_t callid, sysarg_t retval, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4)
{
	return ipc_answer_4(callid, retval, arg1, arg2, arg3, arg4);
}

sysarg_t async_answer_5(ipc_callid_t callid, sysarg_t retval, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5)
{
	return ipc_answer_5(callid, retval, arg1, arg2, arg3, arg4, arg5);
}

int async_forward_fast(ipc_callid_t callid, async_exch_t *exch,
    sysarg_t imethod, sysarg_t arg1, sysarg_t arg2, unsigned int mode)
{
	if (exch == NULL)
		return ENOENT;
	
	return ipc_forward_fast(callid, exch->phone, imethod, arg1, arg2, mode);
}

int async_forward_slow(ipc_callid_t callid, async_exch_t *exch,
    sysarg_t imethod, sysarg_t arg1, sysarg_t arg2, sysarg_t arg3,
    sysarg_t arg4, sysarg_t arg5, unsigned int mode)
{
	if (exch == NULL)
		return ENOENT;
	
	return ipc_forward_slow(callid, exch->phone, imethod, arg1, arg2, arg3,
	    arg4, arg5, mode);
}

/** Wrapper for making IPC_M_CONNECT_TO_ME calls using the async framework.
 *
 * Ask through phone for a new connection to some service.
 *
 * @param exch            Exchange for sending the message.
 * @param arg1            User defined argument.
 * @param arg2            User defined argument.
 * @param arg3            User defined argument.
 * @param client_receiver Connection handing routine.
 *
 * @return Zero on success or a negative error code.
 *
 */
int async_connect_to_me(async_exch_t *exch, sysarg_t arg1, sysarg_t arg2,
    sysarg_t arg3, async_client_conn_t client_receiver, void *carg)
{
	if (exch == NULL)
		return ENOENT;
	
	sysarg_t phone_hash;
	sysarg_t rc;

	aid_t req;
	ipc_call_t answer;
	req = async_send_3(exch, IPC_M_CONNECT_TO_ME, arg1, arg2, arg3,
	    &answer);
	async_wait_for(req, &rc);
	if (rc != EOK)
		return (int) rc;

	phone_hash = IPC_GET_ARG5(answer);

	if (client_receiver != NULL)
		async_new_connection(answer.in_task_id, phone_hash, 0, NULL,
		    client_receiver, carg);
	
	return EOK;
}

/** Wrapper for making IPC_M_CLONE_ESTABLISH calls using the async framework.
 *
 * Ask for a cloned connection to some service.
 *
 * @param mgmt Exchange management style.
 * @param exch Exchange for sending the message.
 *
 * @return New session on success or NULL on error.
 *
 */
async_sess_t *async_clone_establish(exch_mgmt_t mgmt, async_exch_t *exch)
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
	
	ipc_call_t result;
	
	amsg_t *msg = amsg_create();
	if (!msg) {
		free(sess);
		errno = ENOMEM;
		return NULL;
	}
	
	msg->dataptr = &result;
	msg->wdata.active = true;
	
	ipc_call_async_0(exch->phone, IPC_M_CLONE_ESTABLISH, msg,
	    reply_received, true);
	
	sysarg_t rc;
	async_wait_for((aid_t) msg, &rc);
	
	if (rc != EOK) {
		errno = rc;
		free(sess);
		return NULL;
	}
	
	int phone = (int) IPC_GET_ARG5(result);
	
	if (phone < 0) {
		errno = phone;
		free(sess);
		return NULL;
	}
	
	sess->mgmt = mgmt;
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

static int async_connect_me_to_internal(int phone, sysarg_t arg1, sysarg_t arg2,
    sysarg_t arg3, sysarg_t arg4)
{
	ipc_call_t result;
	
	amsg_t *msg = amsg_create();
	if (!msg)
		return ENOENT;
	
	msg->dataptr = &result;
	msg->wdata.active = true;
	
	ipc_call_async_4(phone, IPC_M_CONNECT_ME_TO, arg1, arg2, arg3, arg4,
	    msg, reply_received, true);
	
	sysarg_t rc;
	async_wait_for((aid_t) msg, &rc);
	
	if (rc != EOK)
		return rc;
	
	return (int) IPC_GET_ARG5(result);
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
	
	int phone = async_connect_me_to_internal(exch->phone, arg1, arg2, arg3,
	    0);
	
	if (phone < 0) {
		errno = phone;
		free(sess);
		return NULL;
	}
	
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
	
	int phone = async_connect_me_to_internal(exch->phone, arg1, arg2, arg3,
	    IPC_FLAG_BLOCKING);
	
	if (phone < 0) {
		errno = phone;
		free(sess);
		return NULL;
	}
	
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
	
	int phone = ipc_connect_kbox(id);
	if (phone < 0) {
		errno = phone;
		free(sess);
		return NULL;
	}
	
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

static int async_hangup_internal(int phone)
{
	return ipc_hangup(phone);
}

/** Wrapper for ipc_hangup.
 *
 * @param sess Session to hung up.
 *
 * @return Zero on success or a negative error code.
 *
 */
int async_hangup(async_sess_t *sess)
{
	async_exch_t *exch;
	
	assert(sess);
	
	if (atomic_get(&sess->refcnt) > 0)
		return EBUSY;
	
	fibril_mutex_lock(&async_sess_mutex);
	
	int rc = async_hangup_internal(sess->phone);
	
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

/** Interrupt one thread of this task from waiting for IPC. */
void async_poke(void)
{
	ipc_poke();
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
	
	async_exch_t *exch;
	
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
		
		if ((sess->mgmt == EXCHANGE_ATOMIC) ||
		    (sess->mgmt == EXCHANGE_SERIALIZE)) {
			exch = (async_exch_t *) malloc(sizeof(async_exch_t));
			if (exch != NULL) {
				link_initialize(&exch->sess_link);
				link_initialize(&exch->global_link);
				exch->sess = sess;
				exch->phone = sess->phone;
			}
		} else {  /* EXCHANGE_PARALLEL */
			/*
			 * Make a one-time attempt to connect a new data phone.
			 */
			
			int phone;
			
retry:
			phone = async_connect_me_to_internal(sess->phone, sess->arg1,
			    sess->arg2, sess->arg3, 0);
			if (phone >= 0) {
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
		
		if (sess->mgmt == EXCHANGE_SERIALIZE)
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
	
	atomic_dec(&sess->refcnt);
	
	if (sess->mgmt == EXCHANGE_SERIALIZE)
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
 * @return Zero on success or a negative error code from errno.h.
 *
 */
int async_share_in_start(async_exch_t *exch, size_t size, sysarg_t arg,
    unsigned int *flags, void **dst)
{
	if (exch == NULL)
		return ENOENT;
	
	sysarg_t _flags = 0;
	sysarg_t _dst = (sysarg_t) -1;
	int res = async_req_2_4(exch, IPC_M_SHARE_IN, (sysarg_t) size,
	    arg, NULL, &_flags, NULL, &_dst);
	
	if (flags)
		*flags = (unsigned int) _flags;
	
	*dst = (void *) _dst;
	return res;
}

/** Wrapper for receiving the IPC_M_SHARE_IN calls using the async framework.
 *
 * This wrapper only makes it more comfortable to receive IPC_M_SHARE_IN
 * calls so that the user doesn't have to remember the meaning of each IPC
 * argument.
 *
 * So far, this wrapper is to be used from within a connection fibril.
 *
 * @param callid Storage for the hash of the IPC_M_SHARE_IN call.
 * @param size   Destination address space area size.
 *
 * @return True on success, false on failure.
 *
 */
bool async_share_in_receive(ipc_callid_t *callid, size_t *size)
{
	assert(callid);
	assert(size);
	
	ipc_call_t data;
	*callid = async_get_call(&data);
	
	if (IPC_GET_IMETHOD(data) != IPC_M_SHARE_IN)
		return false;
	
	*size = (size_t) IPC_GET_ARG1(data);
	return true;
}

/** Wrapper for answering the IPC_M_SHARE_IN calls using the async framework.
 *
 * This wrapper only makes it more comfortable to answer IPC_M_SHARE_IN
 * calls so that the user doesn't have to remember the meaning of each IPC
 * argument.
 *
 * @param callid Hash of the IPC_M_DATA_READ call to answer.
 * @param src    Source address space base.
 * @param flags  Flags to be used for sharing. Bits can be only cleared.
 *
 * @return Zero on success or a value from @ref errno.h on failure.
 *
 */
int async_share_in_finalize(ipc_callid_t callid, void *src, unsigned int flags)
{
	return ipc_answer_3(callid, EOK, (sysarg_t) src, (sysarg_t) flags,
	    (sysarg_t) __entry);
}

/** Wrapper for IPC_M_SHARE_OUT calls using the async framework.
 *
 * @param exch  Exchange for sending the message.
 * @param src   Source address space area base address.
 * @param flags Flags to be used for sharing. Bits can be only cleared.
 *
 * @return Zero on success or a negative error code from errno.h.
 *
 */
int async_share_out_start(async_exch_t *exch, void *src, unsigned int flags)
{
	if (exch == NULL)
		return ENOENT;
	
	return async_req_3_0(exch, IPC_M_SHARE_OUT, (sysarg_t) src, 0,
	    (sysarg_t) flags);
}

/** Wrapper for receiving the IPC_M_SHARE_OUT calls using the async framework.
 *
 * This wrapper only makes it more comfortable to receive IPC_M_SHARE_OUT
 * calls so that the user doesn't have to remember the meaning of each IPC
 * argument.
 *
 * So far, this wrapper is to be used from within a connection fibril.
 *
 * @param callid Storage for the hash of the IPC_M_SHARE_OUT call.
 * @param size   Storage for the source address space area size.
 * @param flags  Storage for the sharing flags.
 *
 * @return True on success, false on failure.
 *
 */
bool async_share_out_receive(ipc_callid_t *callid, size_t *size, unsigned int *flags)
{
	assert(callid);
	assert(size);
	assert(flags);
	
	ipc_call_t data;
	*callid = async_get_call(&data);
	
	if (IPC_GET_IMETHOD(data) != IPC_M_SHARE_OUT)
		return false;
	
	*size = (size_t) IPC_GET_ARG2(data);
	*flags = (unsigned int) IPC_GET_ARG3(data);
	return true;
}

/** Wrapper for answering the IPC_M_SHARE_OUT calls using the async framework.
 *
 * This wrapper only makes it more comfortable to answer IPC_M_SHARE_OUT
 * calls so that the user doesn't have to remember the meaning of each IPC
 * argument.
 *
 * @param callid Hash of the IPC_M_DATA_WRITE call to answer.
 * @param dst    Address of the storage for the destination address space area
 *               base address.
 *
 * @return Zero on success or a value from @ref errno.h on failure.
 *
 */
int async_share_out_finalize(ipc_callid_t callid, void **dst)
{
	return ipc_answer_2(callid, EOK, (sysarg_t) __entry, (sysarg_t) dst);
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
 * @return Zero on success or a negative error code from errno.h.
 *
 */
int async_data_read_start(async_exch_t *exch, void *dst, size_t size)
{
	if (exch == NULL)
		return ENOENT;
	
	return async_req_2_0(exch, IPC_M_DATA_READ, (sysarg_t) dst,
	    (sysarg_t) size);
}

/** Wrapper for receiving the IPC_M_DATA_READ calls using the async framework.
 *
 * This wrapper only makes it more comfortable to receive IPC_M_DATA_READ
 * calls so that the user doesn't have to remember the meaning of each IPC
 * argument.
 *
 * So far, this wrapper is to be used from within a connection fibril.
 *
 * @param callid Storage for the hash of the IPC_M_DATA_READ.
 * @param size   Storage for the maximum size. Can be NULL.
 *
 * @return True on success, false on failure.
 *
 */
bool async_data_read_receive(ipc_callid_t *callid, size_t *size)
{
	ipc_call_t data;
	return async_data_read_receive_call(callid, &data, size);
}

/** Wrapper for receiving the IPC_M_DATA_READ calls using the async framework.
 *
 * This wrapper only makes it more comfortable to receive IPC_M_DATA_READ
 * calls so that the user doesn't have to remember the meaning of each IPC
 * argument.
 *
 * So far, this wrapper is to be used from within a connection fibril.
 *
 * @param callid Storage for the hash of the IPC_M_DATA_READ.
 * @param size   Storage for the maximum size. Can be NULL.
 *
 * @return True on success, false on failure.
 *
 */
bool async_data_read_receive_call(ipc_callid_t *callid, ipc_call_t *data,
    size_t *size)
{
	assert(callid);
	assert(data);
	
	*callid = async_get_call(data);
	
	if (IPC_GET_IMETHOD(*data) != IPC_M_DATA_READ)
		return false;
	
	if (size)
		*size = (size_t) IPC_GET_ARG2(*data);
	
	return true;
}

/** Wrapper for answering the IPC_M_DATA_READ calls using the async framework.
 *
 * This wrapper only makes it more comfortable to answer IPC_M_DATA_READ
 * calls so that the user doesn't have to remember the meaning of each IPC
 * argument.
 *
 * @param callid Hash of the IPC_M_DATA_READ call to answer.
 * @param src    Source address for the IPC_M_DATA_READ call.
 * @param size   Size for the IPC_M_DATA_READ call. Can be smaller than
 *               the maximum size announced by the sender.
 *
 * @return Zero on success or a value from @ref errno.h on failure.
 *
 */
int async_data_read_finalize(ipc_callid_t callid, const void *src, size_t size)
{
	return ipc_answer_2(callid, EOK, (sysarg_t) src, (sysarg_t) size);
}

/** Wrapper for forwarding any read request
 *
 */
int async_data_read_forward_fast(async_exch_t *exch, sysarg_t imethod,
    sysarg_t arg1, sysarg_t arg2, sysarg_t arg3, sysarg_t arg4,
    ipc_call_t *dataptr)
{
	if (exch == NULL)
		return ENOENT;
	
	ipc_callid_t callid;
	if (!async_data_read_receive(&callid, NULL)) {
		ipc_answer_0(callid, EINVAL);
		return EINVAL;
	}
	
	aid_t msg = async_send_fast(exch, imethod, arg1, arg2, arg3, arg4,
	    dataptr);
	if (msg == 0) {
		ipc_answer_0(callid, EINVAL);
		return EINVAL;
	}
	
	int retval = ipc_forward_fast(callid, exch->phone, 0, 0, 0,
	    IPC_FF_ROUTE_FROM_ME);
	if (retval != EOK) {
		async_forget(msg);
		ipc_answer_0(callid, retval);
		return retval;
	}
	
	sysarg_t rc;
	async_wait_for(msg, &rc);
	
	return (int) rc;
}

/** Wrapper for IPC_M_DATA_WRITE calls using the async framework.
 *
 * @param exch Exchange for sending the message.
 * @param src  Address of the beginning of the source buffer.
 * @param size Size of the source buffer.
 *
 * @return Zero on success or a negative error code from errno.h.
 *
 */
int async_data_write_start(async_exch_t *exch, const void *src, size_t size)
{
	if (exch == NULL)
		return ENOENT;
	
	return async_req_2_0(exch, IPC_M_DATA_WRITE, (sysarg_t) src,
	    (sysarg_t) size);
}

/** Wrapper for receiving the IPC_M_DATA_WRITE calls using the async framework.
 *
 * This wrapper only makes it more comfortable to receive IPC_M_DATA_WRITE
 * calls so that the user doesn't have to remember the meaning of each IPC
 * argument.
 *
 * So far, this wrapper is to be used from within a connection fibril.
 *
 * @param callid Storage for the hash of the IPC_M_DATA_WRITE.
 * @param size   Storage for the suggested size. May be NULL.
 *
 * @return True on success, false on failure.
 *
 */
bool async_data_write_receive(ipc_callid_t *callid, size_t *size)
{
	ipc_call_t data;
	return async_data_write_receive_call(callid, &data, size);
}

/** Wrapper for receiving the IPC_M_DATA_WRITE calls using the async framework.
 *
 * This wrapper only makes it more comfortable to receive IPC_M_DATA_WRITE
 * calls so that the user doesn't have to remember the meaning of each IPC
 * argument.
 *
 * So far, this wrapper is to be used from within a connection fibril.
 *
 * @param callid Storage for the hash of the IPC_M_DATA_WRITE.
 * @param data   Storage for the ipc call data.
 * @param size   Storage for the suggested size. May be NULL.
 *
 * @return True on success, false on failure.
 *
 */
bool async_data_write_receive_call(ipc_callid_t *callid, ipc_call_t *data,
    size_t *size)
{
	assert(callid);
	assert(data);
	
	*callid = async_get_call(data);
	
	if (IPC_GET_IMETHOD(*data) != IPC_M_DATA_WRITE)
		return false;
	
	if (size)
		*size = (size_t) IPC_GET_ARG2(*data);
	
	return true;
}

/** Wrapper for answering the IPC_M_DATA_WRITE calls using the async framework.
 *
 * This wrapper only makes it more comfortable to answer IPC_M_DATA_WRITE
 * calls so that the user doesn't have to remember the meaning of each IPC
 * argument.
 *
 * @param callid Hash of the IPC_M_DATA_WRITE call to answer.
 * @param dst    Final destination address for the IPC_M_DATA_WRITE call.
 * @param size   Final size for the IPC_M_DATA_WRITE call.
 *
 * @return Zero on success or a value from @ref errno.h on failure.
 *
 */
int async_data_write_finalize(ipc_callid_t callid, void *dst, size_t size)
{
	return ipc_answer_2(callid, EOK, (sysarg_t) dst, (sysarg_t) size);
}

/** Wrapper for receiving binary data or strings
 *
 * This wrapper only makes it more comfortable to use async_data_write_*
 * functions to receive binary data or strings.
 *
 * @param data       Pointer to data pointer (which should be later disposed
 *                   by free()). If the operation fails, the pointer is not
 *                   touched.
 * @param nullterm   If true then the received data is always zero terminated.
 *                   This also causes to allocate one extra byte beyond the
 *                   raw transmitted data.
 * @param min_size   Minimum size (in bytes) of the data to receive.
 * @param max_size   Maximum size (in bytes) of the data to receive. 0 means
 *                   no limit.
 * @param granulariy If non-zero then the size of the received data has to
 *                   be divisible by this value.
 * @param received   If not NULL, the size of the received data is stored here.
 *
 * @return Zero on success or a value from @ref errno.h on failure.
 *
 */
int async_data_write_accept(void **data, const bool nullterm,
    const size_t min_size, const size_t max_size, const size_t granularity,
    size_t *received)
{
	assert(data);
	
	ipc_callid_t callid;
	size_t size;
	if (!async_data_write_receive(&callid, &size)) {
		ipc_answer_0(callid, EINVAL);
		return EINVAL;
	}
	
	if (size < min_size) {
		ipc_answer_0(callid, EINVAL);
		return EINVAL;
	}
	
	if ((max_size > 0) && (size > max_size)) {
		ipc_answer_0(callid, EINVAL);
		return EINVAL;
	}
	
	if ((granularity > 0) && ((size % granularity) != 0)) {
		ipc_answer_0(callid, EINVAL);
		return EINVAL;
	}
	
	void *_data;
	
	if (nullterm)
		_data = malloc(size + 1);
	else
		_data = malloc(size);
	
	if (_data == NULL) {
		ipc_answer_0(callid, ENOMEM);
		return ENOMEM;
	}
	
	int rc = async_data_write_finalize(callid, _data, size);
	if (rc != EOK) {
		free(_data);
		return rc;
	}
	
	if (nullterm)
		((char *) _data)[size] = 0;
	
	*data = _data;
	if (received != NULL)
		*received = size;
	
	return EOK;
}

/** Wrapper for voiding any data that is about to be received
 *
 * This wrapper can be used to void any pending data
 *
 * @param retval Error value from @ref errno.h to be returned to the caller.
 *
 */
void async_data_write_void(sysarg_t retval)
{
	ipc_callid_t callid;
	async_data_write_receive(&callid, NULL);
	ipc_answer_0(callid, retval);
}

/** Wrapper for forwarding any data that is about to be received
 *
 */
int async_data_write_forward_fast(async_exch_t *exch, sysarg_t imethod,
    sysarg_t arg1, sysarg_t arg2, sysarg_t arg3, sysarg_t arg4,
    ipc_call_t *dataptr)
{
	if (exch == NULL)
		return ENOENT;
	
	ipc_callid_t callid;
	if (!async_data_write_receive(&callid, NULL)) {
		ipc_answer_0(callid, EINVAL);
		return EINVAL;
	}
	
	aid_t msg = async_send_fast(exch, imethod, arg1, arg2, arg3, arg4,
	    dataptr);
	if (msg == 0) {
		ipc_answer_0(callid, EINVAL);
		return EINVAL;
	}
	
	int retval = ipc_forward_fast(callid, exch->phone, 0, 0, 0,
	    IPC_FF_ROUTE_FROM_ME);
	if (retval != EOK) {
		async_forget(msg);
		ipc_answer_0(callid, retval);
		return retval;
	}
	
	sysarg_t rc;
	async_wait_for(msg, &rc);
	
	return (int) rc;
}

/** Wrapper for sending an exchange over different exchange for cloning
 *
 * @param exch       Exchange to be used for sending.
 * @param clone_exch Exchange to be cloned.
 *
 */
int async_exchange_clone(async_exch_t *exch, async_exch_t *clone_exch)
{
	return async_req_1_0(exch, IPC_M_CONNECTION_CLONE, clone_exch->phone);
}

/** Wrapper for receiving the IPC_M_CONNECTION_CLONE calls.
 *
 * If the current call is IPC_M_CONNECTION_CLONE then a new
 * async session is created for the accepted phone.
 *
 * @param mgmt Exchange management style.
 *
 * @return New async session or NULL on failure.
 *
 */
async_sess_t *async_clone_receive(exch_mgmt_t mgmt)
{
	/* Accept the phone */
	ipc_call_t call;
	ipc_callid_t callid = async_get_call(&call);
	int phone = (int) IPC_GET_ARG1(call);
	
	if ((IPC_GET_IMETHOD(call) != IPC_M_CONNECTION_CLONE) ||
	    (phone < 0)) {
		async_answer_0(callid, EINVAL);
		return NULL;
	}
	
	async_sess_t *sess = (async_sess_t *) malloc(sizeof(async_sess_t));
	if (sess == NULL) {
		async_answer_0(callid, ENOMEM);
		return NULL;
	}
	
	sess->mgmt = mgmt;
	sess->phone = phone;
	sess->arg1 = 0;
	sess->arg2 = 0;
	sess->arg3 = 0;
	
	fibril_mutex_initialize(&sess->remote_state_mtx);
	sess->remote_state_data = NULL;
	
	list_initialize(&sess->exch_list);
	fibril_mutex_initialize(&sess->mutex);
	atomic_set(&sess->refcnt, 0);
	
	/* Acknowledge the cloned phone */
	async_answer_0(callid, EOK);
	
	return sess;
}

/** Wrapper for receiving the IPC_M_CONNECT_TO_ME calls.
 *
 * If the current call is IPC_M_CONNECT_TO_ME then a new
 * async session is created for the accepted phone.
 *
 * @param mgmt Exchange management style.
 *
 * @return New async session.
 * @return NULL on failure.
 *
 */
async_sess_t *async_callback_receive(exch_mgmt_t mgmt)
{
	/* Accept the phone */
	ipc_call_t call;
	ipc_callid_t callid = async_get_call(&call);
	int phone = (int) IPC_GET_ARG5(call);
	
	if ((IPC_GET_IMETHOD(call) != IPC_M_CONNECT_TO_ME) ||
	    (phone < 0)) {
		async_answer_0(callid, EINVAL);
		return NULL;
	}
	
	async_sess_t *sess = (async_sess_t *) malloc(sizeof(async_sess_t));
	if (sess == NULL) {
		async_answer_0(callid, ENOMEM);
		return NULL;
	}
	
	sess->mgmt = mgmt;
	sess->phone = phone;
	sess->arg1 = 0;
	sess->arg2 = 0;
	sess->arg3 = 0;
	
	fibril_mutex_initialize(&sess->remote_state_mtx);
	sess->remote_state_data = NULL;
	
	list_initialize(&sess->exch_list);
	fibril_mutex_initialize(&sess->mutex);
	atomic_set(&sess->refcnt, 0);
	
	/* Acknowledge the connected phone */
	async_answer_0(callid, EOK);
	
	return sess;
}

/** Wrapper for receiving the IPC_M_CONNECT_TO_ME calls.
 *
 * If the call is IPC_M_CONNECT_TO_ME then a new
 * async session is created. However, the phone is
 * not accepted automatically.
 *
 * @param mgmt   Exchange management style.
 * @param call   Call data.
 *
 * @return New async session.
 * @return NULL on failure.
 * @return NULL if the call is not IPC_M_CONNECT_TO_ME.
 *
 */
async_sess_t *async_callback_receive_start(exch_mgmt_t mgmt, ipc_call_t *call)
{
	int phone = (int) IPC_GET_ARG5(*call);
	
	if ((IPC_GET_IMETHOD(*call) != IPC_M_CONNECT_TO_ME) ||
	    (phone < 0))
		return NULL;
	
	async_sess_t *sess = (async_sess_t *) malloc(sizeof(async_sess_t));
	if (sess == NULL)
		return NULL;
	
	sess->mgmt = mgmt;
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

int async_state_change_start(async_exch_t *exch, sysarg_t arg1, sysarg_t arg2,
    sysarg_t arg3, async_exch_t *other_exch)
{
	return async_req_5_0(exch, IPC_M_STATE_CHANGE_AUTHORIZE,
	    arg1, arg2, arg3, 0, other_exch->phone);
}

bool async_state_change_receive(ipc_callid_t *callid, sysarg_t *arg1,
    sysarg_t *arg2, sysarg_t *arg3)
{
	assert(callid);

	ipc_call_t call;
	*callid = async_get_call(&call);

	if (IPC_GET_IMETHOD(call) != IPC_M_STATE_CHANGE_AUTHORIZE)
		return false;
	
	if (arg1)
		*arg1 = IPC_GET_ARG1(call);
	if (arg2)
		*arg2 = IPC_GET_ARG2(call);
	if (arg3)
		*arg3 = IPC_GET_ARG3(call);

	return true;
}

int async_state_change_finalize(ipc_callid_t callid, async_exch_t *other_exch)
{
	return ipc_answer_1(callid, EOK, other_exch->phone);
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

/** @}
 */
