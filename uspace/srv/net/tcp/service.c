/*
 * Copyright (c) 2015 Jiri Svoboda
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

/** @addtogroup udp
 * @{
 */

/**
 * @file HelenOS service implementation
 */

#include <async.h>
#include <errno.h>
#include <inet/endpoint.h>
#include <inet/inet.h>
#include <io/log.h>
#include <ipc/services.h>
#include <ipc/tcp.h>
#include <loc.h>
#include <macros.h>
#include <stdlib.h>

#include "conn.h"
#include "service.h"
#include "tcp_type.h"
#include "ucall.h"

#define NAME "tcp"

#define MAX_MSG_SIZE DATA_XFER_LIMIT

static void tcp_ev_data(tcp_cconn_t *);
static void tcp_ev_connected(tcp_cconn_t *);
static void tcp_ev_conn_failed(tcp_cconn_t *);
static void tcp_ev_conn_reset(tcp_cconn_t *);

static void tcp_service_cstate_change(tcp_conn_t *, void *, tcp_cstate_t);
static void tcp_service_recv_data(tcp_conn_t *, void *);

static tcp_cb_t tcp_service_cb = {
	.cstate_change = tcp_service_cstate_change,
	.recv_data = tcp_service_recv_data
};

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
		/* Connection reset */
		tcp_ev_conn_reset(cconn);
	}

	/* XXX Failed to establish connection */
	if (0) tcp_ev_conn_failed(cconn);
}

static void tcp_service_recv_data(tcp_conn_t *conn, void *arg)
{
	tcp_cconn_t *cconn = (tcp_cconn_t *)arg;

	tcp_ev_data(cconn);
}

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

static void tcp_ev_connected(tcp_cconn_t *cconn)
{
	async_exch_t *exch;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_ev_connected()");

	exch = async_exchange_begin(cconn->client->sess);
	aid_t req = async_send_1(exch, TCP_EV_CONNECTED, cconn->id, NULL);
	async_exchange_end(exch);

	async_forget(req);
}

static void tcp_ev_conn_failed(tcp_cconn_t *cconn)
{
	async_exch_t *exch;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_ev_conn_failed()");

	exch = async_exchange_begin(cconn->client->sess);
	aid_t req = async_send_1(exch, TCP_EV_CONN_FAILED, cconn->id, NULL);
	async_exchange_end(exch);

	async_forget(req);
}

static void tcp_ev_conn_reset(tcp_cconn_t *cconn)
{
	async_exch_t *exch;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_ev_conn_reset()");

	exch = async_exchange_begin(cconn->client->sess);
	aid_t req = async_send_1(exch, TCP_EV_CONN_RESET, cconn->id, NULL);
	async_exchange_end(exch);

	async_forget(req);
}

static int tcp_cconn_create(tcp_client_t *client, tcp_conn_t *conn,
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

static void tcp_cconn_destroy(tcp_cconn_t *cconn)
{
	list_remove(&cconn->lclient);
	free(cconn);
}

static int tcp_clistener_create(tcp_client_t *client, tcp_conn_t *conn,
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

static void tcp_clistener_destroy(tcp_clst_t *clst)
{
	list_remove(&clst->lclient);
	free(clst);
}

static int tcp_cconn_get(tcp_client_t *client, sysarg_t id,
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

static int tcp_clistener_get(tcp_client_t *client, sysarg_t id,
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


static int tcp_conn_create_impl(tcp_client_t *client, inet_ep2_t *epp,
    sysarg_t *rconn_id)
{
	tcp_conn_t *conn;
	tcp_cconn_t *cconn;
	tcp_sock_t local;
	tcp_sock_t remote;
	inet_addr_t local_addr;
	int rc;
	tcp_error_t trc;
	char *slocal;
	char *sremote;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_create_impl");

	/* Fill in local address? */
	if (inet_addr_is_any(&epp->local.addr)) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_create_impl: "
		    "determine local address");
		rc = inet_get_srcaddr(&epp->remote.addr, 0, &local_addr);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_create_impl: "
			    "cannot determine local address");
			return rc;
		}
	} else {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_create_impl: "
		    "local address specified");
		local_addr = epp->local.addr;
	}

	/* Allocate local port? */
	if (epp->local.port == 0) {
		epp->local.port = 49152; /* XXX */
	}

	local.addr = local_addr;
	local.port = epp->local.port;
	remote.addr = epp->remote.addr;
	remote.port = epp->remote.port;

	inet_addr_format(&local_addr, &slocal);
	inet_addr_format(&remote.addr, &sremote);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_create: local=%s remote=%s",
	    slocal, sremote);

	trc = tcp_uc_open(&local, &remote, ap_active, tcp_open_nonblock, &conn);
	if (trc != TCP_EOK)
		return EIO;

	rc = tcp_cconn_create(client, conn, &cconn);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		tcp_conn_delete(conn);
		return ENOMEM;
	}

	/* XXX Is there a race here (i.e. the connection is already active)? */
	tcp_uc_set_cb(conn, &tcp_service_cb, cconn);

//	assoc->cb = &udp_cassoc_cb;
//	assoc->cb_arg = cassoc;

	*rconn_id = cconn->id;
	return EOK;
}

static int tcp_conn_destroy_impl(tcp_client_t *client, sysarg_t conn_id)
{
	tcp_cconn_t *cconn;
	int rc;

	rc = tcp_cconn_get(client, conn_id, &cconn);
	if (rc != EOK) {
		assert(rc == ENOENT);
		return ENOENT;
	}

	tcp_uc_close(cconn->conn);
	tcp_cconn_destroy(cconn);
	return EOK;
}

static int tcp_listener_create_impl(tcp_client_t *client, inet_ep_t *ep,
    sysarg_t *rlst_id)
{
	tcp_conn_t *conn;
	tcp_clst_t *clst;
	tcp_sock_t local;
	int rc;
	tcp_error_t trc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_listener_create_impl");

	local.addr = ep->addr;
	local.port = ep->port;

	trc = tcp_uc_open(&local, NULL, ap_passive, 0, &conn);
	if (trc != TCP_EOK)
		return EIO;

	rc = tcp_clistener_create(client, conn, &clst);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		tcp_conn_delete(conn);
		return ENOMEM;
	}

//	assoc->cb = &udp_cassoc_cb;
//	assoc->cb_arg = cassoc;

	*rlst_id = clst->id;
	return EOK;
}

static int tcp_listener_destroy_impl(tcp_client_t *client, sysarg_t lst_id)
{
	tcp_clst_t *clst;
	int rc;

	rc = tcp_clistener_get(client, lst_id, &clst);
	if (rc != EOK) {
		assert(rc == ENOENT);
		return ENOENT;
	}

//	tcp_uc_close(cconn->conn);
	tcp_clistener_destroy(clst);
	return EOK;
}

static int tcp_conn_send_fin_impl(tcp_client_t *client, sysarg_t conn_id)
{
	tcp_cconn_t *cconn;
	int rc;

	rc = tcp_cconn_get(client, conn_id, &cconn);
	if (rc != EOK) {
		assert(rc == ENOENT);
		return ENOENT;
	}

	(void) cconn;
	/* XXX TODO */
	return EOK;
}

static int tcp_conn_push_impl(tcp_client_t *client, sysarg_t conn_id)
{
	tcp_cconn_t *cconn;
	int rc;

	rc = tcp_cconn_get(client, conn_id, &cconn);
	if (rc != EOK) {
		assert(rc == ENOENT);
		return ENOENT;
	}

	(void) cconn;
	/* XXX TODO */
	return EOK;
}

static int tcp_conn_reset_impl(tcp_client_t *client, sysarg_t conn_id)
{
	tcp_cconn_t *cconn;
	int rc;

	rc = tcp_cconn_get(client, conn_id, &cconn);
	if (rc != EOK) {
		assert(rc == ENOENT);
		return ENOENT;
	}

	tcp_uc_abort(cconn->conn);
	return EOK;
}

static int tcp_conn_send_impl(tcp_client_t *client, sysarg_t conn_id,
    void *data, size_t size)
{
	tcp_cconn_t *cconn;
	int rc;

	rc = tcp_cconn_get(client, conn_id, &cconn);
	if (rc != EOK)
		return rc;

	rc = tcp_uc_send(cconn->conn, data, size, 0);
	if (rc != EOK)
		return rc;

	return EOK;
}

static int tcp_conn_recv_impl(tcp_client_t *client, sysarg_t conn_id,
    void *data, size_t size, size_t *nrecv)
{
	tcp_cconn_t *cconn;
	xflags_t xflags;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_impl()");

	rc = tcp_cconn_get(client, conn_id, &cconn);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_impl() - conn not found");
		return rc;
	}

	rc = tcp_uc_receive(cconn->conn, data, size, nrecv, &xflags);
	if (rc != EOK) {
		switch (rc) {
		case TCP_EAGAIN:
			log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_impl() - EAGAIN");
			return EAGAIN;
		default:
			log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_impl() - trc=%d", rc);
			return EIO;
		}
	}

	return EOK;
}

static void tcp_callback_create_srv(tcp_client_t *client, ipc_callid_t iid,
    ipc_call_t *icall)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_callback_create_srv()");

	async_sess_t *sess = async_callback_receive(EXCHANGE_SERIALIZE);
	if (sess == NULL) {
		async_answer_0(iid, ENOMEM);
		return;
	}

	client->sess = sess;
	async_answer_0(iid, EOK);
}

static void tcp_conn_create_srv(tcp_client_t *client, ipc_callid_t iid,
    ipc_call_t *icall)
{
	ipc_callid_t callid;
	size_t size;
	inet_ep2_t epp;
	sysarg_t conn_id;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_create_srv()");

	if (!async_data_write_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}

	if (size != sizeof(inet_ep2_t)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}

	rc = async_data_write_finalize(callid, &epp, size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		return;
	}

	rc = tcp_conn_create_impl(client, &epp, &conn_id);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}

	async_answer_1(iid, EOK, conn_id);
}

static void tcp_conn_destroy_srv(tcp_client_t *client, ipc_callid_t iid,
    ipc_call_t *icall)
{
	sysarg_t conn_id;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_destroy_srv()");

	conn_id = IPC_GET_ARG1(*icall);
	rc = tcp_conn_destroy_impl(client, conn_id);
	async_answer_0(iid, rc);
}

static void tcp_listener_create_srv(tcp_client_t *client, ipc_callid_t iid,
    ipc_call_t *icall)
{
	ipc_callid_t callid;
	size_t size;
	inet_ep_t ep;
	sysarg_t lst_id;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_listener_create_srv()");

	if (!async_data_write_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}

	if (size != sizeof(inet_ep_t)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}

	rc = async_data_write_finalize(callid, &ep, size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		return;
	}

	rc = tcp_listener_create_impl(client, &ep, &lst_id);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}

	async_answer_1(iid, EOK, lst_id);
}

static void tcp_listener_destroy_srv(tcp_client_t *client, ipc_callid_t iid,
    ipc_call_t *icall)
{
	sysarg_t lst_id;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_listener_destroy_srv()");

	lst_id = IPC_GET_ARG1(*icall);
	rc = tcp_listener_destroy_impl(client, lst_id);
	async_answer_0(iid, rc);
}

static void tcp_conn_send_fin_srv(tcp_client_t *client, ipc_callid_t iid,
    ipc_call_t *icall)
{
	sysarg_t conn_id;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_send_fin_srv()");

	conn_id = IPC_GET_ARG1(*icall);
	rc = tcp_conn_send_fin_impl(client, conn_id);
	async_answer_0(iid, rc);
}

static void tcp_conn_push_srv(tcp_client_t *client, ipc_callid_t iid,
    ipc_call_t *icall)
{
	sysarg_t conn_id;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_push_srv()");

	conn_id = IPC_GET_ARG1(*icall);
	rc = tcp_conn_push_impl(client, conn_id);
	async_answer_0(iid, rc);
}

static void tcp_conn_reset_srv(tcp_client_t *client, ipc_callid_t iid,
    ipc_call_t *icall)
{
	sysarg_t conn_id;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_reset_srv()");

	conn_id = IPC_GET_ARG1(*icall);
	rc = tcp_conn_reset_impl(client, conn_id);
	async_answer_0(iid, rc);
}

static void tcp_conn_send_srv(tcp_client_t *client, ipc_callid_t iid,
    ipc_call_t *icall)
{
	ipc_callid_t callid;
	size_t size;
	sysarg_t conn_id;
	void *data;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_send_srv())");

	/* Receive message data */

	if (!async_data_write_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}

	if (size > MAX_MSG_SIZE) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}

	data = malloc(size);
	if (data == NULL) {
		async_answer_0(callid, ENOMEM);
		async_answer_0(iid, ENOMEM);
	}

	rc = async_data_write_finalize(callid, data, size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		free(data);
		return;
	}

	conn_id = IPC_GET_ARG1(*icall);

	rc = tcp_conn_send_impl(client, conn_id, data, size);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		free(data);
		return;
	}

	async_answer_0(iid, EOK);
	free(data);
}

static void tcp_conn_recv_srv(tcp_client_t *client, ipc_callid_t iid,
    ipc_call_t *icall)
{
	ipc_callid_t callid;
	sysarg_t conn_id;
	size_t size, rsize;
	void *data;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_srv()");

	conn_id = IPC_GET_ARG1(*icall);

	if (!async_data_read_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}

	size = max(size, 16384);
	data = malloc(size);
	if (data == NULL) {
		async_answer_0(callid, ENOMEM);
		async_answer_0(iid, ENOMEM);
		return;
	}

	rc = tcp_conn_recv_impl(client, conn_id, data, size, &rsize);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		free(data);
		return;
	}

	rc = async_data_read_finalize(callid, data, size);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		free(data);
		return;
	}

	async_answer_1(iid, EOK, rsize);
	free(data);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_srv(): OK");
}

static void tcp_conn_recv_wait_srv(tcp_client_t *client, ipc_callid_t iid,
    ipc_call_t *icall)
{
	ipc_callid_t callid;
	sysarg_t conn_id;
	size_t size, rsize;
	void *data;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_wait_srv()");

	conn_id = IPC_GET_ARG1(*icall);

	if (!async_data_read_receive(&callid, &size)) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_wait_srv - data_receive failed");
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}

	size = min(size, 16384);
	data = malloc(size);
	if (data == NULL) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_wait_srv - allocation failed");
		async_answer_0(callid, ENOMEM);
		async_answer_0(iid, ENOMEM);
		return;
	}

	rc = tcp_conn_recv_impl(client, conn_id, data, size, &rsize);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_wait_srv - recv_impl failed rc=%d", rc);
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		free(data);
		return;
	}

	rc = async_data_read_finalize(callid, data, size);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_wait_srv - finalize failed");
		async_answer_0(iid, rc);
		free(data);
		return;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_wait_srv(): rsize=%zu", size);
	async_answer_1(iid, EOK, rsize);
	free(data);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_recv_wait_srv(): OK");
}

#include <mem.h>

static void tcp_client_init(tcp_client_t *client)
{
	memset(client, 0, sizeof(tcp_client_t));
	client->sess = NULL;
	list_initialize(&client->cconn);
	list_initialize(&client->clst);
}

static void tcp_client_fini(tcp_client_t *client)
{
	tcp_cconn_t *cconn;
	size_t n;

	n = list_count(&client->cconn);
	if (n != 0) {
		log_msg(LOG_DEFAULT, LVL_WARN, "Client with %zu active "
		    "connections closed session", n);

		while (!list_empty(&client->cconn)) {
			cconn = list_get_instance(list_first(&client->cconn),
			    tcp_cconn_t, lclient);
			tcp_uc_close(cconn->conn);
			tcp_cconn_destroy(cconn);
		}
	}

	n = list_count(&client->clst);
	if (n != 0) {
		log_msg(LOG_DEFAULT, LVL_WARN, "Client with %zu active "
		    "listeners closed session", n);
		/* XXX Destroy listeners */
	}
}

static void tcp_client_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	tcp_client_t client;

	/* Accept the connection */
	async_answer_0(iid, EOK);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_client_conn() - client=%p",
	    &client);

	tcp_client_init(&client);

	while (true) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_client_conn: wait req");
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		sysarg_t method = IPC_GET_IMETHOD(call);

		log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_client_conn: method=%d",
		    (int)method);
		if (!method) {
			/* The other side has hung up */
			async_answer_0(callid, EOK);
			break;
		}

		switch (method) {
		case TCP_CALLBACK_CREATE:
			tcp_callback_create_srv(&client, callid, &call);
			break;
		case TCP_CONN_CREATE:
			tcp_conn_create_srv(&client, callid, &call);
			break;
		case TCP_CONN_DESTROY:
			tcp_conn_destroy_srv(&client, callid, &call);
			break;
		case TCP_LISTENER_CREATE:
			tcp_listener_create_srv(&client, callid, &call);
			break;
		case TCP_LISTENER_DESTROY:
			tcp_listener_destroy_srv(&client, callid, &call);
			break;
		case TCP_CONN_SEND_FIN:
			tcp_conn_send_fin_srv(&client, callid, &call);
			break;
		case TCP_CONN_PUSH:
			tcp_conn_push_srv(&client, callid, &call);
			break;
		case TCP_CONN_RESET:
			tcp_conn_reset_srv(&client, callid, &call);
			break;
		case TCP_CONN_SEND:
			tcp_conn_send_srv(&client, callid, &call);
			break;
		case TCP_CONN_RECV:
			tcp_conn_recv_srv(&client, callid, &call);
			break;
		case TCP_CONN_RECV_WAIT:
			tcp_conn_recv_wait_srv(&client, callid, &call);
			break;
		default:
			async_answer_0(callid, ENOTSUP);
			break;
		}
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_client_conn TERMINATED");
	tcp_client_fini(&client);
}

int tcp_service_init(void)
{
	int rc;
	service_id_t sid;

	async_set_client_connection(tcp_client_conn);

	rc = loc_server_register(NAME);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering server.");
		return EIO;
	}

	rc = loc_service_register(SERVICE_NAME_TCP, &sid);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering service.");
		return EIO;
	}

	return EOK;
}

/**
 * @}
 */
