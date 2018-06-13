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

/** Async framework global futex */
futex_t async_futex = FUTEX_INITIALIZER;

/** Number of threads waiting for IPC in the kernel. */
static atomic_t threads_in_ipc_wait = { 0 };

/** Call data */
typedef struct {
	link_t link;

	cap_call_handle_t chandle;
	ipc_call_t call;
} msg_t;

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
	cap_call_handle_t chandle;

	/** Call data of the opening call. */
	ipc_call_t call;

	/** Identification of the closing call. */
	cap_call_handle_t close_chandle;

	/** Fibril function that will be used to handle the connection. */
	async_port_handler_t handler;

	/** Client data */
	void *data;
} connection_t;

/* Notification data */
typedef struct {
	/** notification_hash_table link */
	ht_link_t htlink;

	/** notification_queue link */
	link_t qlink;

	/** Notification method */
	sysarg_t imethod;

	/** Notification handler */
	async_notification_handler_t handler;

	/** Notification handler argument */
	void *arg;

	/** Data of the most recent notification. */
	ipc_call_t calldata;

	/**
	 * How many notifications with this `imethod` arrived since it was last
	 * handled. If `count` > 1, `calldata` only holds the data for the most
	 * recent such notification, all the older data being lost.
	 *
	 * `async_spawn_notification_handler()` can be used to increase the
	 * number of notifications that can be processed simultaneously,
	 * reducing the likelihood of losing them when the handler blocks.
	 */
	long count;
} notification_t;

/** Identifier of the incoming connection handled by the current fibril. */
static fibril_local connection_t *fibril_connection;

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

static hash_table_t client_hash_table;
static hash_table_t conn_hash_table;

// TODO: lockfree notification_queue?
static futex_t notification_futex = FUTEX_INITIALIZER;
static hash_table_t notification_hash_table;
static LIST_INITIALIZE(notification_queue);
static FIBRIL_SEMAPHORE_INITIALIZE(notification_semaphore, 0);

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

typedef struct {
	task_id_t task_id;
	sysarg_t phone_hash;
} conn_key_t;

/** Compute hash into the connection hash table
 *
 * The hash is based on the source task ID and the source phone hash. The task
 * ID is included in the hash because a phone hash alone might not be unique
 * while we still track connections for killed tasks due to kernel's recycling
 * of phone structures.
 *
 * @param key Pointer to the connection key structure.
 *
 * @return Index into the connection hash table.
 *
 */
static size_t conn_key_hash(void *key)
{
	conn_key_t *ck = (conn_key_t *) key;

	size_t hash = 0;
	hash = hash_combine(hash, LOWER32(ck->task_id));
	hash = hash_combine(hash, UPPER32(ck->task_id));
	hash = hash_combine(hash, ck->phone_hash);
	return hash;
}

static size_t conn_hash(const ht_link_t *item)
{
	connection_t *conn = hash_table_get_inst(item, connection_t, link);
	return conn_key_hash(&(conn_key_t){
		.task_id = conn->in_task_id,
		.phone_hash = conn->in_phone_hash
	});
}

static bool conn_key_equal(void *key, const ht_link_t *item)
{
	conn_key_t *ck = (conn_key_t *) key;
	connection_t *conn = hash_table_get_inst(item, connection_t, link);
	return ((ck->task_id == conn->in_task_id) &&
	    (ck->phone_hash == conn->in_phone_hash));
}

/** Operations for the connection hash table. */
static hash_table_ops_t conn_hash_table_ops = {
	.hash = conn_hash,
	.key_hash = conn_key_hash,
	.key_equal = conn_key_equal,
	.equal = NULL,
	.remove_callback = NULL
};

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

/** Wrapper for client connection fibril.
 *
 * When a new connection arrives, a fibril with this implementing
 * function is created.
 *
 * @param arg Connection structure pointer.
 *
 * @return Always zero.
 *
 */
static errno_t connection_fibril(void *arg)
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
		ipc_answer_0(fibril_connection->chandle, ENOMEM);
		return 0;
	}

	fibril_connection->client = client;

	/*
	 * Call the connection handler function.
	 */
	fibril_connection->handler(fibril_connection->chandle,
	    &fibril_connection->call, fibril_connection->data);

	/*
	 * Remove the reference for this client task connection.
	 */
	async_client_put(client);

	/*
	 * Remove myself from the connection hash table.
	 */
	futex_down(&async_futex);
	hash_table_remove(&conn_hash_table, &(conn_key_t){
		.task_id = fibril_connection->in_task_id,
		.phone_hash = fibril_connection->in_phone_hash
	});
	futex_up(&async_futex);

	/*
	 * Answer all remaining messages with EHANGUP.
	 */
	while (!list_empty(&fibril_connection->msg_queue)) {
		msg_t *msg =
		    list_get_instance(list_first(&fibril_connection->msg_queue),
		    msg_t, link);

		list_remove(&msg->link);
		ipc_answer_0(msg->chandle, EHANGUP);
		free(msg);
	}

	/*
	 * If the connection was hung-up, answer the last call,
	 * i.e. IPC_M_PHONE_HUNGUP.
	 */
	if (fibril_connection->close_chandle)
		ipc_answer_0(fibril_connection->close_chandle, EOK);

	free(fibril_connection);
	return EOK;
}

/** Create a new fibril for a new connection.
 *
 * Create new fibril for connection, fill in connection structures and insert it
 * into the hash table, so that later we can easily do routing of messages to
 * particular fibrils.
 *
 * @param in_task_id     Identification of the incoming connection.
 * @param in_phone_hash  Identification of the incoming connection.
 * @param chandle        Handle of the opening IPC_M_CONNECT_ME_TO call.
 *                       If chandle is CAP_NIL, the connection was opened by
 *                       accepting the IPC_M_CONNECT_TO_ME call and this
 *                       function is called directly by the server.
 * @param call           Call data of the opening call.
 * @param handler        Connection handler.
 * @param data           Client argument to pass to the connection handler.
 *
 * @return  New fibril id or NULL on failure.
 *
 */
static fid_t async_new_connection(task_id_t in_task_id, sysarg_t in_phone_hash,
    cap_call_handle_t chandle, ipc_call_t *call, async_port_handler_t handler,
    void *data)
{
	connection_t *conn = malloc(sizeof(*conn));
	if (!conn) {
		if (chandle != CAP_NIL)
			ipc_answer_0(chandle, ENOMEM);

		return (uintptr_t) NULL;
	}

	conn->in_task_id = in_task_id;
	conn->in_phone_hash = in_phone_hash;
	list_initialize(&conn->msg_queue);
	conn->chandle = chandle;
	conn->close_chandle = CAP_NIL;
	conn->handler = handler;
	conn->data = data;

	if (call)
		conn->call = *call;

	/* We will activate the fibril ASAP */
	conn->wdata.active = true;
	conn->wdata.fid = fibril_create(connection_fibril, conn);

	if (conn->wdata.fid == 0) {
		free(conn);

		if (chandle != CAP_NIL)
			ipc_answer_0(chandle, ENOMEM);

		return (uintptr_t) NULL;
	}

	/* Add connection to the connection hash table */

	futex_down(&async_futex);
	hash_table_insert(&conn_hash_table, &conn->link);
	futex_up(&async_futex);

	fibril_add_ready(conn->wdata.fid);

	return conn->wdata.fid;
}

/** Wrapper for making IPC_M_CONNECT_TO_ME calls using the async framework.
 *
 * Ask through phone for a new connection to some service.
 *
 * @param exch    Exchange for sending the message.
 * @param iface   Callback interface.
 * @param arg1    User defined argument.
 * @param arg2    User defined argument.
 * @param handler Callback handler.
 * @param data    Handler data.
 * @param port_id ID of the newly created port.
 *
 * @return Zero on success or an error code.
 *
 */
errno_t async_create_callback_port(async_exch_t *exch, iface_t iface, sysarg_t arg1,
    sysarg_t arg2, async_port_handler_t handler, void *data, port_id_t *port_id)
{
	if ((iface & IFACE_MOD_CALLBACK) != IFACE_MOD_CALLBACK)
		return EINVAL;

	if (exch == NULL)
		return ENOENT;

	ipc_call_t answer;
	aid_t req = async_send_3(exch, IPC_M_CONNECT_TO_ME, iface, arg1, arg2,
	    &answer);

	errno_t rc;
	async_wait_for(req, &rc);
	if (rc != EOK)
		return rc;

	rc = async_create_port_internal(iface, handler, data, port_id);
	if (rc != EOK)
		return rc;

	sysarg_t phone_hash = IPC_GET_ARG5(answer);
	fid_t fid = async_new_connection(answer.in_task_id, phone_hash,
	    CAP_NIL, NULL, handler, data);
	if (fid == (uintptr_t) NULL)
		return ENOMEM;

	return EOK;
}

static size_t notification_key_hash(void *key)
{
	sysarg_t id = *(sysarg_t *) key;
	return id;
}

static size_t notification_hash(const ht_link_t *item)
{
	notification_t *notification =
	    hash_table_get_inst(item, notification_t, htlink);
	return notification_key_hash(&notification->imethod);
}

static bool notification_key_equal(void *key, const ht_link_t *item)
{
	sysarg_t id = *(sysarg_t *) key;
	notification_t *notification =
	    hash_table_get_inst(item, notification_t, htlink);
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
		awaiter_t *cur =
		    list_get_instance(tmp, awaiter_t, to_event.link);

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
 * @param chandle  Handle of the incoming call.
 * @param call     Data of the incoming call.
 *
 * @return False if the call doesn't match any connection.
 * @return True if the call was passed to the respective connection fibril.
 *
 */
static bool route_call(cap_call_handle_t chandle, ipc_call_t *call)
{
	assert(call);

	futex_down(&async_futex);

	ht_link_t *link = hash_table_find(&conn_hash_table, &(conn_key_t){
		.task_id = call->in_task_id,
		.phone_hash = call->in_phone_hash
	});
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

	msg->chandle = chandle;
	msg->call = *call;
	list_append(&msg->link, &conn->msg_queue);

	if (IPC_GET_IMETHOD(*call) == IPC_M_PHONE_HUNGUP)
		conn->close_chandle = chandle;

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

/** Function implementing the notification handler fibril. Never returns. */
static errno_t notification_fibril_func(void *arg)
{
	(void) arg;

	while (true) {
		fibril_semaphore_down(&notification_semaphore);

		futex_lock(&notification_futex);

		/*
		 * The semaphore ensures that if we get this far,
		 * the queue must be non-empty.
		 */
		assert(!list_empty(&notification_queue));

		notification_t *notification = list_get_instance(
		    list_first(&notification_queue), notification_t, qlink);
		list_remove(&notification->qlink);

		async_notification_handler_t handler = notification->handler;
		void *arg = notification->arg;
		ipc_call_t calldata = notification->calldata;
		long count = notification->count;

		notification->count = 0;

		futex_unlock(&notification_futex);

		// FIXME: Pass count to the handler. It might be important.
		(void) count;

		if (handler)
			handler(&calldata, arg);
	}

	/* Not reached. */
	return EOK;
}

/**
 * Creates a new dedicated fibril for handling notifications.
 * By default, there is one such fibril. This function can be used to
 * create more in order to increase the number of notification that can
 * be processed concurrently.
 *
 * Currently, there is no way to destroy those fibrils after they are created.
 */
errno_t async_spawn_notification_handler(void)
{
	fid_t f = fibril_create(notification_fibril_func, NULL);
	if (f == 0)
		return ENOMEM;

	fibril_add_ready(f);
	return EOK;
}

/** Queue notification.
 *
 * @param call   Data of the incoming call.
 *
 */
static void queue_notification(ipc_call_t *call)
{
	assert(call);

	futex_lock(&notification_futex);

	ht_link_t *link = hash_table_find(&notification_hash_table,
	    &IPC_GET_IMETHOD(*call));
	if (!link) {
		/* Invalid notification. */
		// TODO: Make sure this can't happen and turn it into assert.
		futex_unlock(&notification_futex);
		return;
	}

	notification_t *notification =
	    hash_table_get_inst(link, notification_t, htlink);

	notification->count++;
	notification->calldata = *call;

	if (link_in_use(&notification->qlink)) {
		/* Notification already queued. */
		futex_unlock(&notification_futex);
		return;
	}

	list_append(&notification->qlink, &notification_queue);
	futex_unlock(&notification_futex);

	fibril_semaphore_up(&notification_semaphore);
}

/**
 * Creates a new notification structure and inserts it into the hash table.
 *
 * @param handler  Function to call when notification is received.
 * @param arg      Argument for the handler function.
 * @return         The newly created notification structure.
 */
static notification_t *notification_create(async_notification_handler_t handler, void *arg)
{
	notification_t *notification = calloc(1, sizeof(notification_t));
	if (!notification)
		return NULL;

	notification->handler = handler;
	notification->arg = arg;

	fid_t fib = 0;

	futex_lock(&notification_futex);

	if (notification_avail == 0) {
		/* Attempt to create the first handler fibril. */
		fib = fibril_create(notification_fibril_func, NULL);
		if (fib == 0) {
			futex_unlock(&notification_futex);
			free(notification);
			return NULL;
		}
	}

	sysarg_t imethod = notification_avail;
	notification_avail++;

	notification->imethod = imethod;
	hash_table_insert(&notification_hash_table, &notification->htlink);

	futex_unlock(&notification_futex);

	if (imethod == 0) {
		assert(fib);
		fibril_add_ready(fib);
	}

	return notification;
}

/** Subscribe to IRQ notification.
 *
 * @param inr     IRQ number.
 * @param handler Notification handler.
 * @param data    Notification handler client data.
 * @param ucode   Top-half pseudocode handler.
 *
 * @param[out] handle  IRQ capability handle on success.
 *
 * @return An error code.
 *
 */
errno_t async_irq_subscribe(int inr, async_notification_handler_t handler,
    void *data, const irq_code_t *ucode, cap_irq_handle_t *handle)
{
	notification_t *notification = notification_create(handler, data);
	if (!notification)
		return ENOMEM;

	cap_irq_handle_t ihandle;
	errno_t rc = ipc_irq_subscribe(inr, notification->imethod, ucode,
	    &ihandle);
	if (rc == EOK && handle != NULL) {
		*handle = ihandle;
	}
	return rc;
}

/** Unsubscribe from IRQ notification.
 *
 * @param handle  IRQ capability handle.
 *
 * @return Zero on success or an error code.
 *
 */
errno_t async_irq_unsubscribe(cap_irq_handle_t ihandle)
{
	// TODO: Remove entry from hash table
	//       to avoid memory leak

	return ipc_irq_unsubscribe(ihandle);
}

/** Subscribe to event notifications.
 *
 * @param evno    Event type to subscribe.
 * @param handler Notification handler.
 * @param data    Notification handler client data.
 *
 * @return Zero on success or an error code.
 *
 */
errno_t async_event_subscribe(event_type_t evno,
    async_notification_handler_t handler, void *data)
{
	notification_t *notification = notification_create(handler, data);
	if (!notification)
		return ENOMEM;

	return ipc_event_subscribe(evno, notification->imethod);
}

/** Subscribe to task event notifications.
 *
 * @param evno    Event type to subscribe.
 * @param handler Notification handler.
 * @param data    Notification handler client data.
 *
 * @return Zero on success or an error code.
 *
 */
errno_t async_event_task_subscribe(event_task_type_t evno,
    async_notification_handler_t handler, void *data)
{
	notification_t *notification = notification_create(handler, data);
	if (!notification)
		return ENOMEM;

	return ipc_event_task_subscribe(evno, notification->imethod);
}

/** Unmask event notifications.
 *
 * @param evno Event type to unmask.
 *
 * @return Value returned by the kernel.
 *
 */
errno_t async_event_unmask(event_type_t evno)
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
errno_t async_event_task_unmask(event_task_type_t evno)
{
	return ipc_event_task_unmask(evno);
}

/** Return new incoming message for the current (fibril-local) connection.
 *
 * @param call   Storage where the incoming call data will be stored.
 * @param usecs  Timeout in microseconds. Zero denotes no timeout.
 *
 * @return  If no timeout was specified, then a handle of the incoming call is
 *          returned. If a timeout is specified, then a handle of the incoming
 *          call is returned unless the timeout expires prior to receiving a
 *          message. In that case zero CAP_NIL is returned.
 */
cap_call_handle_t async_get_call_timeout(ipc_call_t *call, suseconds_t usecs)
{
	assert(call);
	assert(fibril_connection);

	/*
	 * Why doing this?
	 * GCC 4.1.0 coughs on fibril_connection-> dereference.
	 * GCC 4.1.1 happilly puts the rdhwr instruction in delay slot.
	 *           I would never expect to find so many errors in
	 *           a compiler.
	 */
	connection_t *conn = fibril_connection;

	futex_down(&async_futex);

	if (usecs) {
		getuptime(&conn->wdata.to_event.expires);
		tv_add_diff(&conn->wdata.to_event.expires, usecs);
	} else
		conn->wdata.to_event.inlist = false;

	/* If nothing in queue, wait until something arrives */
	while (list_empty(&conn->msg_queue)) {
		if (conn->close_chandle) {
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
			return conn->close_chandle;
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
		if ((usecs) && (conn->wdata.to_event.occurred) &&
		    (list_empty(&conn->msg_queue))) {
			/* If we timed out -> exit */
			futex_up(&async_futex);
			return CAP_NIL;
		}
	}

	msg_t *msg = list_get_instance(list_first(&conn->msg_queue),
	    msg_t, link);
	list_remove(&msg->link);

	cap_call_handle_t chandle = msg->chandle;
	*call = msg->call;
	free(msg);

	futex_up(&async_futex);
	return chandle;
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

/** Handle a call that was received.
 *
 * If the call has the IPC_M_CONNECT_ME_TO method, a new connection is created.
 * Otherwise the call is routed to its connection fibril.
 *
 * @param chandle  Handle of the incoming call.
 * @param call     Data of the incoming call.
 *
 */
static void handle_call(cap_call_handle_t chandle, ipc_call_t *call)
{
	assert(call);

	/* Kernel notification */
	if ((chandle == CAP_NIL) && (call->flags & IPC_CALL_NOTIF)) {
		queue_notification(call);
		return;
	}

	/* New connection */
	if (IPC_GET_IMETHOD(*call) == IPC_M_CONNECT_ME_TO) {
		iface_t iface = (iface_t) IPC_GET_ARG1(*call);
		sysarg_t in_phone_hash = IPC_GET_ARG5(*call);

		// TODO: Currently ignores all ports but the first one.
		void *data;
		async_port_handler_t handler =
		    async_get_port_handler(iface, 0, &data);

		async_new_connection(call->in_task_id, in_phone_hash, chandle,
		    call, handler, data);
		return;
	}

	/* Try to route the call through the connection hash table */
	if (route_call(chandle, call))
		return;

	/* Unknown call from unknown phone - hang it up */
	ipc_answer_0(chandle, EHANGUP);
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
static errno_t async_manager_worker(void)
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
				timeout = tv_sub_diff(&waiter->to_event.expires,
				    &tv);
				futex_up(&async_futex);
			}
		} else {
			futex_up(&async_futex);
			timeout = SYNCH_NO_TIMEOUT;
		}

		atomic_inc(&threads_in_ipc_wait);

		ipc_call_t call;
		errno_t rc = ipc_wait_cycle(&call, timeout, flags);

		atomic_dec(&threads_in_ipc_wait);

		assert(rc == EOK);

		if (call.cap_handle == CAP_NIL) {
			if ((call.flags &
			    (IPC_CALL_NOTIF | IPC_CALL_ANSWERED)) == 0) {
				/* Neither a notification nor an answer. */
				handle_expired_timeouts();
				continue;
			}
		}

		if (call.flags & IPC_CALL_ANSWERED)
			continue;

		handle_call(call.cap_handle, &call);
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
static errno_t async_manager_fibril(void *arg)
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
	fid_t fid = fibril_create_generic(async_manager_fibril, NULL, PAGE_SIZE);
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
void __async_server_init(void)
{
	if (!hash_table_create(&client_hash_table, 0, 0, &client_hash_table_ops))
		abort();

	if (!hash_table_create(&conn_hash_table, 0, 0, &conn_hash_table_ops))
		abort();

	if (!hash_table_create(&notification_hash_table, 0, 0,
	    &notification_hash_table_ops))
		abort();
}

errno_t async_answer_0(cap_call_handle_t chandle, errno_t retval)
{
	return ipc_answer_0(chandle, retval);
}

errno_t async_answer_1(cap_call_handle_t chandle, errno_t retval, sysarg_t arg1)
{
	return ipc_answer_1(chandle, retval, arg1);
}

errno_t async_answer_2(cap_call_handle_t chandle, errno_t retval, sysarg_t arg1,
    sysarg_t arg2)
{
	return ipc_answer_2(chandle, retval, arg1, arg2);
}

errno_t async_answer_3(cap_call_handle_t chandle, errno_t retval, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3)
{
	return ipc_answer_3(chandle, retval, arg1, arg2, arg3);
}

errno_t async_answer_4(cap_call_handle_t chandle, errno_t retval, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4)
{
	return ipc_answer_4(chandle, retval, arg1, arg2, arg3, arg4);
}

errno_t async_answer_5(cap_call_handle_t chandle, errno_t retval, sysarg_t arg1,
    sysarg_t arg2, sysarg_t arg3, sysarg_t arg4, sysarg_t arg5)
{
	return ipc_answer_5(chandle, retval, arg1, arg2, arg3, arg4, arg5);
}

errno_t async_forward_fast(cap_call_handle_t chandle, async_exch_t *exch,
    sysarg_t imethod, sysarg_t arg1, sysarg_t arg2, unsigned int mode)
{
	if (exch == NULL)
		return ENOENT;

	return ipc_forward_fast(chandle, exch->phone, imethod, arg1, arg2, mode);
}

errno_t async_forward_slow(cap_call_handle_t chandle, async_exch_t *exch,
    sysarg_t imethod, sysarg_t arg1, sysarg_t arg2, sysarg_t arg3,
    sysarg_t arg4, sysarg_t arg5, unsigned int mode)
{
	if (exch == NULL)
		return ENOENT;

	return ipc_forward_slow(chandle, exch->phone, imethod, arg1, arg2, arg3,
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
 *
 * @return Zero on success or an error code.
 *
 */
errno_t async_connect_to_me(async_exch_t *exch, sysarg_t arg1, sysarg_t arg2,
    sysarg_t arg3)
{
	if (exch == NULL)
		return ENOENT;

	ipc_call_t answer;
	aid_t req = async_send_3(exch, IPC_M_CONNECT_TO_ME, arg1, arg2, arg3,
	    &answer);

	errno_t rc;
	async_wait_for(req, &rc);
	if (rc != EOK)
		return (errno_t) rc;

	return EOK;
}

/** Interrupt one thread of this task from waiting for IPC. */
void async_poke(void)
{
	if (atomic_get(&threads_in_ipc_wait) > 0)
		ipc_poke();
}

/** Wrapper for receiving the IPC_M_SHARE_IN calls using the async framework.
 *
 * This wrapper only makes it more comfortable to receive IPC_M_SHARE_IN
 * calls so that the user doesn't have to remember the meaning of each IPC
 * argument.
 *
 * So far, this wrapper is to be used from within a connection fibril.
 *
 * @param chandle  Storage for the handle of the IPC_M_SHARE_IN call.
 * @param size     Destination address space area size.
 *
 * @return True on success, false on failure.
 *
 */
bool async_share_in_receive(cap_call_handle_t *chandle, size_t *size)
{
	assert(chandle);
	assert(size);

	ipc_call_t data;
	*chandle = async_get_call(&data);

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
 * @param chandle  Handle of the IPC_M_DATA_READ call to answer.
 * @param src      Source address space base.
 * @param flags    Flags to be used for sharing. Bits can be only cleared.
 *
 * @return Zero on success or a value from @ref errno.h on failure.
 *
 */
errno_t async_share_in_finalize(cap_call_handle_t chandle, void *src,
    unsigned int flags)
{
	// FIXME: The source has no business deciding destination address.
	return ipc_answer_3(chandle, EOK, (sysarg_t) src, (sysarg_t) flags,
	    (sysarg_t) _end);
}

/** Wrapper for receiving the IPC_M_SHARE_OUT calls using the async framework.
 *
 * This wrapper only makes it more comfortable to receive IPC_M_SHARE_OUT
 * calls so that the user doesn't have to remember the meaning of each IPC
 * argument.
 *
 * So far, this wrapper is to be used from within a connection fibril.
 *
 * @param chandle  Storage for the hash of the IPC_M_SHARE_OUT call.
 * @param size     Storage for the source address space area size.
 * @param flags    Storage for the sharing flags.
 *
 * @return True on success, false on failure.
 *
 */
bool async_share_out_receive(cap_call_handle_t *chandle, size_t *size,
    unsigned int *flags)
{
	assert(chandle);
	assert(size);
	assert(flags);

	ipc_call_t data;
	*chandle = async_get_call(&data);

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
 * @param chandle  Handle of the IPC_M_DATA_WRITE call to answer.
 * @param dst      Address of the storage for the destination address space area
 *                 base address.
 *
 * @return  Zero on success or a value from @ref errno.h on failure.
 *
 */
errno_t async_share_out_finalize(cap_call_handle_t chandle, void **dst)
{
	return ipc_answer_2(chandle, EOK, (sysarg_t) _end, (sysarg_t) dst);
}

/** Wrapper for receiving the IPC_M_DATA_READ calls using the async framework.
 *
 * This wrapper only makes it more comfortable to receive IPC_M_DATA_READ
 * calls so that the user doesn't have to remember the meaning of each IPC
 * argument.
 *
 * So far, this wrapper is to be used from within a connection fibril.
 *
 * @param chandle  Storage for the handle of the IPC_M_DATA_READ.
 * @param size     Storage for the maximum size. Can be NULL.
 *
 * @return True on success, false on failure.
 *
 */
bool async_data_read_receive(cap_call_handle_t *chandle, size_t *size)
{
	ipc_call_t data;
	return async_data_read_receive_call(chandle, &data, size);
}

/** Wrapper for receiving the IPC_M_DATA_READ calls using the async framework.
 *
 * This wrapper only makes it more comfortable to receive IPC_M_DATA_READ
 * calls so that the user doesn't have to remember the meaning of each IPC
 * argument.
 *
 * So far, this wrapper is to be used from within a connection fibril.
 *
 * @param chandle  Storage for the handle of the IPC_M_DATA_READ.
 * @param size     Storage for the maximum size. Can be NULL.
 *
 * @return True on success, false on failure.
 *
 */
bool async_data_read_receive_call(cap_call_handle_t *chandle, ipc_call_t *data,
    size_t *size)
{
	assert(chandle);
	assert(data);

	*chandle = async_get_call(data);

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
 * @param chandle  Handle of the IPC_M_DATA_READ call to answer.
 * @param src      Source address for the IPC_M_DATA_READ call.
 * @param size     Size for the IPC_M_DATA_READ call. Can be smaller than
 *                 the maximum size announced by the sender.
 *
 * @return  Zero on success or a value from @ref errno.h on failure.
 *
 */
errno_t async_data_read_finalize(cap_call_handle_t chandle, const void *src,
    size_t size)
{
	return ipc_answer_2(chandle, EOK, (sysarg_t) src, (sysarg_t) size);
}

/** Wrapper for forwarding any read request
 *
 */
errno_t async_data_read_forward_fast(async_exch_t *exch, sysarg_t imethod,
    sysarg_t arg1, sysarg_t arg2, sysarg_t arg3, sysarg_t arg4,
    ipc_call_t *dataptr)
{
	if (exch == NULL)
		return ENOENT;

	cap_call_handle_t chandle;
	if (!async_data_read_receive(&chandle, NULL)) {
		ipc_answer_0(chandle, EINVAL);
		return EINVAL;
	}

	aid_t msg = async_send_fast(exch, imethod, arg1, arg2, arg3, arg4,
	    dataptr);
	if (msg == 0) {
		ipc_answer_0(chandle, EINVAL);
		return EINVAL;
	}

	errno_t retval = ipc_forward_fast(chandle, exch->phone, 0, 0, 0,
	    IPC_FF_ROUTE_FROM_ME);
	if (retval != EOK) {
		async_forget(msg);
		ipc_answer_0(chandle, retval);
		return retval;
	}

	errno_t rc;
	async_wait_for(msg, &rc);

	return (errno_t) rc;
}

/** Wrapper for receiving the IPC_M_DATA_WRITE calls using the async framework.
 *
 * This wrapper only makes it more comfortable to receive IPC_M_DATA_WRITE
 * calls so that the user doesn't have to remember the meaning of each IPC
 * argument.
 *
 * So far, this wrapper is to be used from within a connection fibril.
 *
 * @param chandle  Storage for the handle of the IPC_M_DATA_WRITE.
 * @param size     Storage for the suggested size. May be NULL.
 *
 * @return  True on success, false on failure.
 *
 */
bool async_data_write_receive(cap_call_handle_t *chandle, size_t *size)
{
	ipc_call_t data;
	return async_data_write_receive_call(chandle, &data, size);
}

/** Wrapper for receiving the IPC_M_DATA_WRITE calls using the async framework.
 *
 * This wrapper only makes it more comfortable to receive IPC_M_DATA_WRITE
 * calls so that the user doesn't have to remember the meaning of each IPC
 * argument.
 *
 * So far, this wrapper is to be used from within a connection fibril.
 *
 * @param chandle  Storage for the handle of the IPC_M_DATA_WRITE.
 * @param data     Storage for the ipc call data.
 * @param size     Storage for the suggested size. May be NULL.
 *
 * @return True on success, false on failure.
 *
 */
bool async_data_write_receive_call(cap_call_handle_t *chandle, ipc_call_t *data,
    size_t *size)
{
	assert(chandle);
	assert(data);

	*chandle = async_get_call(data);

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
 * @param chandle  Handle of the IPC_M_DATA_WRITE call to answer.
 * @param dst      Final destination address for the IPC_M_DATA_WRITE call.
 * @param size     Final size for the IPC_M_DATA_WRITE call.
 *
 * @return  Zero on success or a value from @ref errno.h on failure.
 *
 */
errno_t async_data_write_finalize(cap_call_handle_t chandle, void *dst,
    size_t size)
{
	return ipc_answer_2(chandle, EOK, (sysarg_t) dst, (sysarg_t) size);
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
errno_t async_data_write_accept(void **data, const bool nullterm,
    const size_t min_size, const size_t max_size, const size_t granularity,
    size_t *received)
{
	assert(data);

	cap_call_handle_t chandle;
	size_t size;
	if (!async_data_write_receive(&chandle, &size)) {
		ipc_answer_0(chandle, EINVAL);
		return EINVAL;
	}

	if (size < min_size) {
		ipc_answer_0(chandle, EINVAL);
		return EINVAL;
	}

	if ((max_size > 0) && (size > max_size)) {
		ipc_answer_0(chandle, EINVAL);
		return EINVAL;
	}

	if ((granularity > 0) && ((size % granularity) != 0)) {
		ipc_answer_0(chandle, EINVAL);
		return EINVAL;
	}

	void *arg_data;

	if (nullterm)
		arg_data = malloc(size + 1);
	else
		arg_data = malloc(size);

	if (arg_data == NULL) {
		ipc_answer_0(chandle, ENOMEM);
		return ENOMEM;
	}

	errno_t rc = async_data_write_finalize(chandle, arg_data, size);
	if (rc != EOK) {
		free(arg_data);
		return rc;
	}

	if (nullterm)
		((char *) arg_data)[size] = 0;

	*data = arg_data;
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
void async_data_write_void(errno_t retval)
{
	cap_call_handle_t chandle;
	async_data_write_receive(&chandle, NULL);
	ipc_answer_0(chandle, retval);
}

/** Wrapper for forwarding any data that is about to be received
 *
 */
errno_t async_data_write_forward_fast(async_exch_t *exch, sysarg_t imethod,
    sysarg_t arg1, sysarg_t arg2, sysarg_t arg3, sysarg_t arg4,
    ipc_call_t *dataptr)
{
	if (exch == NULL)
		return ENOENT;

	cap_call_handle_t chandle;
	if (!async_data_write_receive(&chandle, NULL)) {
		ipc_answer_0(chandle, EINVAL);
		return EINVAL;
	}

	aid_t msg = async_send_fast(exch, imethod, arg1, arg2, arg3, arg4,
	    dataptr);
	if (msg == 0) {
		ipc_answer_0(chandle, EINVAL);
		return EINVAL;
	}

	errno_t retval = ipc_forward_fast(chandle, exch->phone, 0, 0, 0,
	    IPC_FF_ROUTE_FROM_ME);
	if (retval != EOK) {
		async_forget(msg);
		ipc_answer_0(chandle, retval);
		return retval;
	}

	errno_t rc;
	async_wait_for(msg, &rc);

	return (errno_t) rc;
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
	cap_call_handle_t chandle = async_get_call(&call);
	cap_phone_handle_t phandle = (cap_handle_t) IPC_GET_ARG5(call);

	if ((IPC_GET_IMETHOD(call) != IPC_M_CONNECT_TO_ME) ||
	    !CAP_HANDLE_VALID((phandle))) {
		async_answer_0(chandle, EINVAL);
		return NULL;
	}

	async_sess_t *sess = (async_sess_t *) malloc(sizeof(async_sess_t));
	if (sess == NULL) {
		async_answer_0(chandle, ENOMEM);
		return NULL;
	}

	sess->iface = 0;
	sess->mgmt = mgmt;
	sess->phone = phandle;
	sess->arg1 = 0;
	sess->arg2 = 0;
	sess->arg3 = 0;

	fibril_mutex_initialize(&sess->remote_state_mtx);
	sess->remote_state_data = NULL;

	list_initialize(&sess->exch_list);
	fibril_mutex_initialize(&sess->mutex);
	atomic_set(&sess->refcnt, 0);

	/* Acknowledge the connected phone */
	async_answer_0(chandle, EOK);

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
	cap_phone_handle_t phandle = (cap_handle_t) IPC_GET_ARG5(*call);

	if ((IPC_GET_IMETHOD(*call) != IPC_M_CONNECT_TO_ME) ||
	    !CAP_HANDLE_VALID((phandle)))
		return NULL;

	async_sess_t *sess = (async_sess_t *) malloc(sizeof(async_sess_t));
	if (sess == NULL)
		return NULL;

	sess->iface = 0;
	sess->mgmt = mgmt;
	sess->phone = phandle;
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

bool async_state_change_receive(cap_call_handle_t *chandle, sysarg_t *arg1,
    sysarg_t *arg2, sysarg_t *arg3)
{
	assert(chandle);

	ipc_call_t call;
	*chandle = async_get_call(&call);

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

errno_t async_state_change_finalize(cap_call_handle_t chandle,
    async_exch_t *other_exch)
{
	return ipc_answer_1(chandle, EOK, CAP_HANDLE_RAW(other_exch->phone));
}

/** @}
 */
