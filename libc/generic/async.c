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

/**
 * Asynchronous library
 *
 * The aim of this library is facilitating writing programs utilizing 
 * the asynchronous nature of Helenos IPC, yet using a normal way
 * of programming. 
 *
 * You should be able to write very simple multithreaded programs, 
 * the async framework will automatically take care of most synchronization
 * problems.
 *
 * Default semantics:
 * - send() - send asynchronously. If the kernel refuses to send more
 *            messages, [ try to get responses from kernel, if nothing
 *            found, might try synchronous ]
 *
 * Example of use:
 * 
 * 1) Multithreaded client application
 *  create_thread(thread1);
 *  create_thread(thread2);
 *  ...
 *  
 *  thread1() {
 *        conn = ipc_connect_me_to();
 *        c1 = send(conn);
 *        c2 = send(conn);
 *        wait_for(c1);
 *        wait_for(c2);
 *  }
 *
 *
 * 2) Multithreaded server application
 * main() {
 *      wait_for_connection(new_connection);
 * }
 * 
 *
 * new_connection(int connection) {
 *       accept(connection);
 *       msg = get_msg();
 *       handle(msg);
 *       answer(msg);
 *
 *       msg = get_msg();
 *       ....
 * }
 *
 * TODO: Detaching/joining dead psthreads?
 */
#include <futex.h>
#include <async.h>
#include <psthread.h>
#include <stdio.h>
#include <libadt/hash_table.h>
#include <libadt/list.h>
#include <ipc/ipc.h>
#include <assert.h>
#include <errno.h>

static atomic_t conn_futex = FUTEX_INITIALIZER;
static hash_table_t conn_hash_table;

typedef struct {
	link_t link;
	ipc_callid_t callid;
	ipc_call_t call;
} msg_t;

typedef struct {
	link_t link;
	ipcarg_t in_phone_hash;		/**< Incoming phone hash. */
	link_t msg_queue;              /**< Messages that should be delivered to this thread */
	pstid_t ptid;                /**< Thread associated with this connection */
	int active;                     /**< If this thread is currently active */
	/* Structures for connection opening packet */
	ipc_callid_t callid;
	ipc_call_t call;
} connection_t;

__thread connection_t *PS_connection;

/* Hash table functions */

#define CONN_HASH_TABLE_CHAINS	32

static hash_index_t conn_hash(unsigned long *key)
{
	assert(key);
	return ((*key) >> 4) % CONN_HASH_TABLE_CHAINS;
}

static int conn_compare(unsigned long key[], hash_count_t keys, link_t *item)
{
	connection_t *hs;

	hs = hash_table_get_instance(item, connection_t, link);
	
	return key[0] == hs->in_phone_hash;
}

static void conn_remove(link_t *item)
{
	free(hash_table_get_instance(item, connection_t, link));
}


/** Operations for NS hash table. */
static hash_table_operations_t conn_hash_table_ops = {
	.hash = conn_hash,
	.compare = conn_compare,
	.remove_callback = conn_remove
};

/** Try to route a call to an appropriate connection thread
 *
 */
static int route_call(ipc_callid_t callid, ipc_call_t *call)
{
	connection_t *conn;
	msg_t *msg;
	link_t *hlp;
	unsigned long key;

	futex_down(&conn_futex);

	key = call->in_phone_hash;
	hlp = hash_table_find(&conn_hash_table, &key);
	if (!hlp) {
		futex_up(&conn_futex);
		return 0;
	}
	conn = hash_table_get_instance(hlp, connection_t, link);

	msg = malloc(sizeof(*msg));
	msg->callid = callid;
	msg->call = *call;
	list_append(&msg->link, &conn->msg_queue);
	
	if (!conn->active) {
		conn->active = 1;
		psthread_add_ready(conn->ptid);
	}

	futex_up(&conn_futex);

	return 1;
}

/** Return new incoming message for current(thread-local) connection */
ipc_callid_t async_get_call(ipc_call_t *call)
{
	msg_t *msg;
	ipc_callid_t callid;
	connection_t *conn;
	
	futex_down(&conn_futex);

	conn = PS_connection;
	/* If nothing in queue, wait until something appears */
	if (list_empty(&conn->msg_queue)) {
		conn->active = 0;
		psthread_schedule_next_adv(PS_TO_MANAGER);
	}
	
	msg = list_get_instance(conn->msg_queue.next, msg_t, link);
	list_remove(&msg->link);
	callid = msg->callid;
	*call = msg->call;
	free(msg);
	
	futex_up(&conn_futex);
	return callid;
}

/** Thread function that gets created on new connection
 *
 * This function is defined as a weak symbol - to be redefined in
 * user code.
 */
void client_connection(ipc_callid_t callid, ipc_call_t *call)
{
	ipc_answer_fast(callid, ENOENT, 0, 0);
}

/** Wrapper for client connection thread
 *
 * When new connection arrives, thread with this function is created.
 * It calls client_connection and does final cleanup.
 *
 * @parameter arg Connection structure pointer
 */
static int connection_thread(void  *arg)
{
	unsigned long key;
	msg_t *msg;

	/* Setup thread local connection pointer */
	PS_connection = (connection_t *)arg;
	client_connection(PS_connection->callid, &PS_connection->call);

	/* Remove myself from connection hash table */
	futex_down(&conn_futex);
	key = PS_connection->in_phone_hash;
	hash_table_remove(&conn_hash_table, &key, 1);
	futex_up(&conn_futex);
	/* Answer all remaining messages with ehangup */
	while (!list_empty(&PS_connection->msg_queue)) {
		msg = list_get_instance(PS_connection->msg_queue.next, msg_t, link);
		list_remove(&msg->link);
		ipc_answer_fast(msg->callid, EHANGUP, 0, 0);
		free(msg);
	}
}

/** Create new thread for a new connection 
 *
 * Creates new thread for connection, fills in connection
 * structures and inserts it into the hash table, so that
 * later we can easily do routing of messages to particular
 * threads.
 */
static void new_connection(ipc_callid_t callid, ipc_call_t *call)
{
	pstid_t ptid;
	connection_t *conn;
	unsigned long key;

	conn = malloc(sizeof(*conn));
	if (!conn) {
		ipc_answer_fast(callid, ENOMEM, 0, 0);
		return;
	}
	conn->in_phone_hash = IPC_GET_ARG3(*call);
	list_initialize(&conn->msg_queue);
	conn->ptid = psthread_create(connection_thread, conn);
	conn->callid = callid;
	conn->call = *call;
	conn->active = 1; /* We will activate it asap */
	list_initialize(&conn->link);
	if (!conn->ptid) {
		free(conn);
		ipc_answer_fast(callid, ENOMEM, 0, 0);
		return;
	}
	key = conn->in_phone_hash;
	futex_down(&conn_futex);
	/* Add connection to hash table */
	hash_table_insert(&conn_hash_table, &key, &conn->link);
	futex_up(&conn_futex);

	psthread_add_ready(conn->ptid);
}

/** Handle call to a task */
static void handle_call(ipc_callid_t callid, ipc_call_t *call)
{
	if (route_call(callid, call))
		return;

	switch (IPC_GET_METHOD(*call)) {
	case IPC_M_INTERRUPT:
		break;
	case IPC_M_CONNECT_ME_TO:
		/* Open new connection with thread etc. */
		new_connection(callid, call);
		break;
	default:
		ipc_answer_fast(callid, EHANGUP, 0, 0);
	}
}

/** Endless loop dispatching incoming calls and answers */
int async_manager()
{
	ipc_call_t call;
	ipc_callid_t callid;

	while (1) {
		if (psthread_schedule_next_adv(PS_FROM_MANAGER)) {
			futex_up(&conn_futex); /* conn_futex is always held
						* when entering manager thread
						*/
			continue;
		}
		callid = ipc_wait_cycle(&call,SYNCH_NO_TIMEOUT,SYNCH_BLOCKING);

		if (callid & IPC_CALLID_ANSWERED)
			continue;
		handle_call(callid, &call);
	}
}

/** Function to start async_manager as a standalone thread 
 * 
 * When more kernel threads are used, one async manager should
 * exist per thread. The particular implementation may change,
 * currently one async_manager is started automatically per kernel
 * thread except main thread. 
 */
static int async_manager_thread(void *arg)
{
	futex_up(&conn_futex); /* conn_futex is always locked when entering
				* manager */
	async_manager();
}

/** Add one manager to manager list */
void async_create_manager(void)
{
	pstid_t ptid;

	ptid = psthread_create(async_manager_thread, NULL);
	psthread_add_manager(ptid);
}

/** Remove one manager from manager list */
void async_destroy_manager(void)
{
	psthread_remove_manager();
}

/** Initialize internal structures needed for async manager */
int _async_init(void)
{
	if (!hash_table_create(&conn_hash_table, CONN_HASH_TABLE_CHAINS, 1, &conn_hash_table_ops)) {
		printf("%s: cannot create hash table\n", "async");
		return ENOMEM;
	}
	
}
