/*
 * Copyright (c) 2008 Lukas Mejdrech
 * Copyright (c) 2013 Jiri Svoboda
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
 * @file Socket provider
 */

#include <async.h>
#include <byteorder.h>
#include <errno.h>
#include <inet/inet.h>
#include <io/log.h>
#include <ipc/services.h>
#include <ipc/socket.h>
#include <net/socket.h>
#include <ns.h>

#include "sock.h"
#include "std.h"
#include "udp_type.h"
#include "ucall.h"

/** Free ports pool start. */
#define UDP_FREE_PORTS_START		1025

/** Free ports pool end. */
#define UDP_FREE_PORTS_END		65535

static int last_used_port = UDP_FREE_PORTS_START - 1;
static socket_ports_t gsock;

static void udp_sock_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg);
static int udp_sock_recv_fibril(void *arg);

int udp_sock_init(void)
{
	socket_ports_initialize(&gsock);
	
	async_set_client_connection(udp_sock_connection);
	
	int rc = service_register(SERVICE_UDP);
	if (rc != EOK)
		return EEXIST;
	
	return EOK;
}

static void udp_free_sock_data(socket_core_t *sock_core)
{
	udp_sockdata_t *socket;

	socket = (udp_sockdata_t *)sock_core->specific_data;
	(void)socket;

	/* XXX We need to force the receive fibril to quit */
}

static void udp_sock_notify_data(socket_core_t *sock_core)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_sock_notify_data(%d)", sock_core->socket_id);
	async_exch_t *exch = async_exchange_begin(sock_core->sess);
	async_msg_5(exch, NET_SOCKET_RECEIVED, (sysarg_t) sock_core->socket_id,
	    UDP_FRAGMENT_SIZE, 0, 0, 1);
	async_exchange_end(exch);
}

static void udp_sock_socket(udp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	udp_sockdata_t *sock;
	socket_core_t *sock_core;
	int sock_id;
	int rc;
	ipc_call_t answer;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_sock_socket()");
	sock = calloc(1, sizeof(udp_sockdata_t));
	if (sock == NULL) {
		async_answer_0(callid, ENOMEM);
		return;
	}

	fibril_mutex_initialize(&sock->lock);
	sock->client = client;

	sock->recv_buffer_used = 0;
	sock->recv_error = UDP_EOK;
	fibril_mutex_initialize(&sock->recv_buffer_lock);
	fibril_condvar_initialize(&sock->recv_buffer_cv);

	rc = udp_uc_create(&sock->assoc);
	if (rc != EOK) {
		free(sock);
		async_answer_0(callid, rc);
		return;
	}

	sock->recv_fibril = fibril_create(udp_sock_recv_fibril, sock);
	if (sock->recv_fibril == 0) {
		udp_uc_destroy(sock->assoc);
		free(sock);
		async_answer_0(callid, ENOMEM);
		return;
	}

	sock_id = SOCKET_GET_SOCKET_ID(call);
	rc = socket_create(&client->sockets, client->sess, sock, &sock_id);
	if (rc != EOK) {
		fibril_destroy(sock->recv_fibril);
		udp_uc_destroy(sock->assoc);
		free(sock);
		async_answer_0(callid, rc);
		return;
	}

	fibril_add_ready(sock->recv_fibril);

	sock_core = socket_cores_find(&client->sockets, sock_id);
	assert(sock_core != NULL);
	sock->sock_core = sock_core;
	
	SOCKET_SET_SOCKET_ID(answer, sock_id);

	SOCKET_SET_DATA_FRAGMENT_SIZE(answer, UDP_FRAGMENT_SIZE);
	SOCKET_SET_HEADER_SIZE(answer, sizeof(udp_header_t));
	async_answer_3(callid, EOK, IPC_GET_ARG1(answer),
	    IPC_GET_ARG2(answer), IPC_GET_ARG3(answer));
}

static void udp_sock_bind(udp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_sock_bind()");
	log_msg(LOG_DEFAULT, LVL_DEBUG, " - async_data_write_accept");
	
	struct sockaddr_in6 *addr6 = NULL;
	size_t addr_len;
	int rc = async_data_write_accept((void **) &addr6, false, 0, 0, 0, &addr_len);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}
	
	if ((addr_len != sizeof(struct sockaddr_in)) &&
	    (addr_len != sizeof(struct sockaddr_in6))) {
		async_answer_0(callid, EINVAL);
		goto out;
	}
	
	struct sockaddr_in *addr = (struct sockaddr_in *) addr6;
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, " - call socket_bind");
	
	rc = socket_bind(&client->sockets, &gsock, SOCKET_GET_SOCKET_ID(call),
	    addr6, addr_len, UDP_FREE_PORTS_START, UDP_FREE_PORTS_END,
	    last_used_port);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		goto out;
	}
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, " - call socket_cores_find");
	
	socket_core_t *sock_core = socket_cores_find(&client->sockets,
	    SOCKET_GET_SOCKET_ID(call));
	if (sock_core == NULL) {
		async_answer_0(callid, ENOENT);
		goto out;
	}
	
	udp_sockdata_t *socket =
	    (udp_sockdata_t *) sock_core->specific_data;
	
	udp_sock_t fsocket;
	
	fsocket.port = sock_core->port;
	
	switch (addr->sin_family) {
	case AF_INET:
		inet_sockaddr_in_addr(addr, &fsocket.addr);
		break;
	case AF_INET6:
		inet_sockaddr_in6_addr(addr6, &fsocket.addr);
		break;
	default:
		async_answer_0(callid, EINVAL);
		goto out;
	}
	
	udp_error_t urc = udp_uc_set_local(socket->assoc, &fsocket);
	
	switch (urc) {
	case UDP_EOK:
		rc = EOK;
		break;
/*	case TCP_ENOTEXIST:
		rc = ENOTCONN;
		break;
	case TCP_ECLOSING:
		rc = ENOTCONN;
		break;
	case TCP_ERESET:
		rc = ECONNABORTED;
		break;*/
	default:
		assert(false);
	}
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, " - success");
	async_answer_0(callid, rc);
	
out:
	if (addr6 != NULL)
		free(addr6);
}

static void udp_sock_listen(udp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_sock_listen()");
	async_answer_0(callid, ENOTSUP);
}

static void udp_sock_connect(udp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_sock_connect()");
	async_answer_0(callid, ENOTSUP);
}

static void udp_sock_accept(udp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_sock_accept()");
	async_answer_0(callid, ENOTSUP);
}

static void udp_sock_sendto(udp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_sock_send()");
	
	uint8_t *buffer = calloc(UDP_FRAGMENT_SIZE, 1);
	if (buffer == NULL) {
		async_answer_0(callid, ENOMEM);
		return;
	}
	
	struct sockaddr_in6 *addr6 = NULL;
	struct sockaddr_in *addr;
	udp_sock_t fsocket;
	udp_sock_t *fsocket_ptr;
	
	if (IPC_GET_IMETHOD(call) == NET_SOCKET_SENDTO) {
		size_t addr_len;
		int rc = async_data_write_accept((void **) &addr6, false,
		    0, 0, 0, &addr_len);
		if (rc != EOK) {
			async_answer_0(callid, rc);
			goto out;
		}
		
		if ((addr_len != sizeof(struct sockaddr_in)) &&
		    (addr_len != sizeof(struct sockaddr_in6))) {
			async_answer_0(callid, EINVAL);
			goto out;
		}
		
		addr = (struct sockaddr_in *) addr6;
		
		switch (addr->sin_family) {
		case AF_INET:
			inet_sockaddr_in_addr(addr, &fsocket.addr);
			break;
		case AF_INET6:
			inet_sockaddr_in6_addr(addr6, &fsocket.addr);
			break;
		default:
			async_answer_0(callid, EINVAL);
			goto out;
		}
		
		fsocket.port = uint16_t_be2host(addr->sin_port);
		fsocket_ptr = &fsocket;
	} else
		fsocket_ptr = NULL;
	
	int socket_id = SOCKET_GET_SOCKET_ID(call);
	
	SOCKET_GET_FLAGS(call);
	
	socket_core_t *sock_core =
	    socket_cores_find(&client->sockets, socket_id);
	if (sock_core == NULL) {
		async_answer_0(callid, ENOTSOCK);
		goto out;
	}
	
	udp_sockdata_t *socket =
	    (udp_sockdata_t *) sock_core->specific_data;
	
	if (sock_core->port <= 0) {
		/* Implicitly bind socket to port */
		int rc = socket_bind_free_port(&gsock, sock_core,
		    UDP_FREE_PORTS_START, UDP_FREE_PORTS_END, last_used_port);
		if (rc != EOK) {
			async_answer_0(callid, rc);
			goto out;
		}
		
		assert(sock_core->port > 0);
		
		udp_error_t urc = udp_uc_set_local_port(socket->assoc,
		    sock_core->port);
		
		if (urc != UDP_EOK) {
			// TODO: better error handling
			async_answer_0(callid, EINTR);
			goto out;
		}
		
		last_used_port = sock_core->port;
	}
	
	fibril_mutex_lock(&socket->lock);
	
	if (inet_addr_is_any(&socket->assoc->ident.local.addr) &&
		socket->assoc->ident.iplink == 0) {
		/* Determine local IP address */
		inet_addr_t loc_addr;
		inet_addr_t rem_addr;
		
		rem_addr = fsocket_ptr ? fsocket.addr :
		    socket->assoc->ident.foreign.addr;
		
		int rc = inet_get_srcaddr(&rem_addr, 0, &loc_addr);
		if (rc != EOK) {
			fibril_mutex_unlock(&socket->lock);
			async_answer_0(callid, rc);
			log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_sock_sendto: Failed to "
			    "determine local address.");
			goto out;
		}
		
		socket->assoc->ident.local.addr = loc_addr;
	}
	
	assert(socket->assoc != NULL);
	
	int fragments = SOCKET_GET_DATA_FRAGMENTS(call);
	for (int index = 0; index < fragments; index++) {
		ipc_callid_t wcallid;
		size_t length;
		
		if (!async_data_write_receive(&wcallid, &length)) {
			fibril_mutex_unlock(&socket->lock);
			async_answer_0(callid, EINVAL);
			goto out;
		}
		
		if (length > UDP_FRAGMENT_SIZE)
			length = UDP_FRAGMENT_SIZE;
		
		int rc = async_data_write_finalize(wcallid, buffer, length);
		if (rc != EOK) {
			fibril_mutex_unlock(&socket->lock);
			async_answer_0(callid, rc);
			goto out;
		}
		
		udp_error_t urc =
		    udp_uc_send(socket->assoc, fsocket_ptr, buffer, length, 0);
		
		switch (urc) {
		case UDP_EOK:
			rc = EOK;
			break;
		case UDP_ENORES:
			rc = ENOMEM;
			break;
		case UDP_EUNSPEC:
			rc = EINVAL;
			break;
		case UDP_ENOROUTE:
			rc = EIO;
			break;
		default:
			assert(false);
		}
		
		if (rc != EOK) {
			fibril_mutex_unlock(&socket->lock);
			async_answer_0(callid, rc);
			goto out;
		}
	}
	
	ipc_call_t answer;
	
	IPC_SET_ARG1(answer, 0);
	SOCKET_SET_DATA_FRAGMENT_SIZE(answer, UDP_FRAGMENT_SIZE);
	async_answer_2(callid, EOK, IPC_GET_ARG1(answer),
	    IPC_GET_ARG2(answer));
	fibril_mutex_unlock(&socket->lock);
	
out:
	if (addr6 != NULL)
		free(addr6);
	
	free(buffer);
}

static void udp_sock_recvfrom(udp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "%p: udp_sock_recv[from]()", client);
	
	int socket_id = SOCKET_GET_SOCKET_ID(call);
	
	socket_core_t *sock_core =
	    socket_cores_find(&client->sockets, socket_id);
	if (sock_core == NULL) {
		async_answer_0(callid, ENOTSOCK);
		return;
	}
	
	udp_sockdata_t *socket =
	    (udp_sockdata_t *) sock_core->specific_data;
	
	fibril_mutex_lock(&socket->lock);
	
	if (socket->assoc == NULL) {
		fibril_mutex_unlock(&socket->lock);
		async_answer_0(callid, ENOTCONN);
		return;
	}
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_sock_recvfrom(): lock recv_buffer lock");
	
	fibril_mutex_lock(&socket->recv_buffer_lock);
	
	while ((socket->recv_buffer_used == 0) &&
	    (socket->recv_error == UDP_EOK)) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_sock_recvfrom(): wait for cv");
		fibril_condvar_wait(&socket->recv_buffer_cv,
		    &socket->recv_buffer_lock);
	}
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, "Got data in sock recv_buffer");
	
	size_t data_len = socket->recv_buffer_used;
	udp_error_t urc = socket->recv_error;
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, "**** recv data_len=%zu", data_len);
	
	int rc;
	
	switch (urc) {
	case UDP_EOK:
		rc = EOK;
		break;
/*	case TCP_ENOTEXIST:
	case TCP_ECLOSING:
		rc = ENOTCONN;
		break;
	case TCP_ERESET:
		rc = ECONNABORTED;
		break;*/
	default:
		assert(false);
	}
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, "**** udp_uc_receive -> %d", rc);
	
	if (rc != EOK) {
		fibril_mutex_unlock(&socket->recv_buffer_lock);
		fibril_mutex_unlock(&socket->lock);
		async_answer_0(callid, rc);
		return;
	}
	
	ipc_callid_t rcallid;
	size_t addr_size = 0;
	
	if (IPC_GET_IMETHOD(call) == NET_SOCKET_RECVFROM) {
		/* Fill address */
		udp_sock_t *rsock = &socket->recv_fsock;
		struct sockaddr_in addr;
		struct sockaddr_in6 addr6;
		size_t addr_length;
		
		uint16_t addr_af = inet_addr_sockaddr_in(&rsock->addr, &addr,
		    &addr6);
		
		switch (addr_af) {
		case AF_INET:
			addr.sin_port = host2uint16_t_be(rsock->port);
			
			log_msg(LOG_DEFAULT, LVL_DEBUG, "addr read receive");
			if (!async_data_read_receive(&rcallid, &addr_length)) {
				fibril_mutex_unlock(&socket->recv_buffer_lock);
				fibril_mutex_unlock(&socket->lock);
				async_answer_0(callid, EINVAL);
				return;
			}
			
			if (addr_length > sizeof(addr))
				addr_length = sizeof(addr);
			
			addr_size = sizeof(addr);
			
			log_msg(LOG_DEFAULT, LVL_DEBUG, "addr read finalize");
			rc = async_data_read_finalize(rcallid, &addr, addr_length);
			if (rc != EOK) {
				fibril_mutex_unlock(&socket->recv_buffer_lock);
				fibril_mutex_unlock(&socket->lock);
				async_answer_0(callid, EINVAL);
				return;
			}
			
			break;
		case AF_INET6:
			addr6.sin6_port = host2uint16_t_be(rsock->port);
			
			log_msg(LOG_DEFAULT, LVL_DEBUG, "addr6 read receive");
			if (!async_data_read_receive(&rcallid, &addr_length)) {
				fibril_mutex_unlock(&socket->recv_buffer_lock);
				fibril_mutex_unlock(&socket->lock);
				async_answer_0(callid, EINVAL);
				return;
			}
			
			if (addr_length > sizeof(addr6))
				addr_length = sizeof(addr6);
			
			addr_size = sizeof(addr6);
			
			log_msg(LOG_DEFAULT, LVL_DEBUG, "addr6 read finalize");
			rc = async_data_read_finalize(rcallid, &addr6, addr_length);
			if (rc != EOK) {
				fibril_mutex_unlock(&socket->recv_buffer_lock);
				fibril_mutex_unlock(&socket->lock);
				async_answer_0(callid, EINVAL);
				return;
			}
			
			break;
		default:
			fibril_mutex_unlock(&socket->recv_buffer_lock);
			fibril_mutex_unlock(&socket->lock);
			async_answer_0(callid, EINVAL);
			return;
		}
	}
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, "data read receive");
	
	size_t length;
	if (!async_data_read_receive(&rcallid, &length)) {
		fibril_mutex_unlock(&socket->recv_buffer_lock);
		fibril_mutex_unlock(&socket->lock);
		async_answer_0(callid, EINVAL);
		return;
	}
	
	if (length > data_len)
		length = data_len;
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, "data read finalize");
	
	rc = async_data_read_finalize(rcallid, socket->recv_buffer, length);
	
	if ((length < data_len) && (rc == EOK))
		rc = EOVERFLOW;
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, "read_data_length <- %zu", length);
	
	ipc_call_t answer;
	
	IPC_SET_ARG2(answer, 0);
	SOCKET_SET_READ_DATA_LENGTH(answer, length);
	SOCKET_SET_ADDRESS_LENGTH(answer, addr_size);
	async_answer_3(callid, EOK, IPC_GET_ARG1(answer),
	    IPC_GET_ARG2(answer), IPC_GET_ARG3(answer));
	
	socket->recv_buffer_used = 0;
	
	fibril_condvar_broadcast(&socket->recv_buffer_cv);
	fibril_mutex_unlock(&socket->recv_buffer_lock);
	fibril_mutex_unlock(&socket->lock);
}

static void udp_sock_close(udp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_sock_close()");
	int socket_id = SOCKET_GET_SOCKET_ID(call);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_sock_close() - find core");
	socket_core_t *sock_core =
	    socket_cores_find(&client->sockets, socket_id);
	if (sock_core == NULL) {
	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_sock_close() - core not found");
		async_answer_0(callid, ENOTSOCK);
		return;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_sock_close() - spec data");
	udp_sockdata_t *socket =
	    (udp_sockdata_t *) sock_core->specific_data;
	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_sock_close() - lock socket");
	fibril_mutex_lock(&socket->lock);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_sock_close() - lock socket buffer");
	fibril_mutex_lock(&socket->recv_buffer_lock);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_sock_close - set socket->sock_core = NULL");
	socket->sock_core = NULL;
	fibril_mutex_unlock(&socket->recv_buffer_lock);

	udp_uc_reset(socket->assoc);

	int rc = socket_destroy(NULL, socket_id, &client->sockets, &gsock,
	    udp_free_sock_data);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_sock_close - socket_destroy failed");
		fibril_mutex_unlock(&socket->lock);
		async_answer_0(callid, rc);
		return;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_sock_close - broadcast recv_buffer_cv");
	fibril_condvar_broadcast(&socket->recv_buffer_cv);

	fibril_mutex_unlock(&socket->lock);
	async_answer_0(callid, EOK);
}

static void udp_sock_getsockopt(udp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_sock_getsockopt()");
	async_answer_0(callid, ENOTSUP);
}

static void udp_sock_setsockopt(udp_client_t *client, ipc_callid_t callid, ipc_call_t call)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_sock_setsockopt)");
	log_msg(LOG_DEFAULT, LVL_DEBUG, " - async_data_write_accept");
	
	void *data = NULL;
	size_t data_len;
	int rc = async_data_write_accept(&data, false, 0, 0, 0, &data_len);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, " - failed accepting data");
		async_answer_0(callid, rc);
		return;
	}
	
	sysarg_t opt_level = SOL_SOCKET;
	sysarg_t opt_name = SOCKET_GET_OPT_NAME(call);
	
	if (opt_level != SOL_SOCKET || opt_name != SO_IPLINK ||
	    data_len != sizeof(service_id_t)) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, " - failed opt_level/name/len");
		log_msg(LOG_DEFAULT, LVL_DEBUG, " - failed opt_level=%d, "
		    "opt_name=%d, data_len=%zu", (int)opt_level, (int)opt_name,
		    data_len);
		async_answer_0(callid, EINVAL);
		return;
	}
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, " - call socket_cores_find");
	
	socket_core_t *sock_core = socket_cores_find(&client->sockets,
	    SOCKET_GET_SOCKET_ID(call));
	if (sock_core == NULL) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, " - failed getting sock_core");
		async_answer_0(callid, ENOENT);
		return;
	}
	
	udp_sockdata_t *socket =
	    (udp_sockdata_t *) sock_core->specific_data;
	
	service_id_t iplink = *(service_id_t *)data;
	udp_uc_set_iplink(socket->assoc, iplink);
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, " - success");
	async_answer_0(callid, EOK);
}


static int udp_sock_recv_fibril(void *arg)
{
	udp_sockdata_t *sock = (udp_sockdata_t *)arg;
	udp_error_t urc;
	xflags_t xflags;
	size_t rcvd;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_sock_recv_fibril()");

	fibril_mutex_lock(&sock->recv_buffer_lock);

	while (true) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "[] wait for rcv buffer empty()");
		while ((sock->recv_buffer_used != 0) && (sock->sock_core != NULL)) {
			fibril_condvar_wait(&sock->recv_buffer_cv,
			    &sock->recv_buffer_lock);
		}

		fibril_mutex_unlock(&sock->recv_buffer_lock);

		log_msg(LOG_DEFAULT, LVL_DEBUG, "[] call udp_uc_receive()");
		urc = udp_uc_receive(sock->assoc, sock->recv_buffer,
		    UDP_FRAGMENT_SIZE, &rcvd, &xflags, &sock->recv_fsock);
		fibril_mutex_lock(&sock->recv_buffer_lock);
		sock->recv_error = urc;

		log_msg(LOG_DEFAULT, LVL_DEBUG, "[] udp_uc_receive -> %d", urc);

		if (sock->sock_core != NULL)
			udp_sock_notify_data(sock->sock_core);

		if (urc != UDP_EOK) {
			log_msg(LOG_DEFAULT, LVL_DEBUG, "[] urc != UDP_EOK, break");
			fibril_condvar_broadcast(&sock->recv_buffer_cv);
			fibril_mutex_unlock(&sock->recv_buffer_lock);
			break;
		}

		log_msg(LOG_DEFAULT, LVL_DEBUG, "[] got data - broadcast recv_buffer_cv");

		sock->recv_buffer_used = rcvd;
		fibril_condvar_broadcast(&sock->recv_buffer_cv);
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_sock_recv_fibril() exited loop");
	udp_uc_destroy(sock->assoc);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_sock_recv_fibril() terminated");

	return 0;
}

static void udp_sock_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	ipc_callid_t callid;
	ipc_call_t call;
	udp_client_t client;

	/* Accept the connection */
	async_answer_0(iid, EOK);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_sock_connection: begin");

	client.sess = async_callback_receive(EXCHANGE_SERIALIZE);
	socket_cores_initialize(&client.sockets);

	while (true) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_sock_connection: wait");
		callid = async_get_call(&call);
		if (!IPC_GET_IMETHOD(call))
			break;

		log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_sock_connection: METHOD=%d",
		    (int)IPC_GET_IMETHOD(call));

		switch (IPC_GET_IMETHOD(call)) {
		case NET_SOCKET:
			udp_sock_socket(&client, callid, call);
			break;
		case NET_SOCKET_BIND:
			udp_sock_bind(&client, callid, call);
			break;
		case NET_SOCKET_LISTEN:
			udp_sock_listen(&client, callid, call);
			break;
		case NET_SOCKET_CONNECT:
			udp_sock_connect(&client, callid, call);
			break;
		case NET_SOCKET_ACCEPT:
			udp_sock_accept(&client, callid, call);
			break;
		case NET_SOCKET_SEND:
		case NET_SOCKET_SENDTO:
			udp_sock_sendto(&client, callid, call);
			break;
		case NET_SOCKET_RECV:
		case NET_SOCKET_RECVFROM:
			udp_sock_recvfrom(&client, callid, call);
			break;
		case NET_SOCKET_CLOSE:
			udp_sock_close(&client, callid, call);
			break;
		case NET_SOCKET_GETSOCKOPT:
			udp_sock_getsockopt(&client, callid, call);
			break;
		case NET_SOCKET_SETSOCKOPT:
			udp_sock_setsockopt(&client, callid, call);
			break;
		default:
			async_answer_0(callid, ENOTSUP);
			break;
		}
	}

	/* Clean up */
	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_sock_connection: Clean up");
	async_hangup(client.sess);
	socket_cores_release(NULL, &client.sockets, &gsock, udp_free_sock_data);
}

/**
 * @}
 */
