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
 *      async_manager();
 * }
 * 
 *
 * client_connection(icallid, *icall) {
 *       if (want_refuse) {
 *           ipc_answer_fast(icallid, ELIMIT, 0, 0);
 *           return;
 *       }
 *       ipc_answer_fast(icallid, 0, 0, 0);
 *
 *       callid = async_get_call(&call);
 *       handle(callid, call);
 *       ipc_answer_fast(callid, 1,2,3);
 *
 *       callid = async_get_call(&call);
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
#include <time.h>
#include <arch/barrier.h>

atomic_t async_futex = FUTEX_INITIALIZER;
static hash_table_t conn_hash_table;
static LIST_INITIALIZE(timeout_list);

typedef struct {
	struct timeval expires;      /**< Expiration time for waiting thread */
	int inlist;             /**< If true, this struct is in timeout list */
	link_t link;

	pstid_t ptid;                /**< Thread waiting for this message */
	int active;                  /**< If this thread is currently active */
	int timedout;                /**< If true, we timed out */
} awaiter_t;

typedef struct {
	awaiter_t wdata;

	int done;                    /**< If reply was received */
	ipc_call_t *dataptr;         /**< Pointer where the answer data
				      *   is stored */
	ipcarg_t retval;
} amsg_t;

typedef struct {
	link_t link;
	ipc_callid_t callid;
	ipc_call_t call;
} msg_t;

typedef struct {
	awaiter_t wdata;

	link_t link;             /**< Hash table link */
	ipcarg_t in_phone_hash;  /**< Incoming phone hash. */
	link_t msg_queue;        /**< Messages that should be delivered to this thread */
	/* Structures for connection opening packet */
	ipc_callid_t callid;
	ipc_call_t call;
	void (*cthread)(ipc_callid_t,ipc_call_t *);
} connection_t;


__thread connection_t *PS_connection;

static void default_client_connection(ipc_callid_t callid, ipc_call_t *call);
static async_client_conn_t client_connection = default_client_connection;

/** Add microseconds to give timeval */
static void tv_add(struct timeval *tv, suseconds_t usecs)
{
	tv->tv_sec += usecs / 1000000;
	tv->tv_usec += usecs % 1000000;
	if (tv->tv_usec > 1000000) {
		tv->tv_sec++;
		tv->tv_usec -= 1000000;
	}
}

/** Subtract 2 timevals, return microseconds difference */
static suseconds_t tv_sub(struct timeval *tv1, struct timeval *tv2)
{
	suseconds_t result;

	result = tv1->tv_usec - tv2->tv_usec;
	result += (tv1->tv_sec - tv2->tv_sec) * 1000000;

	return result;
}

/** Compare timeval
 *
 * @return 1 if tv1 > tv2, otherwise 0
 */
static int tv_gt(struct timeval *tv1, struct timeval *tv2)
{
	if (tv1->tv_sec > tv2->tv_sec)
		return 1;
	if (tv1->tv_sec == tv2->tv_sec && tv1->tv_usec > tv2->tv_usec)
		return 1;
	return 0;
}
static int tv_gteq(struct timeval *tv1, struct timeval *tv2)
{
	if (tv1->tv_sec > tv2->tv_sec)
		return 1;
	if (tv1->tv_sec == tv2->tv_sec && tv1->tv_usec >= tv2->tv_usec)
		return 1;
	return 0;
}

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

/** Insert sort timeout msg into timeouts list
 *
 */
static void insert_timeout(awaiter_t *wd)
{
	link_t *tmp;
	awaiter_t *cur;

	wd->timedout = 0;

	tmp = timeout_list.next;
	while (tmp != &timeout_list) {
		cur = list_get_instance(tmp, awaiter_t, link);
		if (tv_gteq(&cur->expires, &wd->expires))
			break;
		tmp = tmp->next;
	}
	list_append(&wd->link, tmp);
}

/*************************************************/

/** Try to route a call to an appropriate connection thread
 *
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
	
	/* If the call is waiting for event, run it */
	if (!conn->wdata.active) {
		/* If in timeout list, remove it */
		if (conn->wdata.inlist) {
			conn->wdata.inlist = 0;
			list_remove(&conn->wdata.link);
		}
		conn->wdata.active = 1;
		psthread_add_ready(conn->wdata.ptid);
	}

	futex_up(&async_futex);

	return 1;
}

/** Return new incoming message for current(thread-local) connection */
ipc_callid_t async_get_call_timeout(ipc_call_t *call, suseconds_t usecs)
{
	msg_t *msg;
	ipc_callid_t callid;
	connection_t *conn;
	
	assert(PS_connection);
	/* GCC 4.1.0 coughs on PS_connection-> dereference,
	 * GCC 4.1.1 happilly puts the rdhwr instruction in delay slot.
	 *           I would never expect to find so many errors in 
	 *           compiler *($&$(*&$
	 */
	conn = PS_connection; 

	if (usecs < 0) /* TODO: let it get through the ipc_call once */
		return 0;

	futex_down(&async_futex);

	if (usecs) {
		gettimeofday(&conn->wdata.expires, NULL);
		tv_add(&conn->wdata.expires, usecs);
	} else {
		conn->wdata.inlist = 0;
	}
	/* If nothing in queue, wait until something appears */
	while (list_empty(&conn->msg_queue)) {
		if (usecs) {
			conn->wdata.inlist = 1;
			insert_timeout(&conn->wdata);
		}
		conn->wdata.active = 0;
		psthread_schedule_next_adv(PS_TO_MANAGER);
		/* Futex is up after getting back from async_manager 
		 * get it again */
		futex_down(&async_futex);
		if (usecs && conn->wdata.timedout && \
		    list_empty(&conn->msg_queue)) {
			/* If we timed out-> exit */
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

/** Thread function that gets created on new connection
 *
 * This function is defined as a weak symbol - to be redefined in
 * user code.
 */
static void default_client_connection(ipc_callid_t callid, ipc_call_t *call)
{
	ipc_answer_fast(callid, ENOENT, 0, 0);
}

/** Function that gets called on interrupt receival
 *
 * This function is defined as a weak symbol - to be redefined in
 * user code.
 */
void interrupt_received(ipc_call_t *call)
{
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
	PS_connection->cthread(PS_connection->callid, &PS_connection->call);

	/* Remove myself from connection hash table */
	futex_down(&async_futex);
	key = PS_connection->in_phone_hash;
	hash_table_remove(&conn_hash_table, &key, 1);
	futex_up(&async_futex);
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
 *
 * @param in_phone_hash Identification of the incoming connection
 * @param callid Callid of the IPC_M_CONNECT_ME_TO packet
 * @param call Call data of the opening packet
 * @param cthread Thread function that should be called upon
 *                opening the connection
 * @return New thread id
 */
pstid_t async_new_connection(ipcarg_t in_phone_hash,ipc_callid_t callid, 
			     ipc_call_t *call,
			     void (*cthread)(ipc_callid_t,ipc_call_t *))
{
	pstid_t ptid;
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
	if (call)
		conn->call = *call;
	conn->wdata.active = 1; /* We will activate it asap */
	conn->cthread = cthread;

	conn->wdata.ptid = psthread_create(connection_thread, conn);
	if (!conn->wdata.ptid) {
		free(conn);
		ipc_answer_fast(callid, ENOMEM, 0, 0);
		return NULL;
	}
	/* Add connection to hash table */
	key = conn->in_phone_hash;
	futex_down(&async_futex);
	hash_table_insert(&conn_hash_table, &key, &conn->link);
	futex_up(&async_futex);

	psthread_add_ready(conn->wdata.ptid);

	return conn->wdata.ptid;
}

/** Handle call that was received */
static void handle_call(ipc_callid_t callid, ipc_call_t *call)
{
	/* Unrouted call - do some default behaviour */
	switch (IPC_GET_METHOD(*call)) {
	case IPC_M_INTERRUPT:
		interrupt_received(call);
		return;
	case IPC_M_CONNECT_ME_TO:
		/* Open new connection with thread etc. */
		async_new_connection(IPC_GET_ARG3(*call), callid, call, client_connection);
		return;
	}

	/* Try to route call through connection tables */
	if (route_call(callid, call))
		return;

	/* Unknown call from unknown phone - hang it up */
	ipc_answer_fast(callid, EHANGUP, 0, 0);
}

/** Fire all timeouts that expired 
 *
 */
static void handle_expired_timeouts(void)
{
	struct timeval tv;
	awaiter_t *waiter;
	link_t *cur;

	gettimeofday(&tv,NULL);
	futex_down(&async_futex);

	cur = timeout_list.next;
	while (cur != &timeout_list) {
		waiter = list_get_instance(cur,awaiter_t,link);
		if (tv_gt(&waiter->expires, &tv))
			break;
		cur = cur->next;
		list_remove(&waiter->link);
		waiter->inlist = 0;
		waiter->timedout = 1;
		/* Redundant condition? The thread should not
		 * be active when it gets here.
		 */
		if (!waiter->active) {
			waiter->active = 1;
			psthread_add_ready(waiter->ptid);
		}
	}

	futex_up(&async_futex);
}

/** Endless loop dispatching incoming calls and answers */
int async_manager(void)
{
	ipc_call_t call;
	ipc_callid_t callid;
	int timeout;
	awaiter_t *waiter;
	struct timeval tv;

	while (1) {
		if (psthread_schedule_next_adv(PS_FROM_MANAGER)) {
			futex_up(&async_futex); /* async_futex is always held
						* when entering manager thread
						*/
			continue;
		}
		futex_down(&async_futex);
		if (!list_empty(&timeout_list)) {
			waiter = list_get_instance(timeout_list.next,awaiter_t,link);
			gettimeofday(&tv,NULL);
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
	futex_up(&async_futex); /* async_futex is always locked when entering
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

/** IPC handler for messages in async framework
 *
 * Notify thread that is waiting for this message, that it arrived
 */
static void reply_received(void *private, int retval,
			   ipc_call_t *data)
{
	amsg_t *msg = (amsg_t *) private;

	msg->retval = retval;

	futex_down(&async_futex);
	/* Copy data after futex_down, just in case the
	 * call was detached 
	 */
	if (msg->dataptr)
		*msg->dataptr = *data; 

	write_barrier();
	/* Remove message from timeout list */
	if (msg->wdata.inlist)
		list_remove(&msg->wdata.link);
	msg->done = 1;
	if (! msg->wdata.active) {
		msg->wdata.active = 1;
		psthread_add_ready(msg->wdata.ptid);
	}
	futex_up(&async_futex);
}

/** Send message and return id of the sent message
 *
 * The return value can be used as input for async_wait() to wait
 * for completion.
 */
aid_t async_send_2(int phoneid, ipcarg_t method, ipcarg_t arg1, ipcarg_t arg2,
		   ipc_call_t *dataptr)
{
	amsg_t *msg;

	msg = malloc(sizeof(*msg));
	msg->done = 0;
	msg->dataptr = dataptr;

	msg->wdata.active = 1; /* We may sleep in next method, but it
				* will use it's own mechanism */
	ipc_call_async_2(phoneid,method,arg1,arg2,msg,reply_received,1);

	return (aid_t) msg;
}

/** Send message and return id of the sent message
 *
 * The return value can be used as input for async_wait() to wait
 * for completion.
 */
aid_t async_send_3(int phoneid, ipcarg_t method, ipcarg_t arg1, ipcarg_t arg2,
		   ipcarg_t arg3, ipc_call_t *dataptr)
{
	amsg_t *msg;

	msg = malloc(sizeof(*msg));
	msg->done = 0;
	msg->dataptr = dataptr;

	msg->wdata.active = 1; /* We may sleep in next method, but it
				* will use it's own mechanism */
	ipc_call_async_3(phoneid,method,arg1,arg2,arg3, msg,reply_received,1);

	return (aid_t) msg;
}

/** Wait for a message sent by async framework
 *
 * @param amsgid Message ID to wait for
 * @param retval Pointer to variable where will be stored retval
 *               of the answered message. If NULL, it is ignored.
 *
 */
void async_wait_for(aid_t amsgid, ipcarg_t *retval)
{
	amsg_t *msg = (amsg_t *) amsgid;
	connection_t *conn;

	futex_down(&async_futex);
	if (msg->done) {
		futex_up(&async_futex);
		goto done;
	}

	msg->wdata.ptid = psthread_get_id();
	msg->wdata.active = 0;
	msg->wdata.inlist = 0;
	/* Leave locked async_futex when entering this function */
	psthread_schedule_next_adv(PS_TO_MANAGER);
	/* futex is up automatically after psthread_schedule_next...*/
done:
	if (retval)
		*retval = msg->retval;
	free(msg);
}

/** Wait for a message sent by async framework with timeout
 *
 * @param amsgid Message ID to wait for
 * @param retval Pointer to variable where will be stored retval
 *               of the answered message. If NULL, it is ignored.
 * @param timeout Timeout in usecs
 * @return 0 on success, ETIMEOUT if timeout expired
 *
 */
int async_wait_timeout(aid_t amsgid, ipcarg_t *retval, suseconds_t timeout)
{
	amsg_t *msg = (amsg_t *) amsgid;
	connection_t *conn;

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

	msg->wdata.ptid = psthread_get_id();
	msg->wdata.active = 0;
	msg->wdata.inlist = 1;

	insert_timeout(&msg->wdata);

	/* Leave locked async_futex when entering this function */
	psthread_schedule_next_adv(PS_TO_MANAGER);
	/* futex is up automatically after psthread_schedule_next...*/

	if (!msg->done)
		return ETIMEOUT;

done:
	if (retval)
		*retval = msg->retval;
	free(msg);

	return 0;
}

/** Wait specified time, but in the meantime handle incoming events
 *
 * @param timeout Time in microseconds to wait
 */
void async_usleep(suseconds_t timeout)
{
	amsg_t *msg;
	
	msg = malloc(sizeof(*msg));
	if (!msg)
		return;

	msg->wdata.ptid = psthread_get_id();
	msg->wdata.inlist = 1;
	msg->wdata.active = 0;

	gettimeofday(&msg->wdata.expires, NULL);
	tv_add(&msg->wdata.expires, timeout);

	futex_down(&async_futex);
	insert_timeout(&msg->wdata);
	/* Leave locked async_futex when entering this function */
	psthread_schedule_next_adv(PS_TO_MANAGER);
	/* futex is up automatically after psthread_schedule_next...*/
	free(msg);
}

/** Set function that is called, IPC_M_CONNECT_ME_TO is received
 *
 * @param conn Function that will form new psthread.
 */
void async_set_client_connection(async_client_conn_t conn)
{
	client_connection = conn;
}
