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

/** @addtogroup libnet
 * @{
 */

/** @file
 * Socket common core implementation.
 */

#include <socket_core.h>
#include <net/socket_codes.h>
#include <net/in.h>
#include <net/inet.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <adt/dynamic_fifo.h>
#include <adt/int_map.h>

/**
 * Maximum number of random attempts to find a new socket identifier before
 * switching to the sequence.
 */
#define SOCKET_ID_TRIES  100

/** Bound port sockets.*/
struct socket_port {
	/** The bound sockets map. */
	socket_port_map_t map;
	/** The bound sockets count. */
	int count;
};

INT_MAP_IMPLEMENT(socket_cores, socket_core_t);

GENERIC_CHAR_MAP_IMPLEMENT(socket_port_map, socket_core_t *);

INT_MAP_IMPLEMENT(socket_ports, socket_port_t);

/** Destroy the socket.
 *
 * If the socket is bound, the port is released.
 * Release all buffered packets, call the release function and remove the
 * socket from the local sockets.
 *
 * @param[in]     sess           Packet server session.
 * @param[in]     socket         Socket to be destroyed.
 * @param[in,out] local_sockets  Local sockets to be updated.
 * @param[in,out] global_sockets Global sockets to be updated.
 * @param[in]     socket_release Client release callback function.
 *
 */
static void socket_destroy_core(async_sess_t *sess, socket_core_t *socket,
    socket_cores_t *local_sockets, socket_ports_t *global_sockets,
    void (* socket_release)(socket_core_t *socket))
{
	/* If bound */
	if (socket->port) {
		/* Release the port */
		socket_port_release(global_sockets, socket);
	}
	
	dyn_fifo_destroy(&socket->accepted);
	
	if (socket_release)
		socket_release(socket);
	
	socket_cores_exclude(local_sockets, socket->socket_id, free);
}

/** Destroy local sockets.
 *
 * Release all buffered packets and call the release function for each of the
 * sockets.
 *
 * @param[in]     sess           Packet server session.
 * @param[in]     local_sockets  Local sockets to be destroyed.
 * @param[in,out] global_sockets Global sockets to be updated.
 * @param[in]     socket_release Client release callback function.
 *
 */
void socket_cores_release(async_sess_t *sess, socket_cores_t *local_sockets,
    socket_ports_t *global_sockets,
    void (* socket_release)(socket_core_t *socket))
{
	if (!socket_cores_is_valid(local_sockets))
		return;
	
	local_sockets->magic = 0;
	
	int index;
	for (index = 0; index < local_sockets->next; ++index) {
		if (socket_cores_item_is_valid(&local_sockets->items[index])) {
			local_sockets->items[index].magic = 0;
			
			if (local_sockets->items[index].value) {
				socket_destroy_core(sess,
				    local_sockets->items[index].value,
				    local_sockets, global_sockets,
				    socket_release);
				free(local_sockets->items[index].value);
				local_sockets->items[index].value = NULL;
			}
		}
	}
	
	free(local_sockets->items);
}

/** Adds the socket to a socket port.
 *
 * @param[in,out] socket_port The socket port structure.
 * @param[in] socket	The socket to be added.
 * @param[in] key	The socket key identifier.
 * @param[in] key_length The socket key length.
 * @return		EOK on success.
 * @return		ENOMEM if there is not enough memory left.
 */
static int
socket_port_add_core(socket_port_t *socket_port, socket_core_t *socket,
    const uint8_t *key, size_t key_length)
{
	socket_core_t **socket_ref;
	int rc;

	/* Create a wrapper */
	socket_ref = malloc(sizeof(*socket_ref));
	if (!socket_ref)
		return ENOMEM;

	*socket_ref = socket;
	/* Add the wrapper */
	rc = socket_port_map_add(&socket_port->map, key, key_length,
	    socket_ref);
	if (rc != EOK) {
		free(socket_ref);
		return rc;
	}
	
	++socket_port->count;
	socket->key = key;
	socket->key_length = key_length;
	
	return EOK;
}

/** Binds the socket to the port.
 *
 * The SOCKET_MAP_KEY_LISTENING key identifier is used.
 *
 * @param[in] global_sockets The global sockets to be updated.
 * @param[in] socket	The socket to be added.
 * @param[in] port	The port number to be bound to.
 * @return		EOK on success.
 * @return		ENOMEM if there is not enough memory left.
 * @return		Other error codes as defined for the
 *			 socket_ports_add() function.
 */
static int
socket_bind_insert(socket_ports_t *global_sockets, socket_core_t *socket,
    int port)
{
	socket_port_t *socket_port;
	int rc;

	/* Create a wrapper */
	socket_port = malloc(sizeof(*socket_port));
	if (!socket_port)
		return ENOMEM;

	socket_port->count = 0;
	rc = socket_port_map_initialize(&socket_port->map);
	if (rc != EOK)
		goto fail;
	
	rc = socket_port_add_core(socket_port, socket,
	    (const uint8_t *) SOCKET_MAP_KEY_LISTENING, 0);
	if (rc != EOK)
		goto fail;
	
	/* Register the incoming port */
	rc = socket_ports_add(global_sockets, port, socket_port);
	if (rc < 0)
		goto fail;
	
	socket->port = port;
	return EOK;

fail:
	socket_port_map_destroy(&socket_port->map, free);
	free(socket_port);
	return rc;
	
}

/** Binds the socket to the port.
 *
 * The address port is used if set, a free port is used if not.
 *
 * @param[in] local_sockets The local sockets to be searched.
 * @param[in,out] global_sockets The global sockets to be updated.
 * @param[in] socket_id	The new socket identifier.
 * @param[in] addr	The address to be bound to.
 * @param[in] addrlen	The address length.
 * @param[in] free_ports_start The minimum free port.
 * @param[in] free_ports_end The maximum free port.
 * @param[in] last_used_port The last used free port.
 * @return		EOK on success.
 * @return		ENOTSOCK if the socket was not found.
 * @return		EAFNOSUPPORT if the address family is not supported.
 * @return		EADDRINUSE if the port is already in use.
 * @return		Other error codes as defined for the
 *			socket_bind_free_port() function.
 * @return		Other error codes as defined for the
 *			socket_bind_insert() function.
 */
int
socket_bind(socket_cores_t *local_sockets, socket_ports_t *global_sockets,
    int socket_id, void *addr, size_t addrlen, int free_ports_start,
    int free_ports_end, int last_used_port)
{
	socket_core_t *socket;
	socket_port_t *socket_port;
	struct sockaddr *address;
	struct sockaddr_in *address_in;

	if (addrlen < sizeof(struct sockaddr))
		return EINVAL;

	address = (struct sockaddr *) addr;
	switch (address->sa_family) {
	case AF_INET:
		if (addrlen != sizeof(struct sockaddr_in))
			return EINVAL;
		
		address_in = (struct sockaddr_in *) addr;
		/* Find the socket */
		socket = socket_cores_find(local_sockets, socket_id);
		if (!socket)
			return ENOTSOCK;
		
		/* Bind a free port? */
		if (address_in->sin_port <= 0)
			return socket_bind_free_port(global_sockets, socket,
			     free_ports_start, free_ports_end, last_used_port);
		
		/* Try to find the port */
		socket_port = socket_ports_find(global_sockets,
		    ntohs(address_in->sin_port));
		if (socket_port) {
			/* Already used */
			return EADDRINUSE;
		}
		
		/* If bound */
		if (socket->port) {
			/* Release the port */
			socket_port_release(global_sockets, socket);
		}
		socket->port = -1;
		
		return socket_bind_insert(global_sockets, socket,
		    ntohs(address_in->sin_port));
		
	case AF_INET6:
		// TODO IPv6
		break;
	}
	
	return EAFNOSUPPORT;
}

/** Binds the socket to a free port.
 *
 * The first free port is used.
 *
 * @param[in,out] global_sockets The global sockets to be updated.
 * @param[in,out] socket The socket to be bound.
 * @param[in] free_ports_start The minimum free port.
 * @param[in] free_ports_end The maximum free port.
 * @param[in] last_used_port The last used free port.
 * @return		EOK on success.
 * @return		ENOTCONN if no free port was found.
 * @return		Other error codes as defined for the
 *			socket_bind_insert() function.
 */
int
socket_bind_free_port(socket_ports_t *global_sockets, socket_core_t *socket,
    int free_ports_start, int free_ports_end, int last_used_port)
{
	int index;

	/* From the last used one */
	index = last_used_port;
	
	do {
		++index;
		
		/* Till the range end */
		if (index >= free_ports_end) {
			/* Start from the range beginning */
			index = free_ports_start - 1;
			do {
				++index;
				/* Till the last used one */
				if (index >= last_used_port) {
					/* None found */
					return ENOTCONN;
				}
			} while (socket_ports_find(global_sockets, index));
			
			/* Found, break immediately */
			break;
		}
		
	} while (socket_ports_find(global_sockets, index));
	
	return socket_bind_insert(global_sockets, socket, index);
}

/** Tries to find a new free socket identifier.
 *
 * @param[in] local_sockets The local sockets to be searched.
 * @param[in] positive	A value indicating whether a positive identifier is
 *			requested. A negative identifier is requested if set to
 *			false.
 * @return		The new socket identifier.
 * @return		ELIMIT if there is no socket identifier available.
 */
static int socket_generate_new_id(socket_cores_t *local_sockets, int positive)
{
	int socket_id;
	int count;

	count = 0;
#if 0
	socket_id = socket_globals.last_id;
#endif
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
				++ socket_id;
#if 0
			} else if(socket_globals.last_id) {
				socket_globals.last_id = 0;
				socket_id = 1;
#endif
			} else {
				return ELIMIT;
			}
		}
	} while (socket_cores_find(local_sockets,
	    ((positive ? 1 : -1) * socket_id)));
	
//	last_id = socket_id
	return socket_id;
}

/** Create a new socket.
 *
 * @param[in,out] local_sockets Local sockets to be updated.
 * @param[in]     sess          Application session.
 * @param[in]     specific_data Socket specific data.
 * @param[in,out] socket_id     New socket identifier. A new identifier
 *                              is chosen if set to zero or negative.
 *                              A negative identifier is chosen if set
 *                              to negative.
 *
 * @return EOK on success.
 * @return EINVAL if the socket_id parameter is NULL.
 * @return ENOMEM if there is not enough memory left.
 *
 */
int socket_create(socket_cores_t *local_sockets, async_sess_t* sess,
    void *specific_data, int *socket_id)
{
	socket_core_t *socket;
	int positive;
	int rc;

	if (!socket_id)
		return EINVAL;
	
	/* Store the socket */
	if (*socket_id <= 0) {
		positive = (*socket_id == 0);
		*socket_id = socket_generate_new_id(local_sockets, positive);
		if (*socket_id <= 0)
			return *socket_id;
		if (!positive)
			*socket_id *= -1;
	} else if(socket_cores_find(local_sockets, *socket_id)) {
		return EEXIST;
	}
	
	socket = (socket_core_t *) malloc(sizeof(*socket));
	if (!socket)
		return ENOMEM;
	
	/* Initialize */
	socket->sess = sess;
	socket->port = -1;
	socket->key = NULL;
	socket->key_length = 0;
	socket->specific_data = specific_data;
	
	rc = dyn_fifo_initialize(&socket->accepted, SOCKET_INITIAL_ACCEPTED_SIZE);
	if (rc != EOK) {
		free(socket);
		return rc;
	}
	socket->socket_id = *socket_id;
	rc = socket_cores_add(local_sockets, socket->socket_id, socket);
	if (rc < 0) {
		dyn_fifo_destroy(&socket->accepted);
		free(socket);
		return rc;
	}
	
	return EOK;
}

/** Destroy the socket.
 *
 * If the socket is bound, the port is released.
 * Release all buffered packets, call the release function and remove the
 * socket from the local sockets.
 *
 * @param[in]     sess           Packet server session.
 * @param[in]     socket_id      Socket identifier.
 * @param[in,out] local_sockets  Local sockets to be updated.
 * @param[in,out] global_sockets Global sockets to be updated.
 * @param[in]     socket_release Client release callback function.
 *
 * @return EOK on success.
 * @return ENOTSOCK if the socket is not found.
 *
 */
int
socket_destroy(async_sess_t *sess, int socket_id, socket_cores_t *local_sockets,
    socket_ports_t *global_sockets,
    void (*socket_release)(socket_core_t *socket))
{
	/* Find the socket */
	socket_core_t *socket = socket_cores_find(local_sockets, socket_id);
	if (!socket)
		return ENOTSOCK;
	
	/* Destroy all accepted sockets */
	int accepted_id;
	while ((accepted_id = dyn_fifo_pop(&socket->accepted)) >= 0)
		socket_destroy(sess, accepted_id, local_sockets,
		    global_sockets, socket_release);
	
	socket_destroy_core(sess, socket, local_sockets, global_sockets,
	    socket_release);
	
	return EOK;
}

/** Finds the bound port socket.
 *
 * @param[in] global_sockets The global sockets to be searched.
 * @param[in] port	The port number.
 * @param[in] key	The socket key identifier.
 * @param[in] key_length The socket key length.
 * @return		The found socket.
 * @return		NULL if no socket was found.
 */
socket_core_t *
socket_port_find(socket_ports_t *global_sockets, int port, const uint8_t *key,
    size_t key_length)
{
	socket_port_t *socket_port;
	socket_core_t **socket_ref;

	socket_port = socket_ports_find(global_sockets, port);
	if (socket_port && (socket_port->count > 0)) {
		socket_ref = socket_port_map_find(&socket_port->map, key,
		    key_length);
		if (socket_ref)
			return *socket_ref;
	}
	
	return NULL;
}

/** Releases the socket port.
 *
 * If the socket is bound the port entry is released.
 * If there are no more port entries the port is release.
 *
 * @param[in] global_sockets The global sockets to be updated.
 * @param[in] socket	The socket to be unbound.
 */
void
socket_port_release(socket_ports_t *global_sockets, socket_core_t *socket)
{
	socket_port_t *socket_port;
	socket_core_t **socket_ref;

	if (!socket->port)
		return;
	
	/* Find ports */
	socket_port = socket_ports_find(global_sockets, socket->port);
	if (socket_port) {
		/* Find the socket */
		socket_ref = socket_port_map_find(&socket_port->map,
		    socket->key, socket->key_length);
		
		if (socket_ref) {
			--socket_port->count;
			
			/* Release if empty */
			if (socket_port->count <= 0) {
				/* Destroy the map */
				socket_port_map_destroy(&socket_port->map, free);
				/* Release the port */
				socket_ports_exclude(global_sockets,
				    socket->port, free);
			} else {
				/* Remove */
				socket_port_map_exclude(&socket_port->map,
				    socket->key, socket->key_length, free);
			}
		}
	}
	
	socket->port = 0;
	socket->key = NULL;
	socket->key_length = 0;
}

/** Adds the socket to an already bound port.
 *
 * @param[in] global_sockets The global sockets to be updated.
 * @param[in] port	The port number to be bound to.
 * @param[in] socket	The socket to be added.
 * @param[in] key	The socket key identifier.
 * @param[in] key_length The socket key length.
 * @return		EOK on success.
 * @return		ENOENT if the port is not already used.
 * @return		Other error codes as defined for the
 *			socket_port_add_core() function.
 */
int
socket_port_add(socket_ports_t *global_sockets, int port,
    socket_core_t *socket, const uint8_t *key, size_t key_length)
{
	socket_port_t *socket_port;
	int rc;

	/* Find ports */
	socket_port = socket_ports_find(global_sockets, port);
	if (!socket_port)
		return ENOENT;
	
	/* Add the socket */
	rc = socket_port_add_core(socket_port, socket, key, key_length);
	if (rc != EOK)
		return rc;
	
	socket->port = port;
	return EOK;
}

/** @}
 */
