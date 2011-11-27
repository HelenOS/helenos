/*
 * Copyright (c) 2008 Lukas Mejdrech
 * Copyright (c) 2011 Jiri Svoboda
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
 * @file Socket provider
 */

#include <async.h>
#include <errno.h>
#include <io/log.h>
#include <ipc/socket.h>
#include <net/modules.h>

#include "sock.h"
#include "std.h"
#include "tcp.h"
#include "tcp_type.h"
#include "ucall.h"

#define FRAGMENT_SIZE 1024

/** Free ports pool start. */
#define TCP_FREE_PORTS_START		1025

/** Free ports pool end. */
#define TCP_FREE_PORTS_END		65535

static int last_used_port;
static socket_ports_t gsock;

static void tcp_free_sock_data(socket_core_t *sock_core)
{
	tcp_sockdata_t *socket;

	socket = (tcp_sockdata_t *)sock_core->specific_data;
	(void)socket;
}

static void tcp_sock_notify_data(socket_core_t *sock_core)
{
	async_exch_t *exch = async_exchange_begin(sock_core->sess);
	async_msg_5(exch, NET_SOCKET_RECEIVED, (sysarg_t)sock_core->socket_id,
	    FRAGMENT_SIZE, 0, 0, 1);
	async_exchange_end(exch);
}

static void tcp_sock_socket(tcp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	tcp_sockdata_t *sock;
	int sock_id;
	int rc;
	ipc_call_t answer;

	log_msg(LVL_DEBUG, "tcp_sock_socket()");
	sock = calloc(sizeof(tcp_sockdata_t), 1);
	if (sock == NULL) {
		async_answer_0(callid, ENOMEM);
		return;
	}

	sock->client = client;

	sock_id = SOCKET_GET_SOCKET_ID(call);
	rc = socket_create(&client->sockets, client->sess, sock, &sock_id);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}

	refresh_answer(&answer, NULL);
	SOCKET_SET_SOCKET_ID(answer, sock_id);

	SOCKET_SET_DATA_FRAGMENT_SIZE(answer, FRAGMENT_SIZE);
	SOCKET_SET_HEADER_SIZE(answer, sizeof(tcp_header_t));
	answer_call(callid, EOK, &answer, 3);
}

static void tcp_sock_bind(tcp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	int rc;
	struct sockaddr *addr;
	size_t addr_len;
	socket_core_t *sock_core;
	tcp_sockdata_t *socket;

	log_msg(LVL_DEBUG, "tcp_sock_bind()");
	rc = async_data_write_accept((void **) &addr, false, 0, 0, 0, &addr_len);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}

	rc = socket_bind(&client->sockets, &gsock, SOCKET_GET_SOCKET_ID(call),
	    addr, addr_len, TCP_FREE_PORTS_START, TCP_FREE_PORTS_END,
	    last_used_port);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}

	sock_core = socket_cores_find(&client->sockets, SOCKET_GET_SOCKET_ID(call));
	if (sock_core != NULL) {
		socket = (tcp_sockdata_t *)sock_core->specific_data;
		/* XXX Anything to do? */
		(void) socket;
	}

	async_answer_0(callid, ENOTSUP);

	/* Push one fragment notification to client's queue */
	tcp_sock_notify_data(sock_core);
}

static void tcp_sock_listen(tcp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	int socket_id;
	int backlog;
	socket_core_t *sock_core;
	tcp_sockdata_t *socket;

	log_msg(LVL_DEBUG, "tcp_sock_listen()");

	socket_id = SOCKET_GET_SOCKET_ID(call);
	backlog = SOCKET_GET_BACKLOG(call);

	if (backlog < 0) {
		async_answer_0(callid, EINVAL);
		return;
	}

	sock_core = socket_cores_find(&client->sockets, socket_id);
	if (sock_core == NULL) {
		async_answer_0(callid, ENOTSOCK);
		return;
	}

	socket = (tcp_sockdata_t *)sock_core->specific_data;
	/* XXX Listen */
	(void)backlog;
	(void)socket;

	async_answer_0(callid, ENOTSUP);

	/* Push one fragment notification to client's queue */
	tcp_sock_notify_data(sock_core);
}

static void tcp_sock_connect(tcp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	int rc;
	struct sockaddr_in *addr;
	int socket_id;
	size_t addr_len;
	socket_core_t *sock_core;
	tcp_sockdata_t *socket;
	tcp_error_t trc;
	tcp_sock_t fsocket;
	uint16_t lport;

	log_msg(LVL_DEBUG, "tcp_sock_connect()");

	rc = async_data_write_accept((void **) &addr, false, 0, 0, 0, &addr_len);
	if (rc != EOK || addr_len != sizeof(struct sockaddr_in)) {
		async_answer_0(callid, rc);
		return;
	}

	socket_id = SOCKET_GET_SOCKET_ID(call);

	sock_core = socket_cores_find(&client->sockets, socket_id);
	if (sock_core == NULL) {
		async_answer_0(callid, ENOTSOCK);
		return;
	}

	socket = (tcp_sockdata_t *)sock_core->specific_data;

	lport = 1024; /* XXX */
	fsocket.addr.ipv4 = uint32_t_be2host(addr->sin_addr.s_addr);
	fsocket.port = uint16_t_be2host(addr->sin_port);

	trc = tcp_uc_open(lport, &fsocket, ap_active, &socket->conn);

	switch (trc) {
	case TCP_EOK:
		rc = EOK;
		break;
	case TCP_ERESET:
		rc = ECONNREFUSED;
		break;
	default:
		assert(false);
	}

	async_answer_0(callid, rc);
}

static void tcp_sock_accept(tcp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	ipc_call_t answer;
	int socket_id;
	int nsocket_id;
	socket_core_t *sock_core;
	tcp_sockdata_t *socket;

	log_msg(LVL_DEBUG, "tcp_sock_accept()");

	socket_id = SOCKET_GET_SOCKET_ID(call);
	nsocket_id = SOCKET_GET_NEW_SOCKET_ID(call);

	sock_core = socket_cores_find(&client->sockets, socket_id);
	if (sock_core == NULL) {
		async_answer_0(callid, ENOTSOCK);
		return;
	}

	socket = (tcp_sockdata_t *)sock_core->specific_data;
	/* XXX Accept */
	(void) socket;
	(void) nsocket_id;
/*
	refresh_answer(&answer, NULL);
	SOCKET_SET_DATA_FRAGMENT_SIZE(answer, size)
	...
	async_answer_0(callid, ENOTSUP);*/

	/* TODO */
	answer_call(callid, ENOTSUP, &answer, 3);
}

static void tcp_sock_send(tcp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	int socket_id;
	int fragments;
	int index;
	socket_core_t *sock_core;
	tcp_sockdata_t *socket;
	ipc_call_t answer;
	ipc_callid_t wcallid;
	size_t length;
	uint8_t buffer[FRAGMENT_SIZE];
	tcp_error_t trc;
	int rc;

	log_msg(LVL_DEBUG, "******** tcp_sock_send() *******************");
	socket_id = SOCKET_GET_SOCKET_ID(call);
	fragments = SOCKET_GET_DATA_FRAGMENTS(call);
	SOCKET_GET_FLAGS(call);

	sock_core = socket_cores_find(&client->sockets, socket_id);
	if (sock_core == NULL) {
		async_answer_0(callid, ENOTSOCK);
		return;
	}

	socket = (tcp_sockdata_t *)sock_core->specific_data;
	if (socket->conn == NULL) {
		async_answer_0(callid, ENOTCONN);
		return;
	}

	for (index = 0; index < fragments; index++) {
		if (!async_data_write_receive(&wcallid, &length)) {
			async_answer_0(callid, EINVAL);
			return;
		}

		if (length > FRAGMENT_SIZE)
			length = FRAGMENT_SIZE;

		rc = async_data_write_finalize(wcallid, buffer, length);
		if (rc != EOK) {
			async_answer_0(callid, rc);
			return;
		}

		trc = tcp_uc_send(socket->conn, buffer, length, 0);

		switch (trc) {
		case TCP_EOK:
			rc = EOK;
			break;
		case TCP_ENOTEXIST:
			rc = ENOTCONN;
			break;
		case TCP_ECLOSING:
			rc = ENOTCONN;
			break;
		default:
			assert(false);
		}

		if (rc != EOK) {
			async_answer_0(callid, rc);
			return;
		}
	}

	refresh_answer(&answer, NULL);
	SOCKET_SET_DATA_FRAGMENT_SIZE(answer, FRAGMENT_SIZE);
	answer_call(callid, EOK, &answer, 2);
}

static void tcp_sock_sendto(tcp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	log_msg(LVL_DEBUG, "tcp_sock_sendto()");
	async_answer_0(callid, ENOTSUP);
}

static void tcp_sock_recv(tcp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	int socket_id;
	int flags;
	size_t length;
	socket_core_t *sock_core;
	tcp_sockdata_t *socket;
	ipc_call_t answer;
	ipc_callid_t rcallid;
	uint8_t buffer[FRAGMENT_SIZE];
	size_t data_len;
	xflags_t xflags;
	tcp_error_t trc;
	int rc;

	log_msg(LVL_DEBUG, "tcp_sock_recv()");

	socket_id = SOCKET_GET_SOCKET_ID(call);
	flags = SOCKET_GET_FLAGS(call);

	sock_core = socket_cores_find(&client->sockets, socket_id);
	if (sock_core == NULL) {
		async_answer_0(callid, ENOTSOCK);
		return;
	}

	socket = (tcp_sockdata_t *)sock_core->specific_data;

	(void)flags;

	trc = tcp_uc_receive(socket->conn, buffer, FRAGMENT_SIZE, &data_len,
	    &xflags);

	switch (trc) {
	case TCP_EOK:
		rc = EOK;
		break;
	case TCP_ENOTEXIST:
	case TCP_ECLOSING:
		rc = ENOTCONN;
		break;
	default:
		assert(false);
	}

	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}

	if (!async_data_read_receive(&rcallid, &length)) {
		async_answer_0(callid, EINVAL);
		return;
	}

	if (length < data_len) {
		async_data_read_finalize(rcallid, buffer, length);
		if (rc == EOK)
			rc = EOVERFLOW;
	} else {
		length = data_len;
	}

	rc = async_data_read_finalize(rcallid, buffer, length);
	length = 0;

	SOCKET_SET_READ_DATA_LENGTH(answer, length);
	answer_call(callid, EOK, &answer, 1);

	/* Push one fragment notification to client's queue */
	tcp_sock_notify_data(sock_core);
}

static void tcp_sock_recvfrom(tcp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	log_msg(LVL_DEBUG, "tcp_sock_recvfrom()");
	async_answer_0(callid, ENOTSUP);
}

static void tcp_sock_close(tcp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	int socket_id;
	socket_core_t *sock_core;
	tcp_sockdata_t *socket;
	int rc;

	log_msg(LVL_DEBUG, "tcp_sock_close()");
	socket_id = SOCKET_GET_SOCKET_ID(call);

	sock_core = socket_cores_find(&client->sockets, socket_id);
	if (sock_core == NULL) {
		async_answer_0(callid, ENOTSOCK);
		return;
	}

	socket = (tcp_sockdata_t *)sock_core->specific_data;
	(void) socket;
	/* XXX Close */

	rc = socket_destroy(net_sess, socket_id, &client->sockets, &gsock,
	    tcp_free_sock_data);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}

	async_answer_0(callid, EOK);
}

static void tcp_sock_getsockopt(tcp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	log_msg(LVL_DEBUG, "tcp_sock_getsockopt()");
	async_answer_0(callid, ENOTSUP);
}

static void tcp_sock_setsockopt(tcp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	log_msg(LVL_DEBUG, "tcp_sock_setsockopt()");
	async_answer_0(callid, ENOTSUP);
}

int tcp_sock_connection(async_sess_t *sess, ipc_callid_t iid, ipc_call_t icall)
{
	ipc_callid_t callid;
	ipc_call_t call;
	tcp_client_t client;

	/* Accept the connection */
	async_answer_0(iid, EOK);

	client.sess = sess;
	socket_cores_initialize(&client.sockets);

	while (true) {
		callid = async_get_call(&call);
		if (!IPC_GET_IMETHOD(call))
			break;

		log_msg(LVL_DEBUG, "tcp_sock_connection: METHOD=%d\n",
		    (int)IPC_GET_IMETHOD(call));

		switch (IPC_GET_IMETHOD(call)) {
		case NET_SOCKET:
			tcp_sock_socket(&client, callid, call);
			break;
		case NET_SOCKET_BIND:
			tcp_sock_bind(&client, callid, call);
			break;
		case NET_SOCKET_LISTEN:
			tcp_sock_listen(&client, callid, call);
			break;
		case NET_SOCKET_CONNECT:
			tcp_sock_connect(&client, callid, call);
			break;
		case NET_SOCKET_ACCEPT:
			tcp_sock_accept(&client, callid, call);
			break;
		case NET_SOCKET_SEND:
			tcp_sock_send(&client, callid, call);
			break;
		case NET_SOCKET_SENDTO:
			tcp_sock_sendto(&client, callid, call);
			break;
		case NET_SOCKET_RECV:
			tcp_sock_recv(&client, callid, call);
			break;
		case NET_SOCKET_RECVFROM:
			tcp_sock_recvfrom(&client, callid, call);
			break;
		case NET_SOCKET_CLOSE:
			tcp_sock_close(&client, callid, call);
			break;
		case NET_SOCKET_GETSOCKOPT:
			tcp_sock_getsockopt(&client, callid, call);
			break;
		case NET_SOCKET_SETSOCKOPT:
			tcp_sock_setsockopt(&client, callid, call);
			break;
		default:
			async_answer_0(callid, ENOTSUP);
			break;
		}
	}

	return EOK;
}

/**
 * @}
 */
