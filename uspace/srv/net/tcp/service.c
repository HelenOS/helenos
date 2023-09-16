/*
 * Copyright (c) 2023 Jiri Svoboda
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

/** @addtogroup tcp
 * @{
 */

/**
 * @file HelenOS service implementation
 */

#include <async.h>
#include <errno.h>
#include <str_error.h>
#include <inet/endpoint.h>
#include <inet/inet.h>
#include <io/log.h>
#include <ipc/services.h>
#include <ipc/tcp.h>
#include <loc.h>
#include <macros.h>
#include <mem.h>
#include <stdlib.h>

#include "conn.h"
#include "service.h"
#include "tcp_type.h"
#include "ucall.h"

#define NAME "tcp"

/** Maximum amount of data transferred in one send call */
#define MAX_MSG_SIZE DATA_XFER_LIMIT

static void tcp_ev_data(tcp_cconn_t *);
static void tcp_ev_connected(tcp_cconn_t *);
static void tcp_ev_conn_failed(tcp_cconn_t *);
static void tcp_ev_conn_reset(tcp_cconn_t *);
static void tcp_ev_new_conn(tcp_clst_t *, tcp_cconn_t *);

static void tcp_service_cstate_change(tcp_conn_t *, void *, tcp_cstate_t);
static void tcp_service_recv_data(tcp_conn_t *, void *);
static void tcp_service_lst_cstate_change(tcp_conn_t *, void *, tcp_cstate_t);

static errno_t tcp_cconn_create(tcp_client_t *, tcp_conn_t *, tcp_cconn_t **);

/** Connection callbacks to tie us to lower layer */
static tcp_cb_t tcp_service_cb = {
	.cstate_change = tcp_service_cstate_change,
	.recv_data = tcp_service_recv_data
};

/** Sentinel connection callbacks to tie us to lower layer */
static tcp_cb_t tcp_service_lst_cb = {
	.cstate_change = tcp_service_lst_cstate_change,
	.recv_data = NULL
};

/** Connection state has changed.
 *
 * @param conn      Connection
 * @param arg       Argument (not used)
 * @param old_state Previous connection state
 */
static void tcp_service_cstate_change(tcp_conn_t *conn, void *arg,
    tcp_cstate_t old_state)
{
	tcp_cstate_t nstate;
	tcp_cconn_t *cconn;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_service_cstate_change()");
	nstate = conn->cstate;
	cconn = tcp_uc_get_userptr(conn);

	if ((old_state == st_syn_sent || old_state == st_syn_received) &&
	    (nstate == st_established)) {
		/* Connection established */
		tcp_ev_connected(cconn);
	}

	if (old_state != st_closed && nstate == st_closed && conn->reset) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_service_cstate_change: "
		    "Connection reset");
		/* Connection reset */
		tcp_ev_conn_reset(cconn);
	} else {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_service_cstate_change: "
		    "old_state=%d nstate=%d conn->reset=%d",
		    old_state, nstate, conn->reset);
	}

	/* XXX Failed to establish connection */
	if (0)
		tcp_ev_conn_failed(cconn);
}

/** Sentinel connection state has changed.
 *
 * @param conn      Connection
 * @param arg       Argument (not used)
 * @param old_state Previous connection state
 */
static void tcp_service_lst_cstate_change(tcp_conn_t *conn, void *arg,
    tcp_cstate_t old_state)
{
	tcp_cstate_t nstate;
	tcp_clst_t *clst;
	tcp_cconn_t *cconn;
	inet_ep2_t epp;
	errno_t rc;
	tcp_error_t trc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_service_lst_cstate_change()");
	nstate = conn->cstate;
	clst = tcp_uc_get_userptr(conn);

	if ((old_state == st_syn_sent || old_state == st_syn_received) &&
	    (nstate == st_established)) {
		/* Connection established */
		clst->conn = NULL;

		rc = tcp_cconn_create(clst->client, conn, &cconn);
		if (rc != EOK) {
			/* XXX Could not create client connection */
			return;
		}

		/* XXX Is there a race here (i.e. the connection is already active)? */
		tcp_uc_set_cb(conn, &tcp_service_cb, cconn);

		/* New incoming connection */
		tcp_ev_new_conn(clst, cconn);
	}

	if (old_state != st_closed && nstate == st_closed && conn->reset) {
		/* Connection reset */
		/* XXX */
	}

	/* XXX Failed to establish connection */
	if (0) {
		/* XXX */
	}

	/* Replenish sentinel connection */

	inet_ep2_init(&epp);
	epp.local = clst->elocal;

	trc = tcp_uc_open(&epp, ap_passive, tcp_open_nonblock,
	    &conn);
	if (trc != TCP_EOK) {
		/* XXX Could not replenish connection */
		return;
	}

	conn->name = (char *) "s";
	clst->conn = conn;

	/* XXX Is there a race here (i.e. the connection is already active)? */
	tcp_uc_set_cb(conn, &tcp_service_lst_cb, clst);
}

/** Received data became available on connection.
 *
 * @param conn Connection
 * @param arg  Client connection
 */
static void tcp_service_recv_data(tcp_conn_t *conn, void *arg)
{
	tcp_cconn_t *cconn = (tcp_cconn_t *)arg;

	tcp_ev_data(cconn);
}

/** Send 'data' event to client.
 *
 * @param cconn Client connection
 */
static void tcp_ev_data(tcp_cconn_t *cconn)
{
	async_exch_t *exch;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_ev_data()");

	log_msg(LOG_DEFAULT, LVL_DEBUG, "client=%p\n", cconn->client);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "sess=%p\n", cconn->client->sess);

	exch = async_exchange_begin(cconn->client->sess);
	aid_t req = async_send_1(exch, TCP_EV_DATA, cconn->id, NULL);
	async_exchange_end(exch);

	async_forget(req);
}

/** Send 'connected' event to client.
 *
 * @param cconn Client connection
 */
static void tcp_ev_connected(tcp_cconn_t *cconn)
{
	async_exch_t *exch;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_ev_connected()");

	exch = async_exchange_begin(cconn->client->sess);
	aid_t req = async_send_1(exch, TCP_EV_CONNECTED, cconn->id, NULL);
	async_exchange_end(exch);

	async_forget(req);
}

/** Send 'conn_failed' event to client.
 *
 * @param cconn Client connection
 */
static void tcp_ev_conn_failed(tcp_cconn_t *cconn)
{
	async_exch_t *exch;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_ev_conn_failed()");

	exch = async_exchange_begin(cconn->client->sess);
	aid_t req = async_send_1(exch, TCP_EV_CONN_FAILED, cconn->id, NULL);
	async_exchange_end(exch);

	async_forget(req);
}

/** Send 'conn_reset' event to client.
 *
 * @param cconn Client connection
 */
static void tcp_ev_conn_reset(tcp_cconn_t *cconn)
{
	async_exch_t *exch;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_ev_conn_reset()");

	exch = async_exchange_begin(cconn->client->sess);
	aid_t req = async_send_1(exch, TCP_EV_CONN_RESET, cconn->id, NULL);
	async_exchange_end(exch);

	async_forget(req);
}

/** Send 'new_conn' event to client.
 *
 * @param clst Client listener that received the connection
 * @param cconn New client connection
 */
static void tcp_ev_new_conn(tcp_clst_t *clst, tcp_cconn_t *cconn)
{
	async_exch_t *exch;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_ev_new_conn()");

	exch = async_exchange_begin(clst->client->sess);
	aid_t req = async_send_2(exch, TCP_EV_NEW_CONN, clst->id, cconn->id,
	    NULL);
	async_exchange_end(exch);

	async_forget(req);
}

/** Create client connection.
 *
 * This effectively adds a connection into a client's namespace.
 *
 * @param client TCP client
 * @param conn   Connection
 * @param rcconn Place to store pointer to new client connection
 *
 * @return EOK on success or ENOMEM if out of memory
 */
static errno_t tcp_cconn_create(tcp_client_t *client, tcp_conn_t *conn,
    tcp_cconn_t **rcconn)
{
	tcp_cconn_t *cconn;
	sysarg_t id;

	cconn = calloc(1, sizeof(tcp_cconn_t));
	if (cconn == NULL)
		return ENOMEM;

	/* Allocate new ID */
	id = 0;
	list_foreach (client->cconn, lclient, tcp_cconn_t, cconn) {
		if (cconn->id >= id)
			id = cconn->id + 1;
	}

	cconn->id = id;
	cconn->client = client;
	cconn->conn = conn;

	list_append(&cconn->lclient, &client->cconn);
	*rcconn = cconn;
	return EOK;
}

/** Destroy client connection.
 *
 * @param cconn Client connection
 */
static void tcp_cconn_destroy(tcp_cconn_t *cconn)
{
	list_remove(&cconn->lclient);
	free(cconn);
}

/** Create client listener.
 *
 * Create client listener based on sentinel connection.
 * XXX Implement actual listener in protocol core
 *
 * @param client TCP client
 * @param conn   Sentinel connection
 * @param rclst  Place to store pointer to new client listener
 *
 * @return EOK on success or ENOMEM if out of memory
 */
static errno_t tcp_clistener_create(tcp_client_t *client, tcp_conn_t *conn,
    tcp_clst_t **rclst)
{
	tcp_clst_t *clst;
	sysarg_t id;

	clst = calloc(1, sizeof(tcp_clst_t));
	if (clst == NULL)
		return ENOMEM;

	/* Allocate new ID */
	id = 0;
	list_foreach (client->clst, lclient, tcp_clst_t, clst) {
		if (clst->id >= id)
			id = clst->id + 1;
	}

	clst->id = id;
	clst->client = client;
	clst->conn = conn;

	list_append(&clst->lclient, &client->clst);
	*rclst = clst;
	return EOK;
}

/** Destroy client listener.
 *
 * @param clst Client listener
 */
static void tcp_clistener_destroy(tcp_clst_t *clst)
{
	list_remove(&clst->lclient);
	free(clst);
}

/** Get client connection by ID.
 *
 * @param client Client
 * @param id     Client connection ID
 * @param rcconn Place to store pointer to client connection
 *
 * @return EOK on success, ENOENT if no client connection with the given ID
 *         is found.
 */
static errno_t tcp_cconn_get(tcp_client_t *client, sysarg_t id,
    tcp_cconn_t **rcconn)
{
	list_foreach (client->cconn, lclient, tcp_cconn_t, cconn) {
		if (cconn->id == id) {
			*rcconn = cconn;
			return EOK;
		}
	}

	return ENOENT;
}

/** Get client listener by ID.
 *
 * @param client Client
 * @param id     Client connection ID
 * @param rclst  Place to store pointer to client listener
 *
 * @return EOK on success, ENOENT if no client listener with the given ID
 *         is found.
 */
static errno_t tcp_clistener_get(tcp_client_t *client, sysarg_t id,
    tcp_clst_t **rclst)
{
	list_foreach (client->clst, lclient, tcp_clst_t, clst) {
		if (clst->id == id) {
			*rclst = clst;
			return EOK;
		}
	}

	return ENOENT;
}

/** Create connection.
 *
 * Handle client request to create connection (with parameters unmarshalled).
 *
 * @param client   TCP client
 * @param epp      Endpoint pair
 * @param rconn_id Place to store ID of new connection
 *
 * @return EOK on success or an error code
 */
static errno_t tcp_conn_create_impl(tcp_client_t *client, inet_ep2_t *epp,
    sysarg_t *rconn_id)
{
	tcp_conn_t *conn;
	tcp_cconn_t *cconn;
	errno_t rc;
	tcp_error_t trc;
	char *slocal;
	char *sremote;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_create_impl");

	inet_addr_format(&epp->local.addr, &slocal);
	inet_addr_format(&epp->remote.addr, &sremote);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_create: local=%s remote=%s",
	    slocal, sremote);
	free(slocal);
	free(sremote);

	trc = tcp_uc_open(epp, ap_active, tcp_open_nonblock, &conn);
	if (trc != TCP_EOK)
		return EIO;

	conn->name = (char *) "c";

	rc = tcp_cconn_create(client, conn, &cconn);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		tcp_conn_delete(conn);
		return ENOMEM;
	}

	/* XXX Is there a race here (i.e. the connection is already active)? */
	tcp_uc_set_cb(conn, &tcp_service_cb, cconn);

	*rconn_id = cconn->id;
	return EOK;
}

/** Destroy connection.
 *
 * Handle client request to destroy connection (with parameters unmarshalled).
 *
 * @param client  TCP client
 * @param conn_id Connection ID
 * @return EOK on success, ENOENT if no such connection is found
 */
static errno_t tcp_conn_destroy_impl(tcp_client_t *client, sysarg_t conn_id)
{
	tcp_cconn_t *cconn;
	errno_t rc;

	rc = tcp_cconn_get(client, conn_id, &cconn);
	if (rc != EOK) {
		assert(rc == ENOENT);
		return ENOENT;
	}

	tcp_uc_close(cconn->conn);
	tcp_uc_delete(cconn->conn);
	tcp_cconn_destroy(cconn);
	return EOK;
}

/** Create listener.
 *
 * Handle client request to create listener (with parameters unmarshalled).
 *
 * @param client  TCP client
 * @param ep      Endpoint
 * @param rlst_id Place to store ID of new listener
 *
 * @return EOK on success or an error code
 */
static errno_t tcp_listener_create_impl(tcp_client_t *client, inet_ep_t *ep,
    sysarg_t *rlst_id)
{
	tcp_conn_t *conn;
	tcp_clst_t *clst;
	inet_ep2_t epp;
	errno_t rc;
	tcp_error_t trc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_listener_create_impl");

	inet_ep2_init(&epp);
	epp.local.addr = ep->addr;
	epp.local.port = ep->port;

	trc = tcp_uc_open(&epp, ap_passive, tcp_open_nonblock, &conn);
	if (trc != TCP_EOK)
		return EIO;

	conn->name = (char *) "s";

	rc = tcp_clistener_create(client, conn, &clst);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		tcp_conn_delete(conn);
		return ENOMEM;
	}

	clst->elocal = epp.local;

	/* XXX Is there a race here (i.e. the connection is already active)? */
	tcp_uc_set_cb(conn, &tcp_service_lst_cb, clst);

	*rlst_id = clst->id;
	return EOK;
}

/** Destroy listener.
 *
 * Handle client request to destroy listener (with parameters unmarshalled).
 *
 * @param client TCP client
 * @param lst_id Listener ID
 *
 * @return EOK on success, ENOENT if no such listener is found
 */
static errno_t tcp_listener_destroy_impl(tcp_client_t *client, sysarg_t lst_id)
{
	tcp_clst_t *clst;
	errno_t rc;

	rc = tcp_clistener_get(client, lst_id, &clst);
	if (rc != EOK) {
		assert(rc == ENOENT);
		return ENOENT;
	}

#if 0
	tcp_uc_close(cconn->conn);
#endif
	tcp_clistener_destroy(clst);
	return EOK;
}

/** Send FIN.
 *
 * Handle client request to send FIN (with parameters unmarshalled).
 *
 * @param client  TCP client
 * @param conn_id Connection ID
 *
 * @return EOK on success or an error code
 */
static errno_t tcp_conn_send_fin_impl(tcp_client_t *client, sysarg_t conn_id)
{
	tcp_cconn_t *cconn;
	errno_t rc;

	rc = tcp_cconn_get(client, conn_id, &cconn);
	if (rc != EOK) {
		assert(rc == ENOENT);
		return ENOENT;
	}

	(void) cconn;
	/* XXX TODO */
	return EOK;
}

/** Push connection.
 *
 * Handle client request to push connection (with parameters unmarshalled).
 *
 * @param client  TCP client
 * @param conn_id Connection ID
 *
 * @return EOK on success or an error code
 */
static errno_t tcp_conn_push_impl(tcp_client_t *client, sysarg_t conn_id)
{
	tcp_cconn_t *cconn;
	errno_t rc;

	rc = tcp_cconn_get(client, conn_id, &cconn);
	if (rc != EOK) {
		assert(rc == ENOENT);
		return ENOENT;
	}

	(void) cconn;
	/* XXX TODO */
	return EOK;
}

/** Reset connection.
 *
 * Handle client request to reset connection (with parameters unmarshalled).
 *
 * @param client  TCP client
 * @param conn_id Connection ID
 *
 * @return EOK on success or an error code
 */
static errno_t tcp_conn_reset_impl(tcp_client_t *client, sysarg_t conn_id)
{
	tcp_cconn_t *cconn;
	errno_t rc;

	rc = tcp_cconn_get(client, conn_id, &cconn);
	if (rc != EOK) {
		assert(rc == ENOENT);
		return ENOENT;
	}

	tcp_uc_abort(cconn->conn);
	return EOK;
}

/** Send data over connection..
 *
 * Handle client request to send data (with parameters unmarshalled).
 *
 * @param client  TCP client
 * @param conn_id Connection ID
 * @param data    Data buffer
 * @param size    Data size in bytes
 *
 * @return EOK on success or an error code
 */
static errno_t tcp_conn_send_impl(tcp_client_t *client, sysarg_t conn_id,
    void *data, size_t size)
{
	tcp_cconn_t *cconn;
	errno_t rc;
	tcp_error_t trc;

	rc = tcp_cconn_get(client, conn_id, &cconn);
	if (rc != EOK)
		return rc;

	trc = tcp_uc_send(cconn->conn, data, size, 0);
	if (trc != TCP_EOK)
		return EIO;

	return EOK;
}

/** Receive data from connection.
 *
 * Handle client request to receive data (with parameters unmarshalled).
 *
 * @param client  TCP client
 * @param conn_id Connection ID
 * @param data    Data buffer
 * @param size    Buffer size in bytes
 * @param nrecv   Place to store actual number of bytes received
 *
 * @return EOK on success or an error code
 */
static errno_t tcp_conn_recv_impl(tcp_client_t *client, sysarg_t conn_id,
    void *data, size_t size, size_t *nrecv)
{
	tcp_cconn_t *cconn;
	xflags_t xflags;
	errno_t rc;
	tcp_error_t trc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_impl()");

	rc = tcp_cconn_get(client, conn_id, &cconn);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_impl() - conn not found");
		return rc;
	}

	trc = tcp_uc_receive(cconn->conn, data, size, nrecv, &xflags);
	if (trc != TCP_EOK) {
		switch (trc) {
		case TCP_EAGAIN:
			log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_impl() - EAGAIN");
			return EAGAIN;
		case TCP_ECLOSING:
			*nrecv = 0;
			return EOK;
		default:
			log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_impl() - trc=%d", trc);
			return EIO;
		}
	}

	return EOK;
}

/** Create client callback session.
 *
 * Handle client request to create callback session.
 *
 * @param client TCP client
 * @param icall  Async request data
 *
 */
static void tcp_callback_create_srv(tcp_client_t *client, ipc_call_t *icall)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_callback_create_srv()");

	async_sess_t *sess = async_callback_receive(EXCHANGE_SERIALIZE);
	if (sess == NULL) {
		async_answer_0(icall, ENOMEM);
		return;
	}

	client->sess = sess;
	async_answer_0(icall, EOK);
}

/** Create connection.
 *
 * Handle client request to create connection.
 *
 * @param client TCP client
 * @param icall  Async request data
 *
 */
static void tcp_conn_create_srv(tcp_client_t *client, ipc_call_t *icall)
{
	ipc_call_t call;
	size_t size;
	inet_ep2_t epp;
	sysarg_t conn_id;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_create_srv()");

	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(inet_ep2_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_write_finalize(&call, &epp, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	rc = tcp_conn_create_impl(client, &epp, &conn_id);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	async_answer_1(icall, EOK, conn_id);
}

/** Destroy connection.
 *
 * Handle client request to destroy connection.
 *
 * @param client TCP client
 * @param icall  Async request data
 *
 */
static void tcp_conn_destroy_srv(tcp_client_t *client, ipc_call_t *icall)
{
	sysarg_t conn_id;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_destroy_srv()");

	conn_id = ipc_get_arg1(icall);
	rc = tcp_conn_destroy_impl(client, conn_id);
	async_answer_0(icall, rc);
}

/** Create listener.
 *
 * Handle client request to create listener.
 *
 * @param client TCP client
 * @param icall  Async request data
 *
 */
static void tcp_listener_create_srv(tcp_client_t *client, ipc_call_t *icall)
{
	ipc_call_t call;
	size_t size;
	inet_ep_t ep;
	sysarg_t lst_id;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_listener_create_srv()");

	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(inet_ep_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_write_finalize(&call, &ep, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	rc = tcp_listener_create_impl(client, &ep, &lst_id);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	async_answer_1(icall, EOK, lst_id);
}

/** Destroy listener.
 *
 * Handle client request to destroy listener.
 *
 * @param client TCP client
 * @param icall  Async request data
 *
 */
static void tcp_listener_destroy_srv(tcp_client_t *client, ipc_call_t *icall)
{
	sysarg_t lst_id;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_listener_destroy_srv()");

	lst_id = ipc_get_arg1(icall);
	rc = tcp_listener_destroy_impl(client, lst_id);
	async_answer_0(icall, rc);
}

/** Send FIN.
 *
 * Handle client request to send FIN.
 *
 * @param client TCP client
 * @param icall  Async request data
 *
 */
static void tcp_conn_send_fin_srv(tcp_client_t *client, ipc_call_t *icall)
{
	sysarg_t conn_id;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_send_fin_srv()");

	conn_id = ipc_get_arg1(icall);
	rc = tcp_conn_send_fin_impl(client, conn_id);
	async_answer_0(icall, rc);
}

/** Push connection.
 *
 * Handle client request to push connection.
 *
 * @param client TCP client
 * @param icall  Async request data
 *
 */
static void tcp_conn_push_srv(tcp_client_t *client, ipc_call_t *icall)
{
	sysarg_t conn_id;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_push_srv()");

	conn_id = ipc_get_arg1(icall);
	rc = tcp_conn_push_impl(client, conn_id);
	async_answer_0(icall, rc);
}

/** Reset connection.
 *
 * Handle client request to reset connection.
 *
 * @param client TCP client
 * @param icall  Async request data
 *
 */
static void tcp_conn_reset_srv(tcp_client_t *client, ipc_call_t *icall)
{
	sysarg_t conn_id;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_reset_srv()");

	conn_id = ipc_get_arg1(icall);
	rc = tcp_conn_reset_impl(client, conn_id);
	async_answer_0(icall, rc);
}

/** Send data via connection..
 *
 * Handle client request to send data via connection.
 *
 * @param client TCP client
 * @param icall  Async request data
 *
 */
static void tcp_conn_send_srv(tcp_client_t *client, ipc_call_t *icall)
{
	ipc_call_t call;
	size_t size;
	sysarg_t conn_id;
	void *data;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_send_srv())");

	/* Receive message data */

	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size > MAX_MSG_SIZE) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	data = malloc(size);
	if (data == NULL) {
		async_answer_0(&call, ENOMEM);
		async_answer_0(icall, ENOMEM);
		return;
	}

	rc = async_data_write_finalize(&call, data, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		free(data);
		return;
	}

	conn_id = ipc_get_arg1(icall);

	rc = tcp_conn_send_impl(client, conn_id, data, size);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		free(data);
		return;
	}

	async_answer_0(icall, EOK);
	free(data);
}

/** Read received data from connection without blocking.
 *
 * Handle client request to read received data via connection without blocking.
 *
 * @param client TCP client
 * @param icall  Async request data
 *
 */
static void tcp_conn_recv_srv(tcp_client_t *client, ipc_call_t *icall)
{
	ipc_call_t call;
	sysarg_t conn_id;
	size_t size, rsize;
	void *data;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_srv()");

	conn_id = ipc_get_arg1(icall);

	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	size = min(size, 16384);
	data = malloc(size);
	if (data == NULL) {
		async_answer_0(&call, ENOMEM);
		async_answer_0(icall, ENOMEM);
		return;
	}

	rc = tcp_conn_recv_impl(client, conn_id, data, size, &rsize);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		free(data);
		return;
	}

	rc = async_data_read_finalize(&call, data, size);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		free(data);
		return;
	}

	async_answer_1(icall, EOK, rsize);
	free(data);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_srv(): OK");
}

/** Read received data from connection with blocking.
 *
 * Handle client request to read received data via connection with blocking.
 *
 * @param client TCP client
 * @param icall  Async request data
 *
 */
static void tcp_conn_recv_wait_srv(tcp_client_t *client, ipc_call_t *icall)
{
	ipc_call_t call;
	sysarg_t conn_id;
	size_t size, rsize;
	void *data;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_wait_srv()");

	conn_id = ipc_get_arg1(icall);

	if (!async_data_read_receive(&call, &size)) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_wait_srv - data_receive failed");
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	size = min(size, 16384);
	data = malloc(size);
	if (data == NULL) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_wait_srv - allocation failed");
		async_answer_0(&call, ENOMEM);
		async_answer_0(icall, ENOMEM);
		return;
	}

	rc = tcp_conn_recv_impl(client, conn_id, data, size, &rsize);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_wait_srv - recv_impl failed rc=%s", str_error_name(rc));
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		free(data);
		return;
	}

	rc = async_data_read_finalize(&call, data, size);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_wait_srv - finalize failed");
		async_answer_0(icall, rc);
		free(data);
		return;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_wait_srv(): rsize=%zu", size);
	async_answer_1(icall, EOK, rsize);
	free(data);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_wait_srv(): OK");
}

/** Initialize TCP client structure.
 *
 * @param client TCP client
 */
static void tcp_client_init(tcp_client_t *client)
{
	memset(client, 0, sizeof(tcp_client_t));
	client->sess = NULL;
	list_initialize(&client->cconn);
	list_initialize(&client->clst);
}

/** Finalize TCP client structure.
 *
 * @param client TCP client
 */
static void tcp_client_fini(tcp_client_t *client)
{
	tcp_cconn_t *cconn;
	unsigned long n;

	n = list_count(&client->cconn);
	if (n != 0) {
		log_msg(LOG_DEFAULT, LVL_WARN, "Client with %lu active "
		    "connections closed session", n);

		while (!list_empty(&client->cconn)) {
			cconn = list_get_instance(list_first(&client->cconn),
			    tcp_cconn_t, lclient);
			tcp_uc_close(cconn->conn);
			tcp_uc_delete(cconn->conn);
			tcp_cconn_destroy(cconn);
		}
	}

	n = list_count(&client->clst);
	if (n != 0) {
		log_msg(LOG_DEFAULT, LVL_WARN, "Client with %lu active "
		    "listeners closed session", n);
		/* XXX Destroy listeners */
	}

	if (client->sess != NULL)
		async_hangup(client->sess);
}

/** Handle TCP client connection.
 *
 * @param icall Connect call data
 * @param arg   Connection argument
 *
 */
static void tcp_client_conn(ipc_call_t *icall, void *arg)
{
	tcp_client_t client;

	/* Accept the connection */
	async_accept_0(icall);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_client_conn() - client=%p",
	    &client);

	tcp_client_init(&client);

	while (true) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_client_conn: wait req");
		ipc_call_t call;
		async_get_call(&call);
		sysarg_t method = ipc_get_imethod(&call);

		log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_client_conn: method=%d",
		    (int)method);
		if (!method) {
			/* The other side has hung up */
			async_answer_0(&call, EOK);
			break;
		}

		switch (method) {
		case TCP_CALLBACK_CREATE:
			tcp_callback_create_srv(&client, &call);
			break;
		case TCP_CONN_CREATE:
			tcp_conn_create_srv(&client, &call);
			break;
		case TCP_CONN_DESTROY:
			tcp_conn_destroy_srv(&client, &call);
			break;
		case TCP_LISTENER_CREATE:
			tcp_listener_create_srv(&client, &call);
			break;
		case TCP_LISTENER_DESTROY:
			tcp_listener_destroy_srv(&client, &call);
			break;
		case TCP_CONN_SEND_FIN:
			tcp_conn_send_fin_srv(&client, &call);
			break;
		case TCP_CONN_PUSH:
			tcp_conn_push_srv(&client, &call);
			break;
		case TCP_CONN_RESET:
			tcp_conn_reset_srv(&client, &call);
			break;
		case TCP_CONN_SEND:
			tcp_conn_send_srv(&client, &call);
			break;
		case TCP_CONN_RECV:
			tcp_conn_recv_srv(&client, &call);
			break;
		case TCP_CONN_RECV_WAIT:
			tcp_conn_recv_wait_srv(&client, &call);
			break;
		default:
			async_answer_0(&call, ENOTSUP);
			break;
		}
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_client_conn TERMINATED");
	tcp_client_fini(&client);
}

/** Initialize TCP service.
 *
 * @return EOK on success or an error code.
 */
errno_t tcp_service_init(void)
{
	errno_t rc;
	service_id_t sid;
	loc_srv_t *srv;

	async_set_fallback_port_handler(tcp_client_conn, NULL);

	rc = loc_server_register(NAME, &srv);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering server.");
		return EIO;
	}

	rc = loc_service_register(srv, SERVICE_NAME_TCP, &sid);
	if (rc != EOK) {
		loc_server_unregister(srv);
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering service.");
		return EIO;
	}

	return EOK;
}

/**
 * @}
 */
