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
 * The aim of this library is facilitating writing programs utilizing the
 * asynchronous nature of HelenOS IPC, yet using a normal way of programming. 
 *
 * You should be able to write very simple multithreaded programs, the async
 * framework will automatically take care of most synchronization problems.
 *
 * Default semantics:
 * - async_send_*():	send asynchronously. If the kernel refuses to send
 * 			more messages, [ try to get responses from kernel, if
 * 			nothing found, might try synchronous ]
 *
 * Example of use (pseudo C):
 * 
 * 1) Multithreaded client application
 *
 * fibril_create(fibril1, ...);
 * fibril_create(fibril2, ...);
 * ...
 *  
 * int fibril1(void *arg)
 * {
 * 	conn = ipc_connect_me_to();
 *	c1 = async_send(conn);
 * 	c2 = async_send(conn);
 * 	async_wait_for(c1);
 * 	async_wait_for(c2);
 * 	...
 * }
 *
 *
 * 2) Multithreaded server application
 * main()
 * {
 * 	async_manager();
 * }
 * 
 *
 * my_client_connection(icallid, *icall)
 * {
 * 	if (want_refuse) {
 * 		ipc_answer_fast(icallid, ELIMIT, 0, 0);
 * 		return;
 * 	}
 * 	ipc_answer_fast(icallid, EOK, 0, 0);
 *
 * 	callid = async_get_call(&call);
 * 	handle_call(callid, call);
 * 	ipc_answer_fast(callid, 1, 2, 3);
 *
 * 	callid = async_get_call(&call);
 * 	....
 * }
 *
 */

#include <futex.h>
#include <async.h>
#include <fibril.h>
#include <stdio.h>
#include <libadt/hash_table.h>
#include <libadt/list.h>
#include <ipc/ipc.h>
#include <assert.h>
#include <errno.h>
#include <sys/time.h>
#include <arch/barrier.h>

atomic_t async_futex = FUTEX_INITIALIZER;
static hash_table_t conn_hash_table;
static LIST_INITIALIZE(timeout_list);

/** Structures of this type represent a waiting fibril. */
typedef struct {
	/** Expiration time. */
	struct timeval expires;		
	/** If true, this struct is in the timeout list. */
	int inlist;
	/** Timeout list link. */
	link_t link;

	/** Identification of and link to the waiting fibril. */
	fid_t fid;
	/** If true, this fibril is currently active. */
	int active;
	/** If true, we have timed out. */
	int timedout;
} awaiter_t;

typedef struct {
	awaiter_t wdata;
	
	/** If reply was received. */
	int done;
	/** Pointer to where the answer data is stored. */
 	ipc_call_t *dataptr;

	ipcarg_t retval;
} amsg_t;

/**
 * Structures of this type are used to group information about a call and a
 * message queue link.
 */
typedef struct {
	link_t link;
	ipc_callid_t callid;
	ipc_call_t call;
} msg_t;

typedef struct {
	awaiter_t wdata;

	/** Hash table link. */
	link_t link;

	/** Incoming phone hash. */
	ipcarg_t in_phone_hash;		

	/** Messages that should be delivered to this fibril. */
	link_t msg_queue;		
					 
	/** Identification of the opening call. */
	ipc_callid_t callid;
	/** Call data of the opening call. */
	ipc_call_t call;

	/** Identification of the closing call. */
	ipc_callid_t close_callid;

	/** Fibril function that will be used to handle the connection. */
	void (*cfibril)(ipc_callid_t, ipc_call_t *);
} connection_t;

/** Identifier of the incoming connection handled by the current fibril. */
__thread connection_t *FIBRIL_connection;

/**
 * If true, it is forbidden to use async_req functions and all preemption is
 * disabled.
 */
__thread int in_interrupt_handler;

static void default_client_connection(ipc_callid_t callid, ipc_call_t *call);
static void default_interrupt_received(ipc_callid_t callid, ipc_call_t *call);

/**
 * Pointer to a fibril function that will be used to handle connections.
 */
static async_client_conn_t client_connection = default_client_connection;
/**
 * Pointer to a fibril function that will be used to handle interrupt
 * notifications.
 */
static async_client_conn_t interrupt_received = default_interrupt_received;

#define CONN_HASH_TABLE_CHAINS	32

/** Compute hash into the connection hash table based on the source phone hash.
 *
 * @param key		Pointer to source phone hash.
 *
 * @return		Index into the connection hash table.
 */
static hash_index_t conn_hash(unsigned long *key)
{
	assert(key);
	return ((*key) >> 4) % CONN_HASH_TABLE_CHAINS;
}

/** Compare hash table item with a key.
 *
 * @param key		Array containing the source phone hash as the only item.
 * @param keys		Expected 1 but ignored.
 * @param item		Connection hash table item.
 *
 * @return		True on match, false otherwise.
 */
static int conn_compare(unsigned long key[], hash_count_t keys, link_t *item)
{
	connection_t *hs;

	hs = hash_table_get_instance(item, connection_t, link);
	
	return key[0] == hs->in_phone_hash;
}

/** Connection hash table removal callback function.
 *
 * This function is called whenever a connection is removed from the connection
 * hash table.
 *
 * @param item		Connection hash table item being removed.
 */
static void conn_remove(link_t *item)
{
	free(hash_table_get_instance(item, connection_t, link));
}


/** Operations for the connection hash table. */
static hash_table_operations_t conn_hash_table_ops = {
	.hash = conn_hash,
	.compare = conn_compare,
	.remove_callback = conn_remove
};

/** Sort in current fibril's timeout request.
 *
 * @param wd		Wait data of the current fibril.
 */
static void insert_timeout(awaiter_t *wd)
{
	link_t *tmp;
	awaiter_t *cur;

	wd->timedout = 0;
	wd->inlist = 1;

	tmp = timeout_list.next;
	while (tmp != &timeout_list) {
		cur = list_get_instance(tmp, awaiter_t, link);
		if (tv_gteq(&cur->expires, &wd->expires))
			break;
		tmp = tmp->next;
	}
	list_append(&wd->link, tmp);
}

/** Try to route a call to an appropriate connection fibril.
 *
 * If the proper connection fibril is found, a message with the call is added to
 * its message queue. If the fibril was not active, it is activated and all
 * timeouts are unregistered.
 *
 * @param callid	Hash of the incoming call.
 * @param call		Data of the incoming call.
 *
 * @return		Zero if the call doesn't match any connection.
 * 			One if the call was passed to the respective connection
 * 			fibril.
 */
static int route_call(ipc_callid_t callid, ipc_call_t *call)
{
	connection_t *conn;
	msg_t *msg;
	link_t *hlp;
	unsigned long key;

	futex_down(&async_futex);

	key = call->in_phone_hash;
	hlp = hash_table_find(&conn_hash_table, &key);
	if (!hlp) {
		futex_up(&async_futex);
		return 0;
	}
	conn = hash_table_get_instance(hlp, connection_t, link);

	msg = malloc(sizeof(*msg));
	msg->callid = callid;
	msg->call = *call;
	list_append(&msg->link, &conn->msg_queue);

	if (IPC_GET_METHOD(*call) == IPC_M_PHONE_HUNGUP)
		conn->close_callid = callid;
	
	/* If the connection fibril is waiting for an event, activate it */
	if (!conn->wdata.active) {
		/* If in timeout list, remove it */
		if (conn->wdata.inlist) {
			conn->wdata.inlist = 0;
			list_remove(&conn->wdata.link);
		}
		conn->wdata.active = 1;
		fibril_add_ready(conn->wdata.fid);
	}

	futex_up(&async_futex);

	return 1;
}

/** Return new incoming message for the current (fibril-local) connection.
 *
 * @param call		Storage where the incoming call data will be stored.
 * @param usecs		Timeout in microseconds. Zero denotes no timeout.
 *
 * @return		If no timeout was specified, then a hash of the
 * 			incoming call is returned. If a timeout is specified,
 * 			then a hash of the incoming call is returned unless
 * 			the timeout expires prior to receiving a message. In
 * 			that case zero is returned.
 */
ipc_callid_t async_get_call_timeout(ipc_call_t *call, suseconds_t usecs)
{
	msg_t *msg;
	ipc_callid_t callid;
	connection_t *conn;
	
	assert(FIBRIL_connection);
	/* GCC 4.1.0 coughs on FIBRIL_connection-> dereference,
	 * GCC 4.1.1 happilly puts the rdhwr instruction in delay slot.
	 *           I would never expect to find so many errors in 
	 *           a compiler *($&$(*&$
	 */
	conn = FIBRIL_connection; 

	futex_down(&async_futex);

	if (usecs) {
		gettimeofday(&conn->wdata.expires, NULL);
		tv_add(&conn->wdata.expires, usecs);
	} else {
		conn->wdata.inlist = 0;
	}
	/* If nothing in queue, wait until something arrives */
	while (list_empty(&conn->msg_queue)) {
		if (usecs)
			insert_timeout(&conn->wdata);

		conn->wdata.active = 0;
		/*
		 * Note: the current fibril will be rescheduled either due to a
		 * timeout or due to an arriving message destined to it. In the
		 * former case, handle_expired_timeouts() and, in the latter
		 * case, route_call() will perform the wakeup.
		 */
		fibril_schedule_next_adv(FIBRIL_TO_MANAGER);
		/*
		 * Futex is up after getting back from async_manager get it
		 * again.
		 */
		futex_down(&async_futex);
		if (usecs && conn->wdata.timedout &&
		    list_empty(&conn->msg_queue)) {
			/* If we timed out -> exit */
			futex_up(&async_futex);
			return 0;
		}
	}
	
	msg = list_get_instance(conn->msg_queue.next, msg_t, link);
	list_remove(&msg->link);
	callid = msg->callid;
	*call = msg->call;
	free(msg);
	
	futex_up(&async_futex);
	return callid;
}

/** Default fibril function that gets called to handle new connection.
 *
 * This function is defined as a weak symbol - to be redefined in user code.
 *
 * @param callid	Hash of the incoming call.
 * @param call		Data of the incoming call.
 */
static void default_client_connection(ipc_callid_t callid, ipc_call_t *call)
{
	ipc_answer_fast(callid, ENOENT, 0, 0);
}

/** Default fibril function that gets called to handle interrupt notifications.
 *
 * @param callid	Hash of the incoming call.
 * @param call		Data of the incoming call.
 */
static void default_interrupt_received(ipc_callid_t callid, ipc_call_t *call)
{
}

/** Wrapper for client connection fibril.
 *
 * When a new connection arrives, a fibril with this implementing function is
 * created. It calls client_connection() and does the final cleanup.
 *
 * @param arg		Connection structure pointer.
 *
 * @return		Always zero.
 */
static int connection_fibril(void  *arg)
{
	unsigned long key;
	msg_t *msg;
	int close_answered = 0;

	/* Setup fibril-local connection pointer */
	FIBRIL_connection = (connection_t *) arg;
	FIBRIL_connection->cfibril(FIBRIL_connection->callid,
	    &FIBRIL_connection->call);
	
	/* Remove myself from the connection hash table */
	futex_down(&async_futex);
	key = FIBRIL_connection->in_phone_hash;
	hash_table_remove(&conn_hash_table, &key, 1);
	futex_up(&async_futex);
	
	/* Answer all remaining messages with EHANGUP */
	while (!list_empty(&FIBRIL_connection->msg_queue)) {
		msg = list_get_instance(FIBRIL_connection->msg_queue.next,
		    msg_t, link);
		list_remove(&msg->link);
		if (msg->callid == FIBRIL_connection->close_callid)
			close_answered = 1;
		ipc_answer_fast(msg->callid, EHANGUP, 0, 0);
		free(msg);
	}
	if (FIBRIL_connection->close_callid)
		ipc_answer_fast(FIBRIL_connection->close_callid, 0, 0, 0);
	
	return 0;
}

/** Create a new fibril for a new connection.
 *
 * Creates new fibril for connection, fills in connection structures and inserts
 * it into the hash table, so that later we can easily do routing of messages to
 * particular fibrils.
 *
 * @param in_phone_hash	Identification of the incoming connection.
 * @param callid	Hash of the opening IPC_M_CONNECT_ME_TO call.
 * @param call		Call data of the opening call.
 * @param cfibril	Fibril function that should be called upon opening the
 * 			connection.
 *
 * @return 		New fibril id or NULL on failure.
 */
fid_t async_new_connection(ipcarg_t in_phone_hash, ipc_callid_t callid,
    ipc_call_t *call, void (*cfibril)(ipc_callid_t, ipc_call_t *))
{
	connection_t *conn;
	unsigned long key;

	conn = malloc(sizeof(*conn));
	if (!conn) {
		ipc_answer_fast(callid, ENOMEM, 0, 0);
		return NULL;
	}
	conn->in_phone_hash = in_phone_hash;
	list_initialize(&conn->msg_queue);
	conn->callid = callid;
	conn->close_callid = 0;
	if (call)
		conn->call = *call;
	conn->wdata.active = 1;	/* We will activate the fibril ASAP */
	conn->cfibril = cfibril;

	conn->wdata.fid = fibril_create(connection_fibril, conn);
	if (!conn->wdata.fid) {
		free(conn);
		ipc_answer_fast(callid, ENOMEM, 0, 0);
		return NULL;
	}
	/* Add connection to the connection hash table */
	key = conn->in_phone_hash;
	futex_down(&async_futex);
	hash_table_insert(&conn_hash_table, &key, &conn->link);
	futex_up(&async_futex);

	fibril_add_ready(conn->wdata.fid);

	return conn->wdata.fid;
}

/** Handle a call that was received.
 *
 * If the call has the IPC_M_CONNECT_ME_TO method, a new connection is created.
 * Otherwise the call is routed to its connection fibril.
 *
 * @param callid	Hash of the incoming call.
 * @param call		Data of the incoming call.
 */
static void handle_call(ipc_callid_t callid, ipc_call_t *call)
{
	/* Unrouted call - do some default behaviour */
	if ((callid & IPC_CALLID_NOTIFICATION)) {
		in_interrupt_handler = 1;
		(*interrupt_received)(callid,call);
		in_interrupt_handler = 0;
		return;
	}		

	switch (IPC_GET_METHOD(*call)) {
	case IPC_M_CONNECT_ME_TO:
		/* Open new connection with fibril etc. */
		async_new_connection(IPC_GET_ARG3(*call), callid, call,
		    client_connection);
		return;
	}

	/* Try to route the call through the connection hash table */
	if (route_call(callid, call))
		return;

	/* Unknown call from unknown phone - hang it up */
	ipc_answer_fast(callid, EHANGUP, 0, 0);
}

/** Fire all timeouts that expired. */
static void handle_expired_timeouts(void)
{
	struct timeval tv;
	awaiter_t *waiter;
	link_t *cur;

	gettimeofday(&tv, NULL);
	futex_down(&async_futex);

	cur = timeout_list.next;
	while (cur != &timeout_list) {
		waiter = list_get_instance(cur, awaiter_t, link);
		if (tv_gt(&waiter->expires, &tv))
			break;
		cur = cur->next;
		list_remove(&waiter->link);
		waiter->inlist = 0;
		waiter->timedout = 1;
		/*
 		 * Redundant condition?
 		 * The fibril should not be active when it gets here.
		 */
		if (!waiter->active) {
			waiter->active = 1;
			fibril_add_ready(waiter->fid);
		}
	}

	futex_up(&async_futex);
}

/** Endless loop dispatching incoming calls and answers.
 *
 * @return		Never returns.
 */
static int async_manager_worker(void)
{
	ipc_call_t call;
	ipc_callid_t callid;
	int timeout;
	awaiter_t *waiter;
	struct timeval tv;

	while (1) {
		if (fibril_schedule_next_adv(FIBRIL_FROM_MANAGER)) {
			futex_up(&async_futex); 
			/*
			 * async_futex is always held when entering a manager
			 * fibril.
			 */
			continue;
		}
		futex_down(&async_futex);
		if (!list_empty(&timeout_list)) {
			waiter = list_get_instance(timeout_list.next, awaiter_t,
			    link);
			gettimeofday(&tv, NULL);
			if (tv_gteq(&tv, &waiter->expires)) {
				futex_up(&async_futex);
				handle_expired_timeouts();
				continue;
			} else
				timeout = tv_sub(&waiter->expires, &tv);
		} else
			timeout = SYNCH_NO_TIMEOUT;
		futex_up(&async_futex);

		callid = ipc_wait_cycle(&call, timeout, SYNCH_FLAGS_NONE);

		if (!callid) {
			handle_expired_timeouts();
			continue;
		}

		if (callid & IPC_CALLID_ANSWERED) {
			continue;
		}

		handle_call(callid, &call);
	}
	
	return 0;
}

/** Function to start async_manager as a standalone fibril.
 * 
 * When more kernel threads are used, one async manager should exist per thread.
 *
 * @param arg		Unused.
 *
 * @return		Never returns.
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
	fid_t fid;

	fid = fibril_create(async_manager_fibril, NULL);
	fibril_add_manager(fid);
}

/** Remove one manager from manager list */
void async_destroy_manager(void)
{
	fibril_remove_manager();
}

/** Initialize the async framework.
 *
 * @return		Zero on success or an error code.
 */
int _async_init(void)
{
	if (!hash_table_create(&conn_hash_table, CONN_HASH_TABLE_CHAINS, 1,
	    &conn_hash_table_ops)) {
		printf("%s: cannot create hash table\n", "async");
		return ENOMEM;
	}
	
	return 0;
}

/** Reply received callback.
 *
 * This function is called whenever a reply for an asynchronous message sent out
 * by the asynchronous framework is received.
 *
 * Notify the fibril which is waiting for this message that it has arrived.
 *
 * @param private	Pointer to the asynchronous message record.
 * @param retval	Value returned in the answer.
 * @param data		Call data of the answer.
 */
static void reply_received(void *private, int retval, ipc_call_t *data)
{
	amsg_t *msg = (amsg_t *) private;

	msg->retval = retval;

	futex_down(&async_futex);
	/* Copy data after futex_down, just in case the call was detached */
	if (msg->dataptr)
		*msg->dataptr = *data; 

	write_barrier();
	/* Remove message from timeout list */
	if (msg->wdata.inlist)
		list_remove(&msg->wdata.link);
	msg->done = 1;
	if (!msg->wdata.active) {
		msg->wdata.active = 1;
		fibril_add_ready(msg->wdata.fid);
	}
	futex_up(&async_futex);
}

/** Send message and return id of the sent message.
 *
 * The return value can be used as input for async_wait() to wait for
 * completion.
 *
 * @param phoneid	Handle of the phone that will be used for the send.
 * @param method	Service-defined method.
 * @param arg1		Service-defined payload argument.
 * @param arg2		Service-defined payload argument.
 * @param dataptr	If non-NULL, storage where the reply data will be
 * 			stored.
 *
 * @return		Hash of the sent message.
 */
aid_t async_send_2(int phoneid, ipcarg_t method, ipcarg_t arg1, ipcarg_t arg2,
    ipc_call_t *dataptr)
{
	amsg_t *msg;

	if (in_interrupt_handler) {
		printf("Cannot send asynchronous request in interrupt "
		    "handler.\n");
		_exit(1);
	}

	msg = malloc(sizeof(*msg));
	msg->done = 0;
	msg->dataptr = dataptr;

	/* We may sleep in the next method, but it will use its own mechanism */
	msg->wdata.active = 1; 
				
	ipc_call_async_2(phoneid, method, arg1, arg2, msg, reply_received, 1);

	return (aid_t) msg;
}

/** Send message and return id of the sent message
 *
 * The return value can be used as input for async_wait() to wait for
 * completion.
 *
 * @param phoneid	Handle of the phone that will be used for the send.
 * @param method	Service-defined method.
 * @param arg1		Service-defined payload argument.
 * @param arg2		Service-defined payload argument.
 * @param arg3		Service-defined payload argument.
 * @param dataptr	If non-NULL, storage where the reply data will be
 * 			stored.
 *
 * @return		Hash of the sent message.
 */
aid_t async_send_3(int phoneid, ipcarg_t method, ipcarg_t arg1, ipcarg_t arg2,
    ipcarg_t arg3, ipc_call_t *dataptr)
{
	amsg_t *msg;

	if (in_interrupt_handler) {
		printf("Cannot send asynchronous request in interrupt "
		    "handler.\n");
		_exit(1);
	}

	msg = malloc(sizeof(*msg));
	msg->done = 0;
	msg->dataptr = dataptr;

	/* We may sleep in next method, but it will use its own mechanism */
	msg->wdata.active = 1; 
				
	ipc_call_async_3(phoneid, method, arg1, arg2, arg3, msg, reply_received,
	    1);

	return (aid_t) msg;
}

/** Wait for a message sent by the async framework.
 *
 * @param amsgid	Hash of the message to wait for.
 * @param retval	Pointer to storage where the retval of the answer will
 * 			be stored.
 */
void async_wait_for(aid_t amsgid, ipcarg_t *retval)
{
	amsg_t *msg = (amsg_t *) amsgid;

	futex_down(&async_futex);
	if (msg->done) {
		futex_up(&async_futex);
		goto done;
	}

	msg->wdata.fid = fibril_get_id();
	msg->wdata.active = 0;
	msg->wdata.inlist = 0;
	/* Leave the async_futex locked when entering this function */
	fibril_schedule_next_adv(FIBRIL_TO_MANAGER);
	/* futex is up automatically after fibril_schedule_next...*/
done:
	if (retval)
		*retval = msg->retval;
	free(msg);
}

/** Wait for a message sent by the async framework, timeout variant.
 *
 * @param amsgid	Hash of the message to wait for.
 * @param retval	Pointer to storage where the retval of the answer will
 * 			be stored.
 * @param timeout	Timeout in microseconds.
 *
 * @return		Zero on success, ETIMEOUT if the timeout has expired.
 */
int async_wait_timeout(aid_t amsgid, ipcarg_t *retval, suseconds_t timeout)
{
	amsg_t *msg = (amsg_t *) amsgid;

	/* TODO: Let it go through the event read at least once */
	if (timeout < 0)
		return ETIMEOUT;

	futex_down(&async_futex);
	if (msg->done) {
		futex_up(&async_futex);
		goto done;
	}

	gettimeofday(&msg->wdata.expires, NULL);
	tv_add(&msg->wdata.expires, timeout);

	msg->wdata.fid = fibril_get_id();
	msg->wdata.active = 0;
	insert_timeout(&msg->wdata);

	/* Leave the async_futex locked when entering this function */
	fibril_schedule_next_adv(FIBRIL_TO_MANAGER);
	/* futex is up automatically after fibril_schedule_next...*/

	if (!msg->done)
		return ETIMEOUT;

done:
	if (retval)
		*retval = msg->retval;
	free(msg);

	return 0;
}

/** Wait for specified time.
 *
 * The current fibril is suspended but the thread continues to execute.
 *
 * @param timeout	Duration of the wait in microseconds.
 */
void async_usleep(suseconds_t timeout)
{
	amsg_t *msg;
	
	if (in_interrupt_handler) {
		printf("Cannot call async_usleep in interrupt handler.\n");
		_exit(1);
	}

	msg = malloc(sizeof(*msg));
	if (!msg)
		return;

	msg->wdata.fid = fibril_get_id();
	msg->wdata.active = 0;

	gettimeofday(&msg->wdata.expires, NULL);
	tv_add(&msg->wdata.expires, timeout);

	futex_down(&async_futex);
	insert_timeout(&msg->wdata);
	/* Leave the async_futex locked when entering this function */
	fibril_schedule_next_adv(FIBRIL_TO_MANAGER);
	/* futex is up automatically after fibril_schedule_next_adv()...*/
	free(msg);
}

/** Setter for client_connection function pointer.
 *
 * @param conn		Function that will implement a new connection fibril.
 */
void async_set_client_connection(async_client_conn_t conn)
{
	client_connection = conn;
}

/** Setter for interrupt_received function pointer.
 *
 * @param conn		Function that will implement a new interrupt
 *			notification fibril.
 */
void async_set_interrupt_received(async_client_conn_t conn)
{
	interrupt_received = conn;
}

/* Primitive functions for simple communication */
void async_msg_3(int phoneid, ipcarg_t method, ipcarg_t arg1,
		 ipcarg_t arg2, ipcarg_t arg3)
{
	ipc_call_async_3(phoneid, method, arg1, arg2, arg3, NULL, NULL,
	    !in_interrupt_handler);
}

void async_msg_2(int phoneid, ipcarg_t method, ipcarg_t arg1, ipcarg_t arg2)
{
	ipc_call_async_2(phoneid, method, arg1, arg2, NULL, NULL,
	    !in_interrupt_handler);
}

/** @}
 */

