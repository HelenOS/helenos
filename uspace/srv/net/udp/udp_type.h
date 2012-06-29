/*
 * Copyright (c) 2012 Jiri Svoboda
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
/** @file UDP type definitions
 */

#ifndef UDP_TYPE_H
#define UDP_TYPE_H

#include <fibril.h>
#include <fibril_synch.h>
#include <socket_core.h>
#include <sys/types.h>

#define UDP_FRAGMENT_SIZE 4096


typedef enum {
	UDP_EOK,
	/* Insufficient resources */
	UDP_ENORES,
	/* Foreign socket unspecified */
	UDP_EUNSPEC,
	/* No route to destination */
	UDP_ENOROUTE
} udp_error_t;

typedef enum {
	XF_DUMMY	= 0x1
} xflags_t;

typedef struct {
	uint32_t ipv4;
} netaddr_t;

enum netaddr {
	UDP_IPV4_ANY = 0
};

enum tcp_port {
	UDP_PORT_ANY = 0
};

typedef struct {
	netaddr_t addr;
	uint16_t port;
} udp_sock_t;

typedef struct {
	udp_sock_t local;
	udp_sock_t foreign;
} udp_sockpair_t;

/** Unencoded UDP message (datagram) */
typedef struct {
	/** Message data */
	void *data;
	/** Message data size */
	size_t data_size;
} udp_msg_t;

/** Encoded PDU */
typedef struct {
	/** Source address */
	netaddr_t src;
	/** Destination address */
	netaddr_t dest;

	/** Encoded PDU data including header */
	void *data;
	/** Encoded PDU data size */
	size_t data_size;
} udp_pdu_t;

typedef struct {
	async_sess_t *sess;
	socket_cores_t sockets;
} udp_client_t;

/** UDP association
 *
 * This is a rough equivalent of a TCP connection endpoint. It allows
 * sending and receiving UDP datagrams and it is uniquely identified
 * by a socket pair.
 */
typedef struct {
	char *name;
	link_t link;

	/** Association identification (local and foreign socket) */
	udp_sockpair_t ident;

	/** True if association was deleted by user */
	bool deleted;

	/** Protects access to association structure */
	fibril_mutex_t lock;
	/** Reference count */
	atomic_t refcnt;

	/** Receive queue */
	list_t rcv_queue;
	/** Receive queue CV. Broadcast when new datagram is inserted */
	fibril_condvar_t rcv_queue_cv;
} udp_assoc_t;

typedef struct {
} udp_assoc_status_t;

typedef struct udp_sockdata {
	/** Lock */
	fibril_mutex_t lock;
	/** Socket core */
	socket_core_t *sock_core;
	/** Client */
	udp_client_t *client;
	/** Connection */
	udp_assoc_t *assoc;
	/** Receiving fibril */
	fid_t recv_fibril;
	uint8_t recv_buffer[UDP_FRAGMENT_SIZE];
	size_t recv_buffer_used;
	udp_sock_t recv_fsock;
	fibril_mutex_t recv_buffer_lock;
	fibril_condvar_t recv_buffer_cv;
	udp_error_t recv_error;
} udp_sockdata_t;

typedef struct {
	/** Link to receive queue */
	link_t link;
	/** Socket pair */
	udp_sockpair_t sp;
	/** Message */
	udp_msg_t *msg;
} udp_rcv_queue_entry_t;

#endif

/** @}
 */
