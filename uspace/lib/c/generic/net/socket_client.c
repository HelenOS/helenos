/*
 * Copyright (c) 2009 Lukas Mejdrech
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
 *  @{
 */

/** @file
 * Socket application program interface (API) implementation.
 * @see socket.h for more information.
 * This is a part of the network application library.
 */

#include <assert.h>
#include <async.h>
#include <fibril_synch.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <task.h>
#include <ns.h>
#include <ipc/services.h>
#include <ipc/socket.h>
#include <net/in.h>
#include <net/socket.h>
#include <adt/dynamic_fifo.h>
#include <adt/int_map.h>

/** Initial received packet queue size. */
#define SOCKET_INITIAL_RECEIVED_SIZE	4

/** Maximum received packet queue size. */
#define SOCKET_MAX_RECEIVED_SIZE	0

/** Initial waiting sockets queue size. */
#define SOCKET_INITIAL_ACCEPTED_SIZE	1

/** Maximum waiting sockets queue size. */
#define SOCKET_MAX_ACCEPTED_SIZE	0

/**
 * Maximum number of random attempts to find a new socket identifier before
 * switching to the sequence.
 */
#define SOCKET_ID_TRIES			100

/** Type definition of the socket specific data.
 * @see socket
 */
typedef struct socket socket_t;

/** Socket specific data.
 *
 * Each socket lock locks only its structure part and any number of them may be
 * locked simultaneously.
 */
struct socket {
	/** Socket identifier. */
	int socket_id;
	/** Parent module session. */
	async_sess_t *sess;
	/** Parent module service. */
	services_t service;
	/** Socket family */
	int family;

	/**
	 * Underlying protocol header size.
	 * Sending and receiving optimalization.
	 */
	size_t header_size;

	/** Packet data fragment size. Sending optimization. */
	size_t data_fragment_size;

	/**
	 * Sending safety lock.
	 * Locks the header_size and data_fragment_size attributes.
	 */
	fibril_rwlock_t sending_lock;

	/** Received packets queue. */
	dyn_fifo_t received;

	/**
	 * Received packets safety lock.
	 * Used for receiving and receive notifications.
	 * Locks the received attribute.
	 */
	fibril_mutex_t receive_lock;

	/** Received packets signaling. Signaled upon receive notification. */
	fibril_condvar_t receive_signal;
	/** Waiting sockets queue. */
	dyn_fifo_t accepted;

	/**
	 * Waiting sockets safety lock.
	 * Used for accepting and accept notifications.
	 * Locks the accepted attribute.
	 */
	fibril_mutex_t accept_lock;

	/** Waiting sockets signaling. Signaled upon accept notification. */
	fibril_condvar_t accept_signal;

	/**
	 * The number of blocked functions called.
	 * Used while waiting for the received packets or accepted sockets.
	 */
	int blocked;
};

/** Sockets map.
 * Maps socket identifiers to the socket specific data.
 * @see int_map.h
 */
INT_MAP_DECLARE(sockets, socket_t);

/** Socket client library global data. */
static struct socket_client_globals {
	/** TCP module session. */
	async_sess_t *tcp_sess;
	/** UDP module session. */
	async_sess_t *udp_sess;

	/** Active sockets. */
	sockets_t *sockets;

	/** Safety lock.
	 * Write lock is used only for adding or removing sockets.
	 * When locked for writing, no other socket locks need to be locked.
	 * When locked for reading, any other socket locks may be locked.
	 * No socket lock may be locked if this lock is unlocked.
	 */
	fibril_rwlock_t lock;
} socket_globals = {
	.tcp_sess = NULL,
	.udp_sess = NULL,
	.sockets = NULL,
	.lock = FIBRIL_RWLOCK_INITIALIZER(socket_globals.lock)
};

INT_MAP_IMPLEMENT(sockets, socket_t);

/** Returns the active sockets.
 *
 *  @return		The active sockets.
 */
static sockets_t *socket_get_sockets(void)
{
	if (!socket_globals.sockets) {
		socket_globals.sockets =
		    (sockets_t *) malloc(sizeof(sockets_t));
		if (!socket_globals.sockets)
			return NULL;

		if (sockets_initialize(socket_globals.sockets) != EOK) {
			free(socket_globals.sockets);
			socket_globals.sockets = NULL;
		}

		srand(task_get_id());
	}

	return socket_globals.sockets;
}

/** Default thread for new connections.
 *
 * @param[in] iid   The initial message identifier.
 * @param[in] icall The initial message call structure.
 * @param[in] arg   Local argument.
 *
 */
static void socket_connection(ipc_callid_t iid, ipc_call_t * icall, void *arg)
{
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call)) {
			async_answer_0(callid, 0);
			return;
		}
		
		int rc;
		
		switch (IPC_GET_IMETHOD(call)) {
		case NET_SOCKET_RECEIVED:
		case NET_SOCKET_ACCEPTED:
		case NET_SOCKET_DATA_FRAGMENT_SIZE:
			fibril_rwlock_read_lock(&socket_globals.lock);
			
			/* Find the socket */
			socket_t *socket = sockets_find(socket_get_sockets(),
			    SOCKET_GET_SOCKET_ID(call));
			if (!socket) {
				rc = ENOTSOCK;
				fibril_rwlock_read_unlock(&socket_globals.lock);
				break;
			}
			
			switch (IPC_GET_IMETHOD(call)) {
			case NET_SOCKET_RECEIVED:
				fibril_mutex_lock(&socket->receive_lock);
				/* Push the number of received packet fragments */
				rc = dyn_fifo_push(&socket->received,
				    SOCKET_GET_DATA_FRAGMENTS(call),
				    SOCKET_MAX_RECEIVED_SIZE);
				if (rc == EOK) {
					/* Signal the received packet */
					fibril_condvar_signal(&socket->receive_signal);
				}
				fibril_mutex_unlock(&socket->receive_lock);
				break;
				
			case NET_SOCKET_ACCEPTED:
				/* Push the new socket identifier */
				fibril_mutex_lock(&socket->accept_lock);
				rc = dyn_fifo_push(&socket->accepted, 1,
				    SOCKET_MAX_ACCEPTED_SIZE);
				if (rc == EOK) {
					/* Signal the accepted socket */
					fibril_condvar_signal(&socket->accept_signal);
				}
				fibril_mutex_unlock(&socket->accept_lock);
				break;
			
			default:
				rc = ENOTSUP;
			}
			
			if ((SOCKET_GET_DATA_FRAGMENT_SIZE(call) > 0) &&
			    (SOCKET_GET_DATA_FRAGMENT_SIZE(call) !=
			    socket->data_fragment_size)) {
				fibril_rwlock_write_lock(&socket->sending_lock);
				
				/* Set the data fragment size */
				socket->data_fragment_size =
				    SOCKET_GET_DATA_FRAGMENT_SIZE(call);
				
				fibril_rwlock_write_unlock(&socket->sending_lock);
			}
			
			fibril_rwlock_read_unlock(&socket_globals.lock);
			break;
			
		default:
			rc = ENOTSUP;
		}
		
		async_answer_0(callid, (sysarg_t) rc);
	}
}

/** Return the TCP module session.
 *
 * Connect to the TCP module if necessary.
 *
 * @return The TCP module session.
 *
 */
static async_sess_t *socket_get_tcp_sess(void)
{
	if (socket_globals.tcp_sess == NULL)
		socket_globals.tcp_sess = service_bind(SERVICE_TCP,
		    0, 0, SERVICE_TCP, socket_connection);
	
	return socket_globals.tcp_sess;
}

/** Return the UDP module session.
 *
 * Connect to the UDP module if necessary.
 *
 * @return The UDP module session.
 *
 */
static async_sess_t *socket_get_udp_sess(void)
{
	if (socket_globals.udp_sess == NULL)
		socket_globals.udp_sess = service_bind(SERVICE_UDP,
		    0, 0, SERVICE_UDP, socket_connection);
	
	return socket_globals.udp_sess;
}

/** Tries to find a new free socket identifier.
 *
 * @return		The new socket identifier.
 * @return		ELIMIT if there is no socket identifier available.
 */
static int socket_generate_new_id(void)
{
	sockets_t *sockets;
	int socket_id = 0;
	int count;

	sockets = socket_get_sockets();
	count = 0;

	do {
		if (count < SOCKET_ID_TRIES) {
			socket_id = rand() % INT_MAX;
			++count;
		} else if (count == SOCKET_ID_TRIES) {
			socket_id = 1;
			++count;
		/* Only this branch for last_id */
		} else {
			if (socket_id < INT_MAX) {
				++socket_id;
			} else {
				return ELIMIT;
			}
		}
	} while (sockets_find(sockets, socket_id));
	
	return socket_id;
}

/** Initializes a new socket specific data.
 *
 * @param[in,out] socket The socket to be initialized.
 * @param[in] socket_id  The new socket identifier.
 * @param[in] sess       The parent module session.
 * @param[in] service    The parent module service.
 */
static void socket_initialize(socket_t *socket, int socket_id,
    async_sess_t *sess, services_t service)
{
	socket->socket_id = socket_id;
	socket->sess = sess;
	socket->service = service;
	dyn_fifo_initialize(&socket->received, SOCKET_INITIAL_RECEIVED_SIZE);
	dyn_fifo_initialize(&socket->accepted, SOCKET_INITIAL_ACCEPTED_SIZE);
	fibril_mutex_initialize(&socket->receive_lock);
	fibril_condvar_initialize(&socket->receive_signal);
	fibril_mutex_initialize(&socket->accept_lock);
	fibril_condvar_initialize(&socket->accept_signal);
	fibril_rwlock_initialize(&socket->sending_lock);
}

/** Creates a new socket.
 *
 * @param[in] domain	The socket protocol family.
 * @param[in] type	Socket type.
 * @param[in] protocol	Socket protocol.
 * @return		The socket identifier on success.
 * @return		EPFNOTSUPPORT if the protocol family is not supported.
 * @return		ESOCKNOTSUPPORT if the socket type is not supported.
 * @return		EPROTONOSUPPORT if the protocol is not supported.
 * @return		ENOMEM if there is not enough memory left.
 * @return		ELIMIT if there was not a free socket identifier found
 *			this time.
 * @return		Other error codes as defined for the NET_SOCKET message.
 * @return		Other error codes as defined for the
 *			service_bind() function.
 */
int socket(int domain, int type, int protocol)
{
	socket_t *socket;
	async_sess_t *sess;
	int socket_id;
	services_t service;
	sysarg_t fragment_size;
	sysarg_t header_size;
	int rc;

	/* Find the appropriate service */
	switch (domain) {
	case PF_INET:
	case PF_INET6:
		switch (type) {
		case SOCK_STREAM:
			if (!protocol)
				protocol = IPPROTO_TCP;

			switch (protocol) {
			case IPPROTO_TCP:
				sess = socket_get_tcp_sess();
				service = SERVICE_TCP;
				break;
			default:
				return EPROTONOSUPPORT;
			}

			break;

		case SOCK_DGRAM:
			if (!protocol)
				protocol = IPPROTO_UDP;

			switch (protocol) {
			case IPPROTO_UDP:
				sess = socket_get_udp_sess();
				service = SERVICE_UDP;
				break;
			default:
				return EPROTONOSUPPORT;
			}

			break;

		case SOCK_RAW:
		default:
			return ESOCKTNOSUPPORT;
		}

		break;

	default:
		return EPFNOSUPPORT;
	}

	if (sess == NULL)
		return ENOENT;

	/* Create a new socket structure */
	socket = (socket_t *) malloc(sizeof(socket_t));
	if (!socket)
		return ENOMEM;

	memset(socket, 0, sizeof(*socket));
	socket->family = domain;
	fibril_rwlock_write_lock(&socket_globals.lock);

	/* Request a new socket */
	socket_id = socket_generate_new_id();
	if (socket_id <= 0) {
		fibril_rwlock_write_unlock(&socket_globals.lock);
		free(socket);
		return socket_id;
	}
	
	async_exch_t *exch = async_exchange_begin(sess);
	rc = (int) async_req_3_3(exch, NET_SOCKET, socket_id, 0, service, NULL,
	    &fragment_size, &header_size);
	async_exchange_end(exch);
	
	if (rc != EOK) {
		fibril_rwlock_write_unlock(&socket_globals.lock);
		free(socket);
		return rc;
	}

	socket->data_fragment_size = (size_t) fragment_size;
	socket->header_size = (size_t) header_size;

	/* Finish the new socket initialization */
	socket_initialize(socket, socket_id, sess, service);
	/* Store the new socket */
	rc = sockets_add(socket_get_sockets(), socket_id, socket);

	fibril_rwlock_write_unlock(&socket_globals.lock);
	if (rc < 0) {
		dyn_fifo_destroy(&socket->received);
		dyn_fifo_destroy(&socket->accepted);
		free(socket);
		
		exch = async_exchange_begin(sess);
		async_msg_3(exch, NET_SOCKET_CLOSE, (sysarg_t) socket_id, 0,
		    service);
		async_exchange_end(exch);
		
		return rc;
	}

	return socket_id;
}

/** Sends message to the socket parent module with specified data.
 *
 * @param[in] socket_id	Socket identifier.
 * @param[in] message	The action message.
 * @param[in] arg2	The second message parameter.
 * @param[in] data	The data to be sent.
 * @param[in] datalength The data length.
 * @return		EOK on success.
 * @return		ENOTSOCK if the socket is not found.
 * @return		EBADMEM if the data parameter is NULL.
 * @return		NO_DATA if the datalength parameter is zero (0).
 * @return		Other error codes as defined for the spcific message.
 */
static int
socket_send_data(int socket_id, sysarg_t message, sysarg_t arg2,
    const void *data, size_t datalength)
{
	socket_t *socket;
	aid_t message_id;
	sysarg_t result;

	if (!data)
		return EBADMEM;

	if (!datalength)
		return NO_DATA;

	fibril_rwlock_read_lock(&socket_globals.lock);

	/* Find the socket */
	socket = sockets_find(socket_get_sockets(), socket_id);
	if (!socket) {
		fibril_rwlock_read_unlock(&socket_globals.lock);
		return ENOTSOCK;
	}

	/* Request the message */
	async_exch_t *exch = async_exchange_begin(socket->sess);
	message_id = async_send_3(exch, message,
	    (sysarg_t) socket->socket_id, arg2, socket->service, NULL);
	/* Send the address */
	async_data_write_start(exch, data, datalength);
	async_exchange_end(exch);

	fibril_rwlock_read_unlock(&socket_globals.lock);
	async_wait_for(message_id, &result);
	return (int) result;
}

/** Binds the socket to a port address.
 *
 * @param[in] socket_id	Socket identifier.
 * @param[in] my_addr	The port address.
 * @param[in] addrlen	The address length.
 * @return		EOK on success.
 * @return		ENOTSOCK if the socket is not found.
 * @return		EBADMEM if the my_addr parameter is NULL.
 * @return		NO_DATA if the addlen parameter is zero.
 * @return		Other error codes as defined for the NET_SOCKET_BIND
 *			message.
 */
int bind(int socket_id, const struct sockaddr * my_addr, socklen_t addrlen)
{
	if (addrlen <= 0)
		return EINVAL;

	/* Send the address */
	return socket_send_data(socket_id, NET_SOCKET_BIND, 0, my_addr,
	    (size_t) addrlen);
}

/** Sets the number of connections waiting to be accepted.
 *
 * @param[in] socket_id	Socket identifier.
 * @param[in] backlog	The maximum number of waiting sockets to be accepted.
 * @return		EOK on success.
 * @return		EINVAL if the backlog parameter is not positive (<=0).
 * @return		ENOTSOCK if the socket is not found.
 * @return		Other error codes as defined for the NET_SOCKET_LISTEN
 *			message.
 */
int listen(int socket_id, int backlog)
{
	socket_t *socket;
	int result;

	if (backlog <= 0)
		return EINVAL;

	fibril_rwlock_read_lock(&socket_globals.lock);

	/* Find the socket */
	socket = sockets_find(socket_get_sockets(), socket_id);
	if (!socket) {
		fibril_rwlock_read_unlock(&socket_globals.lock);
		return ENOTSOCK;
	}

	/* Request listen backlog change */
	async_exch_t *exch = async_exchange_begin(socket->sess);
	result = (int) async_req_3_0(exch, NET_SOCKET_LISTEN,
	    (sysarg_t) socket->socket_id, (sysarg_t) backlog, socket->service);
	async_exchange_end(exch);

	fibril_rwlock_read_unlock(&socket_globals.lock);
	return result;
}

/** Accepts waiting socket.
 *
 * Blocks until such a socket exists.
 *
 * @param[in] socket_id	Socket identifier.
 * @param[out] cliaddr	The remote client address.
 * @param[in] addrlen	The address length.
 * @return		EOK on success.
 * @return		EBADMEM if the cliaddr or addrlen parameter is NULL.
 * @return		EINVAL if the backlog parameter is not positive (<=0).
 * @return		ENOTSOCK if the socket is not found.
 * @return		Other error codes as defined for the NET_SOCKET_ACCEPT
 *			message.
 */
int accept(int socket_id, struct sockaddr * cliaddr, socklen_t * addrlen)
{
	socket_t *socket;
	socket_t *new_socket;
	aid_t message_id;
	sysarg_t ipc_result;
	int result;
	ipc_call_t answer;

	if (!cliaddr || !addrlen)
		return EBADMEM;

	fibril_rwlock_write_lock(&socket_globals.lock);

	/* Find the socket */
	socket = sockets_find(socket_get_sockets(), socket_id);
	if (!socket) {
		fibril_rwlock_write_unlock(&socket_globals.lock);
		return ENOTSOCK;
	}

	fibril_mutex_lock(&socket->accept_lock);

	/* Wait for an accepted socket */
	++ socket->blocked;
	while (dyn_fifo_value(&socket->accepted) <= 0) {
		fibril_rwlock_write_unlock(&socket_globals.lock);
		fibril_condvar_wait(&socket->accept_signal, &socket->accept_lock);
		/* Drop the accept lock to avoid deadlock */
		fibril_mutex_unlock(&socket->accept_lock);
		fibril_rwlock_write_lock(&socket_globals.lock);
		fibril_mutex_lock(&socket->accept_lock);
	}
	-- socket->blocked;

	/* Create a new socket */
	new_socket = (socket_t *) malloc(sizeof(socket_t));
	if (!new_socket) {
		fibril_mutex_unlock(&socket->accept_lock);
		fibril_rwlock_write_unlock(&socket_globals.lock);
		return ENOMEM;
	}
	memset(new_socket, 0, sizeof(*new_socket));
	socket_id = socket_generate_new_id();
	if (socket_id <= 0) {
		fibril_mutex_unlock(&socket->accept_lock);
		fibril_rwlock_write_unlock(&socket_globals.lock);
		free(new_socket);
		return socket_id;
	}
	socket_initialize(new_socket, socket_id, socket->sess,
	    socket->service);
	result = sockets_add(socket_get_sockets(), new_socket->socket_id,
	    new_socket);
	if (result < 0) {
		fibril_mutex_unlock(&socket->accept_lock);
		fibril_rwlock_write_unlock(&socket_globals.lock);
		free(new_socket);
		return result;
	}

	/* Request accept */
	async_exch_t *exch = async_exchange_begin(socket->sess);
	message_id = async_send_5(exch, NET_SOCKET_ACCEPT,
	    (sysarg_t) socket->socket_id, 0, socket->service, 0,
	    new_socket->socket_id, &answer);

	/* Read address */
	async_data_read_start(exch, cliaddr, *addrlen);
	async_exchange_end(exch);
	
	fibril_rwlock_write_unlock(&socket_globals.lock);
	async_wait_for(message_id, &ipc_result);
	result = (int) ipc_result;
	if (result > 0) {
		if (result != socket_id)
			result = EINVAL;

		/* Dequeue the accepted socket if successful */
		dyn_fifo_pop(&socket->accepted);
		/* Set address length */
		*addrlen = SOCKET_GET_ADDRESS_LENGTH(answer);
		new_socket->data_fragment_size =
		    SOCKET_GET_DATA_FRAGMENT_SIZE(answer);
	} else if (result == ENOTSOCK) {
		/* Empty the queue if no accepted sockets */
		while (dyn_fifo_pop(&socket->accepted) > 0)
			;
	}

	fibril_mutex_unlock(&socket->accept_lock);
	return result;
}

/** Connects socket to the remote server.
 *
 * @param[in] socket_id	Socket identifier.
 * @param[in] serv_addr	The remote server address.
 * @param[in] addrlen	The address length.
 * @return		EOK on success.
 * @return		EBADMEM if the serv_addr parameter is NULL.
 * @return		NO_DATA if the addlen parameter is zero.
 * @return		ENOTSOCK if the socket is not found.
 * @return		Other error codes as defined for the NET_SOCKET_CONNECT
 *			message.
 */
int connect(int socket_id, const struct sockaddr *serv_addr, socklen_t addrlen)
{
	if (!serv_addr)
		return EDESTADDRREQ;

	if (!addrlen)
		return EDESTADDRREQ;

	/* Send the address */
	return socket_send_data(socket_id, NET_SOCKET_CONNECT, 0, serv_addr,
	    addrlen);
}

/** Clears and destroys the socket.
 *
 * @param[in] socket	The socket to be destroyed.
 */
static void socket_destroy(socket_t *socket)
{
	int accepted_id;

	/* Destroy all accepted sockets */
	while ((accepted_id = dyn_fifo_pop(&socket->accepted)) >= 0)
		socket_destroy(sockets_find(socket_get_sockets(), accepted_id));

	dyn_fifo_destroy(&socket->received);
	dyn_fifo_destroy(&socket->accepted);
	sockets_exclude(socket_get_sockets(), socket->socket_id, free);
}

/** Closes the socket.
 *
 * @param[in] socket_id	Socket identifier.
 * @return		EOK on success.
 * @return		ENOTSOCK if the socket is not found.
 * @return		EINPROGRESS if there is another blocking function in
 *			progress.
 * @return		Other error codes as defined for the NET_SOCKET_CLOSE
 *			message.
 */
int closesocket(int socket_id)
{
	socket_t *socket;
	int rc;

	fibril_rwlock_write_lock(&socket_globals.lock);

	socket = sockets_find(socket_get_sockets(), socket_id);
	if (!socket) {
		fibril_rwlock_write_unlock(&socket_globals.lock);
		return ENOTSOCK;
	}
	if (socket->blocked) {
		fibril_rwlock_write_unlock(&socket_globals.lock);
		return EINPROGRESS;
	}

	/* Request close */
	async_exch_t *exch = async_exchange_begin(socket->sess);
	rc = (int) async_req_3_0(exch, NET_SOCKET_CLOSE,
	    (sysarg_t) socket->socket_id, 0, socket->service);
	async_exchange_end(exch);
	
	if (rc != EOK) {
		fibril_rwlock_write_unlock(&socket_globals.lock);
		return rc;
	}
	/* Free the socket structure */
	socket_destroy(socket);

	fibril_rwlock_write_unlock(&socket_globals.lock);
	return EOK;
}

/** Sends data via the socket to the remote address.
 *
 * Binds the socket to a free port if not already connected/bound.
 *
 * @param[in] message	The action message.
 * @param[in] socket_id Socket identifier.
 * @param[in] data	The data to be sent.
 * @param[in] datalength The data length.
 * @param[in] flags	Various send flags.
 * @param[in] toaddr	The destination address. May be NULL for connected
 *			sockets.
 * @param[in] addrlen	The address length. Used only if toaddr is not NULL.
 * @return		EOK on success.
 * @return		ENOTSOCK if the socket is not found.
 * @return		EBADMEM if the data or toaddr parameter is NULL.
 * @return		NO_DATA if the datalength or the addrlen parameter is
 *			zero (0).
 * @return		Other error codes as defined for the NET_SOCKET_SENDTO
 *			message.
 */
static int
sendto_core(sysarg_t message, int socket_id, const void *data,
    size_t datalength, int flags, const struct sockaddr *toaddr,
    socklen_t addrlen)
{
	socket_t *socket;
	aid_t message_id;
	sysarg_t result;
	size_t fragments;
	ipc_call_t answer;

	if (!data)
		return EBADMEM;

	if (!datalength)
		return NO_DATA;

	fibril_rwlock_read_lock(&socket_globals.lock);

	/* Find socket */
	socket = sockets_find(socket_get_sockets(), socket_id);
	if (!socket) {
		fibril_rwlock_read_unlock(&socket_globals.lock);
		return ENOTSOCK;
	}

	fibril_rwlock_read_lock(&socket->sending_lock);

	/* Compute data fragment count */
	if (socket->data_fragment_size > 0) {
		fragments = (datalength + socket->header_size) /
		    socket->data_fragment_size;
		if ((datalength + socket->header_size) %
		    socket->data_fragment_size)
			++fragments;
	} else {
		fragments = 1;
	}

	/* Request send */
	async_exch_t *exch = async_exchange_begin(socket->sess);
	
	message_id = async_send_5(exch, message,
	    (sysarg_t) socket->socket_id,
	    (fragments == 1 ? datalength : socket->data_fragment_size),
	    socket->service, (sysarg_t) flags, fragments, &answer);

	/* Send the address if given */
	if (!toaddr ||
	    (async_data_write_start(exch, toaddr, addrlen) == EOK)) {
		if (fragments == 1) {
			/* Send all if only one fragment */
			async_data_write_start(exch, data, datalength);
		} else {
			/* Send the first fragment */
			async_data_write_start(exch, data,
			    socket->data_fragment_size - socket->header_size);
			data = ((const uint8_t *) data) +
			    socket->data_fragment_size - socket->header_size;
	
			/* Send the middle fragments */
			while (--fragments > 1) {
				async_data_write_start(exch, data,
				    socket->data_fragment_size);
				data = ((const uint8_t *) data) +
				    socket->data_fragment_size;
			}

			/* Send the last fragment */
			async_data_write_start(exch, data,
			    (datalength + socket->header_size) %
			    socket->data_fragment_size);
		}
	}
	
	async_exchange_end(exch);

	async_wait_for(message_id, &result);

	if ((SOCKET_GET_DATA_FRAGMENT_SIZE(answer) > 0) &&
	    (SOCKET_GET_DATA_FRAGMENT_SIZE(answer) !=
	    socket->data_fragment_size)) {
		/* Set the data fragment size */
		socket->data_fragment_size =
		    SOCKET_GET_DATA_FRAGMENT_SIZE(answer);
	}

	fibril_rwlock_read_unlock(&socket->sending_lock);
	fibril_rwlock_read_unlock(&socket_globals.lock);
	return (int) result;
}

/** Sends data via the socket.
 *
 * @param[in] socket_id	Socket identifier.
 * @param[in] data	The data to be sent.
 * @param[in] datalength The data length.
 * @param[in] flags	Various send flags.
 * @return		EOK on success.
 * @return		ENOTSOCK if the socket is not found.
 * @return		EBADMEM if the data parameter is NULL.
 * @return		NO_DATA if the datalength parameter is zero.
 * @return		Other error codes as defined for the NET_SOCKET_SEND
 *			message.
 */
int send(int socket_id, const void *data, size_t datalength, int flags)
{
	/* Without the address */
	return sendto_core(NET_SOCKET_SEND, socket_id, data, datalength, flags,
	    NULL, 0);
}

/** Sends data via the socket to the remote address.
 *
 * Binds the socket to a free port if not already connected/bound.
 *
 * @param[in] socket_id	Socket identifier.
 * @param[in] data	The data to be sent.
 * @param[in] datalength The data length.
 * @param[in] flags	Various send flags.
 * @param[in] toaddr	The destination address.
 * @param[in] addrlen	The address length.
 * @return		EOK on success.
 * @return		ENOTSOCK if the socket is not found.
 * @return		EBADMEM if the data or toaddr parameter is NULL.
 * @return		NO_DATA if the datalength or the addrlen parameter is
 *			zero.
 * @return		Other error codes as defined for the NET_SOCKET_SENDTO
 *			message.
 */
int
sendto(int socket_id, const void *data, size_t datalength, int flags,
    const struct sockaddr *toaddr, socklen_t addrlen)
{
	if (!toaddr)
		return EDESTADDRREQ;

	if (!addrlen)
		return EDESTADDRREQ;

	/* With the address */
	return sendto_core(NET_SOCKET_SENDTO, socket_id, data, datalength,
	    flags, toaddr, addrlen);
}

/** Receives data via the socket.
 *
 * @param[in] message	The action message.
 * @param[in] socket_id	Socket identifier.
 * @param[out] data	The data buffer to be filled.
 * @param[in] datalength The data length.
 * @param[in] flags	Various receive flags.
 * @param[out] fromaddr	The source address. May be NULL for connected sockets.
 * @param[in,out] addrlen The address length. The maximum address length is
 *			read. The actual address length is set. Used only if
 *			fromaddr is not NULL.
 * @return		Positive received message size in bytes on success.
 * @return		Zero if no more data (other side closed the connection).
 * @return		ENOTSOCK if the socket is not found.
 * @return		EBADMEM if the data parameter is NULL.
 * @return		NO_DATA if the datalength or addrlen parameter is zero.
 * @return		Other error codes as defined for the spcific message.
 */
static ssize_t
recvfrom_core(sysarg_t message, int socket_id, void *data, size_t datalength,
    int flags, struct sockaddr *fromaddr, socklen_t *addrlen)
{
	socket_t *socket;
	aid_t message_id;
	sysarg_t ipc_result;
	int result;
	size_t fragments;
	size_t *lengths;
	size_t index;
	ipc_call_t answer;
	ssize_t retval;

	if (!data)
		return EBADMEM;

	if (!datalength)
		return NO_DATA;

	if (fromaddr && !addrlen)
		return EINVAL;

	fibril_rwlock_read_lock(&socket_globals.lock);

	/* Find the socket */
	socket = sockets_find(socket_get_sockets(), socket_id);
	if (!socket) {
		fibril_rwlock_read_unlock(&socket_globals.lock);
		return ENOTSOCK;
	}

	fibril_mutex_lock(&socket->receive_lock);
	/* Wait for a received packet */
	++socket->blocked;
	while ((result = dyn_fifo_value(&socket->received)) < 0) {
		fibril_rwlock_read_unlock(&socket_globals.lock);
		fibril_condvar_wait(&socket->receive_signal,
		    &socket->receive_lock);

		/* Drop the receive lock to avoid deadlock */
		fibril_mutex_unlock(&socket->receive_lock);
		fibril_rwlock_read_lock(&socket_globals.lock);
		fibril_mutex_lock(&socket->receive_lock);
	}
	--socket->blocked;
	fragments = (size_t) result;

	if (fragments == 0) {
		/* No more data, other side has closed the connection. */
		fibril_mutex_unlock(&socket->receive_lock);
		fibril_rwlock_read_unlock(&socket_globals.lock);
		return 0;
	}
	
	async_exch_t *exch = async_exchange_begin(socket->sess);

	/* Prepare lengths if more fragments */
	if (fragments > 1) {
		lengths = (size_t *) malloc(sizeof(size_t) * fragments +
		    sizeof(size_t));
		if (!lengths) {
			fibril_mutex_unlock(&socket->receive_lock);
			fibril_rwlock_read_unlock(&socket_globals.lock);
			return ENOMEM;
		}

		/* Request packet data */
		message_id = async_send_4(exch, message,
		    (sysarg_t) socket->socket_id, 0, socket->service,
		    (sysarg_t) flags, &answer);

		/* Read the address if desired */
		if(!fromaddr ||
		    (async_data_read_start(exch, fromaddr,
		    *addrlen) == EOK)) {
			/* Read the fragment lengths */
			if (async_data_read_start(exch, lengths,
			    sizeof(int) * (fragments + 1)) == EOK) {
				if (lengths[fragments] <= datalength) {

					/* Read all fragments if long enough */
					for (index = 0; index < fragments;
					    ++index) {
						async_data_read_start(exch, data,
						    lengths[index]);
						data = ((uint8_t *) data) +
						    lengths[index];
					}
				}
			}
		}

		free(lengths);
	} else { /* fragments == 1 */
		/* Request packet data */
		message_id = async_send_4(exch, message,
		    (sysarg_t) socket->socket_id, 0, socket->service,
		    (sysarg_t) flags, &answer);

		/* Read the address if desired */
		if (!fromaddr ||
		    (async_data_read_start(exch, fromaddr, *addrlen) == EOK)) {
			/* Read all if only one fragment */
			async_data_read_start(exch, data, datalength);
		}
	}
	
	async_exchange_end(exch);

	async_wait_for(message_id, &ipc_result);
	result = (int) ipc_result;
	if (result == EOK) {
		/* Dequeue the received packet */
		dyn_fifo_pop(&socket->received);
		/* Return read data length */
		retval = SOCKET_GET_READ_DATA_LENGTH(answer);
		/* Set address length */
		if (fromaddr && addrlen)
			*addrlen = SOCKET_GET_ADDRESS_LENGTH(answer);
	} else {
		retval = (ssize_t) result;
	}

	fibril_mutex_unlock(&socket->receive_lock);
	fibril_rwlock_read_unlock(&socket_globals.lock);
	return retval;
}

/** Receives data via the socket.
 *
 * @param[in] socket_id	Socket identifier.
 * @param[out] data	The data buffer to be filled.
 * @param[in] datalength The data length.
 * @param[in] flags	Various receive flags.
 * @return		Positive received message size in bytes on success.
 * @return		Zero if no more data (other side closed the connection).
 * @return		ENOTSOCK if the socket is not found.
 * @return		EBADMEM if the data parameter is NULL.
 * @return		NO_DATA if the datalength parameter is zero.
 * @return		Other error codes as defined for the NET_SOCKET_RECV
 *			message.
 */
ssize_t recv(int socket_id, void *data, size_t datalength, int flags)
{
	/* Without the address */
	return recvfrom_core(NET_SOCKET_RECV, socket_id, data, datalength,
	    flags, NULL, NULL);
}

/** Receives data via the socket.
 *
 * @param[in] socket_id	Socket identifier.
 * @param[out] data	The data buffer to be filled.
 * @param[in] datalength The data length.
 * @param[in] flags	Various receive flags.
 * @param[out] fromaddr	The source address.
 * @param[in,out] addrlen The address length. The maximum address length is
 *			read. The actual address length is set.
 * @return		Positive received message size in bytes on success.
 * @return		Zero if no more data (other side closed the connection).
 * @return		ENOTSOCK if the socket is not found.
 * @return		EBADMEM if the data or fromaddr parameter is NULL.
 * @return		NO_DATA if the datalength or addrlen parameter is zero.
 * @return		Other error codes as defined for the NET_SOCKET_RECVFROM
 *			message.
 */
ssize_t
recvfrom(int socket_id, void *data, size_t datalength, int flags,
    struct sockaddr *fromaddr, socklen_t *addrlen)
{
	if (!fromaddr)
		return EBADMEM;

	if (!addrlen)
		return NO_DATA;

	/* With the address */
	return recvfrom_core(NET_SOCKET_RECVFROM, socket_id, data, datalength,
	    flags, fromaddr, addrlen);
}

/** Gets socket option.
 *
 * @param[in] socket_id	Socket identifier.
 * @param[in] level	The socket options level.
 * @param[in] optname	The socket option to be get.
 * @param[out] value	The value buffer to be filled.
 * @param[in,out] optlen The value buffer length. The maximum length is read.
 *			The actual length is set.
 * @return		EOK on success.
 * @return		ENOTSOCK if the socket is not found.
 * @return		EBADMEM if the value or optlen parameter is NULL.
 * @return		NO_DATA if the optlen parameter is zero.
 * @return		Other error codes as defined for the
 *			NET_SOCKET_GETSOCKOPT message.
 */
int
getsockopt(int socket_id, int level, int optname, void *value, size_t *optlen)
{
	socket_t *socket;
	aid_t message_id;
	sysarg_t result;

	if (!value || !optlen)
		return EBADMEM;

	if (!*optlen)
		return NO_DATA;

	fibril_rwlock_read_lock(&socket_globals.lock);

	/* Find the socket */
	socket = sockets_find(socket_get_sockets(), socket_id);
	if (!socket) {
		fibril_rwlock_read_unlock(&socket_globals.lock);
		return ENOTSOCK;
	}

	/* Request option value */
	async_exch_t *exch = async_exchange_begin(socket->sess);
	
	message_id = async_send_3(exch, NET_SOCKET_GETSOCKOPT,
	    (sysarg_t) socket->socket_id, (sysarg_t) optname, socket->service,
	    NULL);

	/* Read the length */
	if (async_data_read_start(exch, optlen,
	    sizeof(*optlen)) == EOK) {
		/* Read the value */
		async_data_read_start(exch, value, *optlen);
	}
	
	async_exchange_end(exch);

	fibril_rwlock_read_unlock(&socket_globals.lock);
	async_wait_for(message_id, &result);
	return (int) result;
}

/** Sets socket option.
 *
 * @param[in] socket_id	Socket identifier.
 * @param[in] level	The socket options level.
 * @param[in] optname	The socket option to be set.
 * @param[in] value	The value to be set.
 * @param[in] optlen	The value length.
 * @return		EOK on success.
 * @return		ENOTSOCK if the socket is not found.
 * @return		EBADMEM if the value parameter is NULL.
 * @return		NO_DATA if the optlen parameter is zero.
 * @return		Other error codes as defined for the
 *			NET_SOCKET_SETSOCKOPT message.
 */
int
setsockopt(int socket_id, int level, int optname, const void *value,
    size_t optlen)
{
	/* Send the value */
	return socket_send_data(socket_id, NET_SOCKET_SETSOCKOPT,
	    (sysarg_t) optname, value, optlen);
}

/** @}
 */
