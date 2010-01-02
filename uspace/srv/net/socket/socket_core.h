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
 *  Socket common core.
 */

#ifndef __NET_SOCKET_CORE_H__
#define __NET_SOCKET_CORE_H__

#include <sys/types.h>

#include "../include/in.h"
#include "../include/device.h"

#include "../structures/generic_char_map.h"
#include "../structures/dynamic_fifo.h"
#include "../structures/int_map.h"
#include "../structures/packet/packet.h"

/** Initial size of the received packet queue.
 */
#define SOCKET_INITIAL_RECEIVED_SIZE	4

/** Maximum size of the received packet queue.
 */
#define SOCKET_MAX_RECEIVED_SIZE		0

/** Initial size of the sockets for acceptance queue.
 */
#define SOCKET_INITIAL_ACCEPTED_SIZE	1

/** Maximum size of the sockets for acceptance queue.
 */
#define SOCKET_MAX_ACCEPTEDED_SIZE		0

/** Listening sockets' port map key.
 */
#define SOCKET_MAP_KEY_LISTENING	"L"

/** Type definition of the socket core.
 *  @see socket_core
 */
typedef struct socket_core	socket_core_t;

/** Type definition of the socket core pointer.
 *  @see socket_core
 */
typedef socket_core_t *	socket_core_ref;

/** Type definition of the socket port.
 *  @see socket_port
 */
typedef struct socket_port	socket_port_t;

/** Type definition of the socket port pointer.
 *  @see socket_port
 */
typedef socket_port_t *	socket_port_ref;

/** Socket core.
 */
struct socket_core{
	/** Socket identifier.
	 */
	int				socket_id;
	/** Client application phone.
	 */
	int				phone;
	/** Bound port.
	 */
	int				port;
	/** Received packets queue.
	 */
	dyn_fifo_t		received;
	/** Sockets for acceptance queue.
	 */
	dyn_fifo_t		accepted;
	/** Protocol specific data.
	 */
	void *			specific_data;
	/** Socket ports map key.
	 */
	const char *	key;
	/** Length of the Socket ports map key.
	 */
	size_t			key_length;
};

/** Sockets map.
 *  The key is the socket identifier.
 */
INT_MAP_DECLARE( socket_cores, socket_core_t );

/** Bount port sockets map.
 *  The listening socket has the SOCKET_MAP_KEY_LISTENING key identifier whereas the other use the remote addresses.
 */
GENERIC_CHAR_MAP_DECLARE( socket_port_map, socket_core_ref );

/** Ports map.
 *  The key is the port number.
 */
INT_MAP_DECLARE( socket_ports, socket_port_t );

/** Destroys local sockets.
 *  Releases all buffered packets and calls the release function for each of the sockets.
 *  @param[in] packet_phone The packet server phone to release buffered packets.
 *  @param[in] local_sockets The local sockets to be destroyed.
 *  @param[in,out] global_sockets The global sockets to be updated.
 *  @param[in] socket_release The client release callback function.
 */
void	socket_cores_release( int packet_phone, socket_cores_ref local_sockets, socket_ports_ref global_sockets, void ( * socket_release )( socket_core_ref socket ));

/** Binds the socket to the port.
 *  The address port is used if set, a free port is used if not.
 *  @param[in] local_sockets The local sockets to be searched.
 *  @param[in,out] global_sockets The global sockets to be updated.
 *  @param[in] socket_id The new socket identifier.
 *  @param[in] addr The address to be bound to.
 *  @param[in] addrlen The address length.
 *  @param[in] free_ports_start The minimum free port.
 *  @param[in] free_ports_end The maximum free port.
 *  @param[in] last_used_port The last used free port.
 *  @returns EOK on success.
 *  @returns ENOTSOCK if the socket was not found.
 *  @returns EAFNOSUPPORT if the address family is not supported.
 *  @returns EADDRINUSE if the port is already in use.
 *  @returns Other error codes as defined for the socket_bind_free_port() function.
 *  @returns Other error codes as defined for the socket_bind_insert() function.
 */
int socket_bind( socket_cores_ref local_sockets, socket_ports_ref global_sockets, int socket_id, void * addr, size_t addrlen, int free_ports_start, int free_ports_end, int last_used_port );

/** Binds the socket to a free port.
 *  The first free port is used.
 *  @param[in,out] global_sockets The global sockets to be updated.
 *  @param[in,out] socket The socket to be bound.
 *  @param[in] free_ports_start The minimum free port.
 *  @param[in] free_ports_end The maximum free port.
 *  @param[in] last_used_port The last used free port.
 *  @returns EOK on success.
 *  @returns ENOTCONN if no free port was found.
 *  @returns Other error codes as defined for the socket_bind_insert() function.
 */
int	socket_bind_free_port( socket_ports_ref global_sockets, socket_core_ref socket, int free_ports_start, int free_ports_end, int last_used_port );

/** Creates a new socket.
 *  @param[in,out] local_sockets The local sockets to be updated.
 *  @param[in] app_phone The application phone.
 *  @param[in] specific_data The socket specific data.
 *  @param[out] socket_id The new socket identifier.
 *  @returns EOK on success.
 *  @returns EBADMEM if the socket_id parameter is NULL.
 *  @returns ENOMEM if there is not enough memory left.
 */
int socket_create( socket_cores_ref local_sockets, int app_phone, void * specific_data, int * socket_id );

/** Destroys the socket.
 *  If the socket is bound, the port is released.
 *  Releases all buffered packets, calls the release function and removes the socket from the local sockets.
 *  @param[in] packet_phone The packet server phone to release buffered packets.
 *  @param[in] socket_id The socket identifier.
 *  @param[in,out] local_sockets The local sockets to be updated.
 *  @param[in,out] global_sockets The global sockets to be updated.
 *  @param[in] socket_release The client release callback function.
 *  @returns EOK on success.
 *  @returns ENOTSOCK if the socket is not found.
 */
int socket_destroy( int packet_phone, int socket_id, socket_cores_ref local_sockets, socket_ports_ref global_sockets, void ( * socket_release )( socket_core_ref socket ));

/** Replies the packet or the packet queue data to the application via the socket.
 *  Uses the current message processing fibril.
 *  @param[in] packet The packet to be transfered.
 *  @param[out] length The total data length.
 *  @returns EOK on success.
 *  @returns EBADMEM if the length parameter is NULL.
 *  @returns ENOMEM if there is not enough memory left.
 *  @returns Other error codes as defined for the data_reply() function.
 */
int	socket_reply_packets( packet_t packet, size_t * length );

/** Finds the bound port socket.
 *  @param[in] global_sockets The global sockets to be searched.
 *  @param[in] port The port number.
 *  @param[in] key The socket key identifier.
 *  @param[in] key_length The socket key length.
 *  @returns The found socket.
 *  @returns NULL if no socket was found.
 */
socket_core_ref	socket_port_find( socket_ports_ref global_sockets, int port, const char * key, size_t key_length );

/** Releases the socket port.
 *  If the socket is bound the port entry is released.
 *  If there are no more port entries the port is release.
 *  @param[in] global_sockets The global sockets to be updated.
 *  @param[in] socket The socket to be unbound.
 */
void	socket_port_release( socket_ports_ref global_sockets, socket_core_ref socket );

/** Adds the socket to an already bound port.
 *  @param[in] global_sockets The global sockets to be updated.
 *  @param[in] port The port number to be bound to.
 *  @param[in] socket The socket to be added.
 *  @param[in] key The socket key identifier.
 *  @param[in] key_length The socket key length.
 *  @returns EOK on success.
 *  @returns ENOENT if the port is not already used.
 *  @returns Other error codes as defined for the socket_port_add_core() function.
 */
int	socket_port_add( socket_ports_ref global_sockets, int port, socket_core_ref socket, const char * key, size_t key_length );

#endif

/** @}
 */
