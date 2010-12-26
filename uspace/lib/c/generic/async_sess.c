/*
 * Copyright (c) 2010 Jakub Jermar
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
 * This file implements simple session support for the async framework.
 *
 * By the term 'session', we mean a logical data path between a client and a
 * server over which the client can perform multiple concurrent transactions.
 * Each transaction consists of one or more requests (IPC calls) which can
 * be potentially blocking.
 *
 * Clients and servers are naturally connected using IPC phones, thus an IPC
 * phone represents a session between a client and a server. In one
 * session, there can be many outstanding transactions. In the current
 * implementation each concurrent transaction takes place over a different
 * connection (there can be at most one active transaction per connection).
 *
 * Sessions make it useful for a client or client API to support concurrent
 * requests, independent of the actual implementation. Sessions provide
 * an abstract interface to concurrent IPC communication. This is especially
 * useful for client API stubs that aim to be reentrant (i.e. that allow
 * themselves to be called from different fibrils and threads concurrently).
 *
 * There are several possible implementations of sessions. This implementation
 * uses additional phones to represent sessions. Using phones both for the
 * session and also for its transactions/connections has several advantages:
 *
 * - to make a series of transactions over a session, the client can continue to
 *   use the existing async framework APIs
 * - the server supports sessions by the virtue of spawning a new connection
 *   fibril, just as it does for every new connection even without sessions
 * - the implementation is pretty straightforward; a very naive implementation
 *   would be to make each transaction using a fresh phone (that is what we
 *   have done in the past); a slightly better approach would be to cache
 *   connections so that they can be reused by a later transaction within
 *   the same session (that is what this implementation does)
 *
 * The main disadvantages of using phones to represent sessions are:
 *
 * - if there are too many transactions (even cached ones), the task may hit its
 *   limit on the maximum number of connected phones, which could prevent the
 *   task from making new IPC connections to other tasks
 * - if there are too many IPC connections already, it may be impossible to
 *   create a transaction by connecting a new phone thanks to the task's limit on
 *   the maximum number of connected phones
 *
 * These problems can be alleviated by increasing the limit on the maximum
 * number of connected phones to some reasonable value and by limiting the number
 * of cached connections to some fraction of this limit.
 *
 * The cache itself has a mechanism to close some number of unused phones if a
 * new phone cannot be connected, but the outer world currently does not have a
 * way to ask the phone cache to shrink.
 *
 * To minimize the confusion stemming from the fact that we use phones for two
 * things (the session itself and also one for each data connection), this file
 * makes the distinction by using the term 'session phone' for the former and
 * 'data phone' for the latter. Under the hood, all phones remain equal,
 * of course.
 *
 * There is a small inefficiency in that the cache repeatedly allocates and
 * deallocates the conn_node_t structures when in fact it could keep the
 * allocated structures around and reuse them later. But such a solution would
 * be effectively implementing a poor man's slab allocator while it would be
 * better to have the slab allocator ported to uspace so that everyone could
 * benefit from it.
 */

#include <async_sess.h>
#include <ipc/ipc.h>
#include <fibril_synch.h>
#include <adt/list.h>
#include <adt/hash_table.h>
#include <malloc.h>
#include <errno.h>
#include <assert.h>

typedef struct {
	link_t conn_link;	/**< Link for the list of connections. */
	link_t global_link;	/**< Link for the global list of phones. */
	int data_phone;		/**< Connected data phone. */
} conn_node_t;

/**
 * Mutex protecting the inactive_conn_head list and the session_hash hash table.
 */
static fibril_mutex_t async_sess_mutex;

/**
 * List of all currently inactive connections.
 */
static LIST_INITIALIZE(inactive_conn_head);

/**
 * List of all existing sessions.
 */
//static LIST_INITIALIZE(session_list);

/** Initialize the async_sess subsystem.
 *
 * Needs to be called prior to any other interface in this file.
 */
void _async_sess_init(void)
{
	fibril_mutex_initialize(&async_sess_mutex);
	list_initialize(&inactive_conn_head);
}

void async_session_create(async_sess_t *sess, int phone)
{
	sess->sess_phone = phone;
	list_initialize(&sess->conn_head);
}

void async_session_destroy(async_sess_t *sess)
{
	sess->sess_phone = -1;
	/* todo */
}

static void conn_node_initialize(conn_node_t *conn)
{
	link_initialize(&conn->conn_link);
	link_initialize(&conn->global_link);
	conn->data_phone = -1;
}

/** Start new transaction in a session.
 *
 * @param sess_phone	Session.
 * @return		Phone representing the new transaction or a negative error
 *			code.
 */
int async_transaction_begin(async_sess_t *sess)
{
	conn_node_t *conn;
	int data_phone;

	fibril_mutex_lock(&async_sess_mutex);

	if (!list_empty(&sess->conn_head)) {
		/*
		 * There are inactive connections in the session.
		 */
		conn = list_get_instance(sess->conn_head.next, conn_node_t,
		    conn_link);
		list_remove(&conn->conn_link);
		list_remove(&conn->global_link);
		
		data_phone = conn->data_phone;
		free(conn);
	} else {
		/*
		 * There are no available connections in the session.
		 * Make a one-time attempt to connect a new data phone.
		 */
retry:
		data_phone = async_connect_me_to(sess->sess_phone, 0, 0, 0);
		if (data_phone >= 0) {
			/* success, do nothing */
		} else if (!list_empty(&inactive_conn_head)) {
			/*
			 * We did not manage to connect a new phone. But we can
			 * try to close some of the currently inactive
			 * connections in other sessions and try again.
			 */
			conn = list_get_instance(inactive_conn_head.next,
			    conn_node_t, global_link);
			list_remove(&conn->global_link);
			list_remove(&conn->conn_link);
			data_phone = conn->data_phone;
			free(conn);
			ipc_hangup(data_phone);
			goto retry;
		} else {
			/*
			 * This is unfortunate. We failed both to find a cached
			 * connection or to create a new one even after cleaning up
			 * the cache. This is most likely due to too many
			 * open sessions (connected session phones).
			 */
			data_phone = ELIMIT;
		}
	}

	fibril_mutex_unlock(&async_sess_mutex);
	return data_phone;
}

/** Finish a transaction.
 *
 * @param sess		Session.
 * @param data_phone	Phone representing the transaction within the session.
 */
void async_transaction_end(async_sess_t *sess, int data_phone)
{
	conn_node_t *conn;

	fibril_mutex_lock(&async_sess_mutex);
	conn = (conn_node_t *) malloc(sizeof(conn_node_t));
	if (!conn) {
		/*
		 * Being unable to remember the connected data phone here
		 * means that we simply hang up.
		 */
		fibril_mutex_unlock(&async_sess_mutex);
		ipc_hangup(data_phone);
		return;
	}

	conn_node_initialize(conn);
	conn->data_phone = data_phone;
	list_append(&conn->conn_link, &sess->conn_head);
	list_append(&conn->global_link, &inactive_conn_head);
	fibril_mutex_unlock(&async_sess_mutex);
}

/** @}
 */
