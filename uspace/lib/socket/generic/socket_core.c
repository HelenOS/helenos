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

/** @addtogroup socket
 *  @{
 */

/** @file
 *  Socket common core implementation.
 */

#include <limits.h>
#include <stdlib.h>

#include <net_err.h>
#include <in.h>
#include <inet.h>
#include <socket_codes.h>
#include <socket_errno.h>
#include <adt/dynamic_fifo.h>
#include <adt/int_map.h>
#include <packet/packet.h>
#include <packet/packet_client.h>
#include <net_modules.h>
#include <socket_core.h>

/** Maximum number of random attempts to find a new socket identifier before switching to the sequence.
 */
#define SOCKET_ID_TRIES					100

/** Bound port sockets.
 */
struct socket_port{
	/** The bound sockets map.
	 */
	socket_port_map_t map;
	/** The bound sockets count.
	 */
	int count;
};

INT_MAP_IMPLEMENT(socket_cores, socket_core_t);

GENERIC_CHAR_MAP_IMPLEMENT(socket_port_map, socket_core_ref);

INT_MAP_IMPLEMENT(socket_ports, socket_port_t);

/** Destroys the socket.
 *  If the socket is bound, the port is released.
 *  Releases all buffered packets, calls the release function and removes the socket from the local sockets.
 *  @param[in] packet_phone The packet server phone to release buffered packets.
 *  @param[in] socket The socket to be destroyed.
 *  @param[in,out] local_sockets The local sockets to be updated.
 *  @param[in,out] global_sockets The global sockets to be updated.
 *  @param[in] socket_release The client release callback function.
 */
static void socket_destroy_core(int packet_phone, socket_core_ref socket, socket_cores_ref local_sockets, socket_ports_ref global_sockets, void (*socket_release)(socket_core_ref socket)){
	int packet_id;

	// if bound
	if(socket->port){
		// release the port
		socket_port_release(global_sockets, socket);
	}
	// release all received packets
	while((packet_id = dyn_fifo_pop(&socket->received)) >= 0){
		pq_release(packet_phone, packet_id);
	}
	dyn_fifo_destroy(&socket->received);
	dyn_fifo_destroy(&socket->accepted);
	if(socket_release){
		socket_release(socket);
	}
	socket_cores_exclude(local_sockets, socket->socket_id);
}

void socket_cores_release(int packet_phone, socket_cores_ref local_sockets, socket_ports_ref global_sockets, void (*socket_release)(socket_core_ref socket)){
	if(socket_cores_is_valid(local_sockets)){
		int index;

		local_sockets->magic = 0;
		for(index = 0; index < local_sockets->next; ++ index){
			if(socket_cores_item_is_valid(&(local_sockets->items[index]))){
				local_sockets->items[index].magic = 0;
				if(local_sockets->items[index].value){
					socket_destroy_core(packet_phone, local_sockets->items[index].value, local_sockets, global_sockets, socket_release);
					free(local_sockets->items[index].value);
					local_sockets->items[index].value = NULL;
				}
			}
		}
		free(local_sockets->items);
	}
}

/** Adds the socket to a socket port.
 *  @param[in,out] socket_port The socket port structure.
 *  @param[in] socket The socket to be added.
 *  @param[in] key The socket key identifier.
 *  @param[in] key_length The socket key length.
 *  @returns EOK on success.
 *  @returns ENOMEM if there is not enough memory left.
 */
static int socket_port_add_core(socket_port_ref socket_port, socket_core_ref socket, const char * key, size_t key_length){
	ERROR_DECLARE;

	socket_core_ref * socket_ref;

	// create a wrapper
	socket_ref = malloc(sizeof(*socket_ref));
	if(! socket_ref){
		return ENOMEM;
	}
	*socket_ref = socket;
	// add the wrapper
	if(ERROR_OCCURRED(socket_port_map_add(&socket_port->map, key, key_length, socket_ref))){
		free(socket_ref);
		return ERROR_CODE;
	}
	++ socket_port->count;
	socket->key = key;
	socket->key_length = key_length;
	return EOK;
}

/** Binds the socket to the port.
 *  The SOCKET_MAP_KEY_LISTENING key identifier is used.
 *  @param[in] global_sockets The global sockets to be updated.
 *  @param[in] socket The socket to be added.
 *  @param[in] port The port number to be bound to.
 *  @returns EOK on success.
 *  @returns ENOMEM if there is not enough memory left.
 *  @returns Other error codes as defined for the socket_ports_add() function.
 */
static int socket_bind_insert(socket_ports_ref global_sockets, socket_core_ref socket, int port){
	ERROR_DECLARE;

	socket_port_ref socket_port;

	// create a wrapper
	socket_port = malloc(sizeof(*socket_port));
	if(! socket_port){
		return ENOMEM;
	}
	socket_port->count = 0;
	if(ERROR_OCCURRED(socket_port_map_initialize(&socket_port->map))
		|| ERROR_OCCURRED(socket_port_add_core(socket_port, socket, SOCKET_MAP_KEY_LISTENING, 0))){
		socket_port_map_destroy(&socket_port->map);
		free(socket_port);
		return ERROR_CODE;
	}
	// register the incomming port
	ERROR_CODE = socket_ports_add(global_sockets, port, socket_port);
	if(ERROR_CODE < 0){
		socket_port_map_destroy(&socket_port->map);
		free(socket_port);
		return ERROR_CODE;
	}
	socket->port = port;
	return EOK;
}

int socket_bind(socket_cores_ref local_sockets, socket_ports_ref global_sockets, int socket_id, void * addr, size_t addrlen, int free_ports_start, int free_ports_end, int last_used_port){
	socket_core_ref socket;
	socket_port_ref socket_port;
	struct sockaddr * address;
	struct sockaddr_in * address_in;

	if(addrlen < sizeof(struct sockaddr)){
		return EINVAL;
	}
	address = (struct sockaddr *) addr;
	switch(address->sa_family){
		case AF_INET:
			if(addrlen != sizeof(struct sockaddr_in)){
				return EINVAL;
			}
			address_in = (struct sockaddr_in *) addr;
			// find the socket
			socket = socket_cores_find(local_sockets, socket_id);
			if(! socket){
				return ENOTSOCK;
			}
			// bind a free port?
			if(address_in->sin_port <= 0){
				return socket_bind_free_port(global_sockets, socket, free_ports_start, free_ports_end, last_used_port);
			}
			// try to find the port
			socket_port = socket_ports_find(global_sockets, ntohs(address_in->sin_port));
			if(socket_port){
				// already used
				return EADDRINUSE;
			}
			// if bound
			if(socket->port){
				// release the port
				socket_port_release(global_sockets, socket);
			}
			socket->port = -1;
			return socket_bind_insert(global_sockets, socket, ntohs(address_in->sin_port));
			break;
		// TODO IPv6
	}
	return EAFNOSUPPORT;
}

int socket_bind_free_port(socket_ports_ref global_sockets, socket_core_ref socket, int free_ports_start, int free_ports_end, int last_used_port){
	int index;

	// from the last used one
	index = last_used_port;
	do{
		++ index;
		// til the range end
		if(index >= free_ports_end){
			// start from the range beginning
			index = free_ports_start - 1;
			do{
				++ index;
				// til the last used one
				if(index >= last_used_port){
					// none found
					return ENOTCONN;
				}
			}while(socket_ports_find(global_sockets, index) != NULL);
			// found, break immediately
			break;
		}
	}while(socket_ports_find(global_sockets, index) != NULL);
	return socket_bind_insert(global_sockets, socket, index);
}

/** Tries to find a new free socket identifier.
 *  @param[in] local_sockets The local sockets to be searched.
 *  @param[in] positive A value indicating whether a positive identifier is requested. A negative identifier is requested if set to false.
 *	@returns The new socket identifier.
 *  @returns ELIMIT if there is no socket identifier available.
 */
static int socket_generate_new_id(socket_cores_ref local_sockets, int positive){
	int socket_id;
	int count;

	count = 0;
//	socket_id = socket_globals.last_id;
	do{
		if(count < SOCKET_ID_TRIES){
			socket_id = rand() % INT_MAX;
			++ count;
		}else if(count == SOCKET_ID_TRIES){
			socket_id = 1;
			++ count;
		// only this branch for last_id
		}else{
			if(socket_id < INT_MAX){
				++ socket_id;
/*			}else if(socket_globals.last_id){
*				socket_globals.last_id = 0;
*				socket_id = 1;
*/			}else{
				return ELIMIT;
			}
		}
	}while(socket_cores_find(local_sockets, ((positive ? 1 : -1) * socket_id)));
//	last_id = socket_id
	return socket_id;
}

int socket_create(socket_cores_ref local_sockets, int app_phone, void * specific_data, int * socket_id){
	ERROR_DECLARE;

	socket_core_ref socket;
	int res;
	int positive;

	if(! socket_id){
		return EINVAL;
	}
	// store the socket
	if(*socket_id <= 0){
		positive = (*socket_id == 0);
		*socket_id = socket_generate_new_id(local_sockets, positive);
		if(*socket_id <= 0){
			return * socket_id;
		}
		if(! positive){
			*socket_id *= -1;
		}
	}else if(socket_cores_find(local_sockets, * socket_id)){
		return EEXIST;
	}
	socket = (socket_core_ref) malloc(sizeof(*socket));
	if(! socket){
		return ENOMEM;
	}
	// initialize
	socket->phone = app_phone;
	socket->port = -1;
	socket->key = NULL;
	socket->key_length = 0;
	socket->specific_data = specific_data;
	if(ERROR_OCCURRED(dyn_fifo_initialize(&socket->received, SOCKET_INITIAL_RECEIVED_SIZE))){
		free(socket);
		return ERROR_CODE;
	}
	if(ERROR_OCCURRED(dyn_fifo_initialize(&socket->accepted, SOCKET_INITIAL_ACCEPTED_SIZE))){
		dyn_fifo_destroy(&socket->received);
		free(socket);
		return ERROR_CODE;
	}
	socket->socket_id = * socket_id;
	res = socket_cores_add(local_sockets, socket->socket_id, socket);
	if(res < 0){
		dyn_fifo_destroy(&socket->received);
		dyn_fifo_destroy(&socket->accepted);
		free(socket);
		return res;
	}
	return EOK;
}

int socket_destroy(int packet_phone, int socket_id, socket_cores_ref local_sockets, socket_ports_ref global_sockets, void (*socket_release)(socket_core_ref socket)){
	socket_core_ref socket;
	int accepted_id;

	// find the socket
	socket = socket_cores_find(local_sockets, socket_id);
	if(! socket){
		return ENOTSOCK;
	}
	// destroy all accepted sockets
	while((accepted_id = dyn_fifo_pop(&socket->accepted)) >= 0){
		socket_destroy(packet_phone, accepted_id, local_sockets, global_sockets, socket_release);
	}
	socket_destroy_core(packet_phone, socket, local_sockets, global_sockets, socket_release);
	return EOK;
}

int socket_reply_packets(packet_t packet, size_t * length){
	ERROR_DECLARE;

	packet_t next_packet;
	size_t fragments;
	size_t * lengths;
	size_t index;

	if(! length){
		return EBADMEM;
	}
	next_packet = pq_next(packet);
	if(! next_packet){
		// write all if only one fragment
		ERROR_PROPAGATE(data_reply(packet_get_data(packet), packet_get_data_length(packet)));
		// store the total length
		*length = packet_get_data_length(packet);
	}else{
		// count the packet fragments
		fragments = 1;
		next_packet = pq_next(packet);
		while((next_packet = pq_next(next_packet))){
			++ fragments;
		}
		// compute and store the fragment lengths
		lengths = (size_t *) malloc(sizeof(size_t) * fragments + sizeof(size_t));
		if(! lengths){
			return ENOMEM;
		}
		lengths[0] = packet_get_data_length(packet);
		lengths[fragments] = lengths[0];
		next_packet = pq_next(packet);
		for(index = 1; index < fragments; ++ index){
			lengths[index] = packet_get_data_length(next_packet);
			lengths[fragments] += lengths[index];
			next_packet = pq_next(packet);
		}while(next_packet);
		// write the fragment lengths
		ERROR_PROPAGATE(data_reply(lengths, sizeof(int) * (fragments + 1)));
		next_packet = packet;
		// write the fragments
		for(index = 0; index < fragments; ++ index){
			ERROR_PROPAGATE(data_reply(packet_get_data(next_packet), lengths[index]));
			next_packet = pq_next(next_packet);
		}while(next_packet);
		// store the total length
		*length = lengths[fragments];
		free(lengths);
	}
	return EOK;
}

socket_core_ref socket_port_find(socket_ports_ref global_sockets, int port, const char * key, size_t key_length){
	socket_port_ref socket_port;
	socket_core_ref * socket_ref;

	socket_port = socket_ports_find(global_sockets, port);
	if(socket_port && (socket_port->count > 0)){
		socket_ref = socket_port_map_find(&socket_port->map, key, key_length);
		if(socket_ref){
			return * socket_ref;
		}
	}
	return NULL;
}

void socket_port_release(socket_ports_ref global_sockets, socket_core_ref socket){
	socket_port_ref socket_port;
	socket_core_ref * socket_ref;

	if(socket->port){
		// find ports
		socket_port = socket_ports_find(global_sockets, socket->port);
		if(socket_port){
			// find the socket
			socket_ref = socket_port_map_find(&socket_port->map, socket->key, socket->key_length);
			if(socket_ref){
				-- socket_port->count;
				// release if empty
				if(socket_port->count <= 0){
					// destroy the map
					socket_port_map_destroy(&socket_port->map);
					// release the port
					socket_ports_exclude(global_sockets, socket->port);
				}else{
					// remove
					socket_port_map_exclude(&socket_port->map, socket->key, socket->key_length);
				}
			}
		}
		socket->port = 0;
		socket->key = NULL;
		socket->key_length = 0;
	}
}

int socket_port_add(socket_ports_ref global_sockets, int port, socket_core_ref socket, const char * key, size_t key_length){
	ERROR_DECLARE;

	socket_port_ref socket_port;

	// find ports
	socket_port = socket_ports_find(global_sockets, port);
	if(! socket_port){
		return ENOENT;
	}
	// add the socket
	ERROR_PROPAGATE(socket_port_add_core(socket_port, socket, key, key_length));
	socket->port = port;
	return EOK;
}

/** @}
 */
