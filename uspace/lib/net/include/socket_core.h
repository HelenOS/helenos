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
 *  @{
 */

/** @file
 * Socket common core.
 */

#ifndef LIBNET_SOCKET_CORE_H_
#define LIBNET_SOCKET_CORE_H_

#include <sys/types.h>
#include <adt/generic_char_map.h>
#include <adt/dynamic_fifo.h>
#include <adt/int_map.h>
#include <net/in.h>
#include <async.h>

/** Initial size of the received packet queue. */
#define SOCKET_INITIAL_RECEIVED_SIZE  4

/** Maximum size of the received packet queue. */
#define SOCKET_MAX_RECEIVED_SIZE  0

/** Initial size of the sockets for acceptance queue. */
#define SOCKET_INITIAL_ACCEPTED_SIZE  1

/** Maximum size of the sockets for acceptance queue. */
#define SOCKET_MAX_ACCEPTEDED_SIZE  0

/** Listening sockets' port map key. */
#define SOCKET_MAP_KEY_LISTENING  "L"

/** Type definition of the socket core.
 * @see socket_core
 */
typedef struct socket_core socket_core_t;

/** Type definition of the socket port.
 * @see socket_port
 */
typedef struct socket_port socket_port_t;

/** Socket core. */
struct socket_core {
	/** Socket identifier. */
	int socket_id;
	/** Client application session. */
	async_sess_t *sess;
	/** Bound port. */
	int port;
	/** Sockets for acceptance queue. */
	dyn_fifo_t accepted;
	/** Protocol specific data. */
	void *specific_data;
	/** Socket ports map key. */
	const uint8_t *key;
	/** Length of the Socket ports map key. */
	size_t key_length;
};

/** Sockets map.
 * The key is the socket identifier.
 */
INT_MAP_DECLARE(socket_cores, socket_core_t);

/** Bount port sockets map.
 *
 * The listening socket has the SOCKET_MAP_KEY_LISTENING key identifier whereas
 * the other use the remote addresses.
 */
GENERIC_CHAR_MAP_DECLARE(socket_port_map, socket_core_t *);

/** Ports map.
 * The key is the port number.
 */
INT_MAP_DECLARE(socket_ports, socket_port_t);

extern void socket_cores_release(async_sess_t *, socket_cores_t *,
    socket_ports_t *, void (*)(socket_core_t *));
extern int socket_bind(socket_cores_t *, socket_ports_t *, int, void *, size_t,
    int, int, int);
extern int socket_bind_free_port(socket_ports_t *, socket_core_t *, int, int,
    int);
extern int socket_create(socket_cores_t *, async_sess_t *, void *, int *);
extern int socket_destroy(async_sess_t *, int, socket_cores_t *,
    socket_ports_t *, void (*)(socket_core_t *));
extern socket_core_t *socket_port_find(socket_ports_t *, int, const uint8_t *,
    size_t);
extern void socket_port_release(socket_ports_t *, socket_core_t *);
extern int socket_port_add(socket_ports_t *, int, socket_core_t *,
    const uint8_t *, size_t);

#endif

/** @}
 */
