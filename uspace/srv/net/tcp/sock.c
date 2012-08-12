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
#include <inet/inet.h>
#include <io/log.h>
#include <ipc/services.h>
#include <ipc/socket.h>
#include <net/socket.h>
#include <ns.h>

#include "sock.h"
#include "std.h"
#include "tcp.h"
#include "tcp_type.h"
#include "ucall.h"

#define MAX_BACKLOG 128

/** Free ports pool start. */
#define TCP_FREE_PORTS_START		1025

/** Free ports pool end. */
#define TCP_FREE_PORTS_END		65535

static int last_used_port = TCP_FREE_PORTS_START - 1;
static socket_ports_t gsock;

static void tcp_sock_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg);
static void tcp_sock_cstate_cb(tcp_conn_t *conn, void *arg);
static int tcp_sock_recv_fibril(void *arg);

int tcp_sock_init(void)
{
	socket_ports_initialize(&gsock);
	
	async_set_client_connection(tcp_sock_connection);
	
	int rc = service_register(SERVICE_TCP);
	if (rc != EOK)
		return EEXIST;
	
	return EOK;
}

static void tcp_free_sock_data(socket_core_t *sock_core)
{
	tcp_sockdata_t *socket;

	socket = (tcp_sockdata_t *)sock_core->specific_data;
	(void)socket;

	/* XXX We need to initiate connection cleanup here */
}

static void tcp_sock_notify_data(socket_core_t *sock_core)
{
	log_msg(LVL_DEBUG, "tcp_sock_notify_data(%d)", sock_core->socket_id);
	async_exch_t *exch = async_exchange_begin(sock_core->sess);
	async_msg_5(exch, NET_SOCKET_RECEIVED, (sysarg_t)sock_core->socket_id,
	    TCP_SOCK_FRAGMENT_SIZE, 0, 0, 1);
	async_exchange_end(exch);
}

static void tcp_sock_notify_aconn(socket_core_t *lsock_core)
{
	log_msg(LVL_DEBUG, "tcp_sock_notify_aconn(%d)", lsock_core->socket_id);
	async_exch_t *exch = async_exchange_begin(lsock_core->sess);
	async_msg_5(exch, NET_SOCKET_ACCEPTED, (sysarg_t)lsock_core->socket_id,
	    TCP_SOCK_FRAGMENT_SIZE, 0, 0, 0);
	async_exchange_end(exch);
}

static int tcp_sock_create(tcp_client_t *client, tcp_sockdata_t **rsock)
{
	tcp_sockdata_t *sock;

	log_msg(LVL_DEBUG, "tcp_sock_create()");
	*rsock = NULL;

	sock = calloc(sizeof(tcp_sockdata_t), 1);
	if (sock == NULL)
		return ENOMEM;

	fibril_mutex_initialize(&sock->lock);
	sock->client = client;

	sock->recv_buffer_used = 0;
	sock->recv_error = TCP_EOK;
	fibril_mutex_initialize(&sock->recv_buffer_lock);
	fibril_condvar_initialize(&sock->recv_buffer_cv);
	list_initialize(&sock->ready);

	*rsock = sock;
	return EOK;
}

static void tcp_sock_uncreate(tcp_sockdata_t *sock)
{
	log_msg(LVL_DEBUG, "tcp_sock_uncreate()");
	free(sock);
}

static int tcp_sock_finish_setup(tcp_sockdata_t *sock, int *sock_id)
{
	socket_core_t *sock_core;
	int rc;

	log_msg(LVL_DEBUG, "tcp_sock_finish_setup()");

	sock->recv_fibril = fibril_create(tcp_sock_recv_fibril, sock);
	if (sock->recv_fibril == 0)
		return ENOMEM;

	rc = socket_create(&sock->client->sockets, sock->client->sess,
	    sock, sock_id);

	if (rc != EOK) {
		fibril_destroy(sock->recv_fibril);
		sock->recv_fibril = 0;
		return rc;
	}

	sock_core = socket_cores_find(&sock->client->sockets, *sock_id);
	assert(sock_core != NULL);
	sock->sock_core = sock_core;

	return EOK;
}

static void tcp_sock_socket(tcp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	tcp_sockdata_t *sock;
	int sock_id;
	int rc;
	ipc_call_t answer;

	log_msg(LVL_DEBUG, "tcp_sock_socket()");

	rc = tcp_sock_create(client, &sock);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}

	sock->laddr.ipv4 = TCP_IPV4_ANY;
	sock->lconn = NULL;
	sock->backlog = 0;

	sock_id = SOCKET_GET_SOCKET_ID(call);
	rc = tcp_sock_finish_setup(sock, &sock_id);
	if (rc != EOK) {
		tcp_sock_uncreate(sock);
		async_answer_0(callid, rc);
		return;
	}

	SOCKET_SET_SOCKET_ID(answer, sock_id);

	SOCKET_SET_DATA_FRAGMENT_SIZE(answer, TCP_SOCK_FRAGMENT_SIZE);
	SOCKET_SET_HEADER_SIZE(answer, sizeof(tcp_header_t));
	
	async_answer_3(callid, EOK, IPC_GET_ARG1(answer),
	    IPC_GET_ARG2(answer), IPC_GET_ARG3(answer));
}

static void tcp_sock_bind(tcp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	int rc;
	struct sockaddr *addr;
	size_t addr_len;
	socket_core_t *sock_core;
	tcp_sockdata_t *socket;

	log_msg(LVL_DEBUG, "tcp_sock_bind()");
	log_msg(LVL_DEBUG, " - async_data_write_accept");
	rc = async_data_write_accept((void **) &addr, false, 0, 0, 0, &addr_len);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}

	log_msg(LVL_DEBUG, " - call socket_bind");
	rc = socket_bind(&client->sockets, &gsock, SOCKET_GET_SOCKET_ID(call),
	    addr, addr_len, TCP_FREE_PORTS_START, TCP_FREE_PORTS_END,
	    last_used_port);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}

	log_msg(LVL_DEBUG, " - call socket_cores_find");
	sock_core = socket_cores_find(&client->sockets, SOCKET_GET_SOCKET_ID(call));
	if (sock_core != NULL) {
		socket = (tcp_sockdata_t *)sock_core->specific_data;
		/* XXX Anything to do? */
		(void) socket;
	}

	log_msg(LVL_DEBUG, " - success");
	async_answer_0(callid, EOK);
}

static void tcp_sock_listen(tcp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	int socket_id;
	int backlog;
	socket_core_t *sock_core;
	tcp_sockdata_t *socket;
	tcp_error_t trc;
	tcp_sock_t lsocket;
	tcp_sock_t fsocket;
	tcp_conn_t *conn;
	tcp_sock_lconn_t *lconn;
	int i;

	log_msg(LVL_DEBUG, "tcp_sock_listen()");

	socket_id = SOCKET_GET_SOCKET_ID(call);
	backlog = SOCKET_GET_BACKLOG(call);

	if (backlog < 0) {
		async_answer_0(callid, EINVAL);
		return;
	}

	if (backlog > MAX_BACKLOG)
		backlog = MAX_BACKLOG;

	sock_core = socket_cores_find(&client->sockets, socket_id);
	if (sock_core == NULL) {
		async_answer_0(callid, ENOTSOCK);
		return;
	}

	socket = (tcp_sockdata_t *)sock_core->specific_data;

	/*
	 * Prepare @c backlog listening connections.
	 */
	fibril_mutex_lock(&socket->lock);

	socket->backlog = backlog;
	socket->lconn = calloc(sizeof(tcp_conn_t *), backlog);
	if (socket->lconn == NULL) {
		fibril_mutex_unlock(&socket->lock);
		async_answer_0(callid, ENOMEM);
		return;
	}

	log_msg(LVL_DEBUG, " - open connections");

	lsocket.addr.ipv4 = TCP_IPV4_ANY;
	lsocket.port = sock_core->port;
	fsocket.addr.ipv4 = TCP_IPV4_ANY;
	fsocket.port = TCP_PORT_ANY;

	for (i = 0; i < backlog; i++) {

		lconn = calloc(sizeof(tcp_sock_lconn_t), 1);
		if (lconn == NULL) {
			/* XXX Clean up */
			fibril_mutex_unlock(&socket->lock);
			async_answer_0(callid, ENOMEM);
			return;
		}

		trc = tcp_uc_open(&lsocket, &fsocket, ap_passive,
		    tcp_open_nonblock, &conn);
		if (conn == NULL) {
			/* XXX Clean up */
			fibril_mutex_unlock(&socket->lock);
			async_answer_0(callid, ENOMEM);
			return;
		}

		tcp_uc_set_cstate_cb(conn, tcp_sock_cstate_cb, lconn);

		assert(trc == TCP_EOK);
		conn->name = (char *)"S";

		lconn->conn = conn;
		lconn->socket = socket;
		link_initialize(&lconn->ready_list);
		socket->lconn[i] = lconn;
	}

	fibril_mutex_unlock(&socket->lock);
	async_answer_0(callid, EOK);
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
	tcp_sock_t lsocket;
	tcp_sock_t fsocket;

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
	if (sock_core->port <= 0) {
		rc = socket_bind_free_port(&gsock, sock_core,
		    TCP_FREE_PORTS_START, TCP_FREE_PORTS_END,
		    last_used_port);
		if (rc != EOK) {
			async_answer_0(callid, rc);
			return;
		}

		last_used_port = sock_core->port;
	}

	fibril_mutex_lock(&socket->lock);

	if (socket->laddr.ipv4 == TCP_IPV4_ANY) {
		/* Determine local IP address */
		inet_addr_t loc_addr, rem_addr;

		rem_addr.ipv4 = uint32_t_be2host(addr->sin_addr.s_addr);
		rc = inet_get_srcaddr(&rem_addr, 0, &loc_addr);
		if (rc != EOK) {
			fibril_mutex_unlock(&socket->lock);
			async_answer_0(callid, rc);
			log_msg(LVL_DEBUG, "tcp_sock_connect: Failed to "
			    "determine local address.");
			return;
		}

		socket->laddr.ipv4 = loc_addr.ipv4;
		log_msg(LVL_DEBUG, "Local IP address is %x", socket->laddr.ipv4);
	}

	lsocket.addr.ipv4 = socket->laddr.ipv4;
	lsocket.port = sock_core->port;
	fsocket.addr.ipv4 = uint32_t_be2host(addr->sin_addr.s_addr);
	fsocket.port = uint16_t_be2host(addr->sin_port);

	trc = tcp_uc_open(&lsocket, &fsocket, ap_active, 0, &socket->conn);

	if (socket->conn != NULL)
		socket->conn->name = (char *)"C";

	fibril_mutex_unlock(&socket->lock);

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

	if (rc == EOK)
		fibril_add_ready(socket->recv_fibril);

	async_answer_0(callid, rc);
}

static void tcp_sock_accept(tcp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	ipc_call_t answer;
	int socket_id;
	int asock_id;
	socket_core_t *sock_core;
	tcp_sockdata_t *socket;
	tcp_sockdata_t *asocket;
	tcp_error_t trc;
	tcp_sock_t lsocket;
	tcp_sock_t fsocket;
	tcp_conn_t *conn;
	tcp_conn_t *rconn;
	tcp_sock_lconn_t *lconn;
	int rc;

	log_msg(LVL_DEBUG, "tcp_sock_accept()");

	socket_id = SOCKET_GET_SOCKET_ID(call);
	asock_id = SOCKET_GET_NEW_SOCKET_ID(call);

	sock_core = socket_cores_find(&client->sockets, socket_id);
	if (sock_core == NULL) {
		async_answer_0(callid, ENOTSOCK);
		return;
	}

	socket = (tcp_sockdata_t *)sock_core->specific_data;
	fibril_mutex_lock(&socket->lock);

	log_msg(LVL_DEBUG, " - verify socket->conn");
	if (socket->conn != NULL) {
		fibril_mutex_unlock(&socket->lock);
		async_answer_0(callid, EINVAL);
		return;
	}

	if (list_empty(&socket->ready)) {
		fibril_mutex_unlock(&socket->lock);
		async_answer_0(callid, ENOENT);
		return;
	}

	lconn = list_get_instance(list_first(&socket->ready),
	    tcp_sock_lconn_t, ready_list);
	list_remove(&lconn->ready_list);

	conn = lconn->conn;
	tcp_uc_set_cstate_cb(conn, NULL, NULL);

	/* Replenish listening connection */

	lsocket.addr.ipv4 = TCP_IPV4_ANY;
	lsocket.port = sock_core->port;
	fsocket.addr.ipv4 = TCP_IPV4_ANY;
	fsocket.port = TCP_PORT_ANY;

	trc = tcp_uc_open(&lsocket, &fsocket, ap_passive, tcp_open_nonblock,
	    &rconn);
	if (rconn == NULL) {
		/* XXX Clean up */
		fibril_mutex_unlock(&socket->lock);
		async_answer_0(callid, ENOMEM);
		return;
	}

	tcp_uc_set_cstate_cb(rconn, tcp_sock_cstate_cb, lconn);

	assert(trc == TCP_EOK);
	rconn->name = (char *)"S";

	lconn->conn = rconn;

	/* Allocate socket for accepted connection */

	rc = tcp_sock_create(client, &asocket);
	if (rc != EOK) {
		fibril_mutex_unlock(&socket->lock);
		async_answer_0(callid, rc);
		return;
	}

	asocket->conn = conn;
	log_msg(LVL_DEBUG, "tcp_sock_accept():create asocket\n");

	rc = tcp_sock_finish_setup(asocket, &asock_id);
	if (rc != EOK) {
		tcp_sock_uncreate(asocket);
		fibril_mutex_unlock(&socket->lock);
		async_answer_0(callid, rc);
		return;
	}

	fibril_add_ready(asocket->recv_fibril);

	log_msg(LVL_DEBUG, "tcp_sock_accept(): find acore\n");

	SOCKET_SET_DATA_FRAGMENT_SIZE(answer, TCP_SOCK_FRAGMENT_SIZE);
	SOCKET_SET_SOCKET_ID(answer, asock_id);
	SOCKET_SET_ADDRESS_LENGTH(answer, sizeof(struct sockaddr_in));
	
	async_answer_3(callid, asocket->sock_core->socket_id,
	    IPC_GET_ARG1(answer), IPC_GET_ARG2(answer),
	    IPC_GET_ARG3(answer));
	
	/* Push one fragment notification to client's queue */
	log_msg(LVL_DEBUG, "tcp_sock_accept(): notify data\n");
	fibril_mutex_unlock(&socket->lock);
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
	uint8_t buffer[TCP_SOCK_FRAGMENT_SIZE];
	tcp_error_t trc;
	int rc;

	log_msg(LVL_DEBUG, "tcp_sock_send()");
	socket_id = SOCKET_GET_SOCKET_ID(call);
	fragments = SOCKET_GET_DATA_FRAGMENTS(call);
	SOCKET_GET_FLAGS(call);

	sock_core = socket_cores_find(&client->sockets, socket_id);
	if (sock_core == NULL) {
		async_answer_0(callid, ENOTSOCK);
		return;
	}

	socket = (tcp_sockdata_t *)sock_core->specific_data;
	fibril_mutex_lock(&socket->lock);

	if (socket->conn == NULL) {
		fibril_mutex_unlock(&socket->lock);
		async_answer_0(callid, ENOTCONN);
		return;
	}

	for (index = 0; index < fragments; index++) {
		if (!async_data_write_receive(&wcallid, &length)) {
			fibril_mutex_unlock(&socket->lock);
			async_answer_0(callid, EINVAL);
			return;
		}

		if (length > TCP_SOCK_FRAGMENT_SIZE)
			length = TCP_SOCK_FRAGMENT_SIZE;

		rc = async_data_write_finalize(wcallid, buffer, length);
		if (rc != EOK) {
			fibril_mutex_unlock(&socket->lock);
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
		case TCP_ERESET:
			rc = ECONNABORTED;
			break;
		default:
			assert(false);
		}

		if (rc != EOK) {
			fibril_mutex_unlock(&socket->lock);
			async_answer_0(callid, rc);
			return;
		}
	}

	IPC_SET_ARG1(answer, 0);
	SOCKET_SET_DATA_FRAGMENT_SIZE(answer, TCP_SOCK_FRAGMENT_SIZE);
	async_answer_2(callid, EOK, IPC_GET_ARG1(answer),
	    IPC_GET_ARG2(answer));
	fibril_mutex_unlock(&socket->lock);
}

static void tcp_sock_sendto(tcp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	log_msg(LVL_DEBUG, "tcp_sock_sendto()");
	async_answer_0(callid, ENOTSUP);
}

static void tcp_sock_recvfrom(tcp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	int socket_id;
	int flags;
	size_t addr_length, length;
	socket_core_t *sock_core;
	tcp_sockdata_t *socket;
	ipc_call_t answer;
	ipc_callid_t rcallid;
	size_t data_len;
	struct sockaddr_in addr;
	tcp_sock_t *rsock;
	int rc;

	log_msg(LVL_DEBUG, "%p: tcp_sock_recv[from]()", client);

	socket_id = SOCKET_GET_SOCKET_ID(call);
	flags = SOCKET_GET_FLAGS(call);

	sock_core = socket_cores_find(&client->sockets, socket_id);
	if (sock_core == NULL) {
		async_answer_0(callid, ENOTSOCK);
		return;
	}

	socket = (tcp_sockdata_t *)sock_core->specific_data;
	fibril_mutex_lock(&socket->lock);

	if (socket->conn == NULL) {
		fibril_mutex_unlock(&socket->lock);
		async_answer_0(callid, ENOTCONN);
		return;
	}

	(void)flags;

	log_msg(LVL_DEBUG, "tcp_sock_recvfrom(): lock recv_buffer_lock");
	fibril_mutex_lock(&socket->recv_buffer_lock);
	while (socket->recv_buffer_used == 0 && socket->recv_error == TCP_EOK) {
		log_msg(LVL_DEBUG, "wait for recv_buffer_cv + recv_buffer_used != 0");
		fibril_condvar_wait(&socket->recv_buffer_cv,
		    &socket->recv_buffer_lock);
	}

	log_msg(LVL_DEBUG, "Got data in sock recv_buffer");

	data_len = socket->recv_buffer_used;
	rc = socket->recv_error;

	switch (socket->recv_error) {
	case TCP_EOK:
		rc = EOK;
		break;
	case TCP_ENOTEXIST:
	case TCP_ECLOSING:
		rc = ENOTCONN;
		break;
	case TCP_ERESET:
		rc = ECONNABORTED;
		break;
	default:
		assert(false);
	}

	log_msg(LVL_DEBUG, "**** recv result -> %d", rc);
	if (rc != EOK) {
		fibril_mutex_unlock(&socket->recv_buffer_lock);
		fibril_mutex_unlock(&socket->lock);
		async_answer_0(callid, rc);
		return;
	}

	if (IPC_GET_IMETHOD(call) == NET_SOCKET_RECVFROM) {
		/* Fill addr */
		rsock = &socket->conn->ident.foreign;
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = host2uint32_t_be(rsock->addr.ipv4);
		addr.sin_port = host2uint16_t_be(rsock->port);

		log_msg(LVL_DEBUG, "addr read receive");
		if (!async_data_read_receive(&rcallid, &addr_length)) {
			fibril_mutex_unlock(&socket->recv_buffer_lock);
			fibril_mutex_unlock(&socket->lock);
			async_answer_0(callid, EINVAL);
			return;
		}

		if (addr_length > sizeof(addr))
			addr_length = sizeof(addr);

		log_msg(LVL_DEBUG, "addr read finalize");
		rc = async_data_read_finalize(rcallid, &addr, addr_length);
		if (rc != EOK) {
			fibril_mutex_unlock(&socket->recv_buffer_lock);
			fibril_mutex_unlock(&socket->lock);
			async_answer_0(callid, EINVAL);
			return;
		}
	}

	log_msg(LVL_DEBUG, "data read receive");
	if (!async_data_read_receive(&rcallid, &length)) {
		fibril_mutex_unlock(&socket->recv_buffer_lock);
		fibril_mutex_unlock(&socket->lock);
		async_answer_0(callid, EINVAL);
		return;
	}

	if (length > data_len)
		length = data_len;

	log_msg(LVL_DEBUG, "data read finalize");
	rc = async_data_read_finalize(rcallid, socket->recv_buffer, length);

	socket->recv_buffer_used -= length;
	log_msg(LVL_DEBUG, "tcp_sock_recvfrom: %zu left in buffer",
	    socket->recv_buffer_used);
	if (socket->recv_buffer_used > 0) {
		memmove(socket->recv_buffer, socket->recv_buffer + length,
		    socket->recv_buffer_used);
		tcp_sock_notify_data(socket->sock_core);
	}

	fibril_condvar_broadcast(&socket->recv_buffer_cv);

	if (length < data_len && rc == EOK)
		rc = EOVERFLOW;

	SOCKET_SET_READ_DATA_LENGTH(answer, length);
	async_answer_1(callid, EOK, IPC_GET_ARG1(answer));

	fibril_mutex_unlock(&socket->recv_buffer_lock);
	fibril_mutex_unlock(&socket->lock);
}

static void tcp_sock_close(tcp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	int socket_id;
	socket_core_t *sock_core;
	tcp_sockdata_t *socket;
	tcp_error_t trc;
	int rc;

	log_msg(LVL_DEBUG, "tcp_sock_close()");
	socket_id = SOCKET_GET_SOCKET_ID(call);

	sock_core = socket_cores_find(&client->sockets, socket_id);
	if (sock_core == NULL) {
		async_answer_0(callid, ENOTSOCK);
		return;
	}

	socket = (tcp_sockdata_t *)sock_core->specific_data;
	fibril_mutex_lock(&socket->lock);

	if (socket->conn != NULL) {
		trc = tcp_uc_close(socket->conn);
		if (trc != TCP_EOK && trc != TCP_ENOTEXIST) {
			fibril_mutex_unlock(&socket->lock);
			async_answer_0(callid, EBADF);
			return;
		}
	}

	/* Grab recv_buffer_lock because of CV wait in tcp_sock_recv_fibril() */
	fibril_mutex_lock(&socket->recv_buffer_lock);
	socket->sock_core = NULL;
	fibril_mutex_unlock(&socket->recv_buffer_lock);

	rc = socket_destroy(NULL, socket_id, &client->sockets, &gsock,
	    tcp_free_sock_data);
	if (rc != EOK) {
		fibril_mutex_unlock(&socket->lock);
		async_answer_0(callid, rc);
		return;
	}

	fibril_mutex_unlock(&socket->lock);
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

/** Called when connection state changes. */
static void tcp_sock_cstate_cb(tcp_conn_t *conn, void *arg)
{
	tcp_conn_status_t cstatus;
	tcp_sock_lconn_t *lconn = (tcp_sock_lconn_t *)arg;
	tcp_sockdata_t *socket = lconn->socket;

	log_msg(LVL_DEBUG, "tcp_sock_cstate_cb()");
	fibril_mutex_lock(&socket->lock);
	assert(conn == lconn->conn);

	tcp_uc_status(conn, &cstatus);
	if (cstatus.cstate != st_established) {
		fibril_mutex_unlock(&socket->lock);
		return;
	}

	assert_link_not_used(&lconn->ready_list);
	list_append(&lconn->ready_list, &socket->ready);

	log_msg(LVL_DEBUG, "tcp_sock_cstate_cb(): notify accept");

	/* Push one accept notification to client's queue */
	tcp_sock_notify_aconn(socket->sock_core);
	fibril_mutex_unlock(&socket->lock);
}

static int tcp_sock_recv_fibril(void *arg)
{
	tcp_sockdata_t *sock = (tcp_sockdata_t *)arg;
	size_t data_len;
	xflags_t xflags;
	tcp_error_t trc;

	log_msg(LVL_DEBUG, "tcp_sock_recv_fibril()");

	fibril_mutex_lock(&sock->recv_buffer_lock);

	while (true) {
		log_msg(LVL_DEBUG, "call tcp_uc_receive()");
		while (sock->recv_buffer_used != 0 && sock->sock_core != NULL)
			fibril_condvar_wait(&sock->recv_buffer_cv,
			    &sock->recv_buffer_lock);

		trc = tcp_uc_receive(sock->conn, sock->recv_buffer,
		    TCP_SOCK_FRAGMENT_SIZE, &data_len, &xflags);

		if (trc != TCP_EOK) {
			sock->recv_error = trc;
			fibril_condvar_broadcast(&sock->recv_buffer_cv);
			if (sock->sock_core != NULL)
				tcp_sock_notify_data(sock->sock_core);
			break;
		}

		log_msg(LVL_DEBUG, "got data - broadcast recv_buffer_cv");

		sock->recv_buffer_used = data_len;
		fibril_condvar_broadcast(&sock->recv_buffer_cv);
		if (sock->sock_core != NULL)
			tcp_sock_notify_data(sock->sock_core);
	}

	fibril_mutex_unlock(&sock->recv_buffer_lock);

	tcp_uc_delete(sock->conn);

	return 0;
}

static void tcp_sock_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	ipc_callid_t callid;
	ipc_call_t call;
	tcp_client_t client;

	/* Accept the connection */
	async_answer_0(iid, EOK);

	client.sess = async_callback_receive(EXCHANGE_SERIALIZE);
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

	/* Clean up */
	log_msg(LVL_DEBUG, "tcp_sock_connection: Clean up");
	async_hangup(client.sess);
	socket_cores_release(NULL, &client.sockets, &gsock, tcp_free_sock_data);
}

/**
 * @}
 */
