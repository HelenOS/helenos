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
/** @file UDP type definitions
 */

#ifndef UDP_TYPE_H
#define UDP_TYPE_H

#include <async.h>
#include <errno.h>
#include <fibril.h>
#include <fibril_synch.h>
#include <inet/endpoint.h>
#include <ipc/loc.h>
#include <refcount.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <inet/addr.h>

#define UDP_FRAGMENT_SIZE 65535

/** UDP error codes */
typedef enum {
	UDP_EOK,
	/* Insufficient resources */
	UDP_ENORES,
	/* Remote endpoint unspecified */
	UDP_EUNSPEC,
	/* No route to destination */
	UDP_ENOROUTE,
	/** Association reset by user */
	UDP_ERESET
} udp_error_t;

typedef enum {
	XF_DUMMY = 0x1
} xflags_t;

/** Unencoded UDP message (datagram) */
typedef struct {
	/** Message data */
	void *data;
	/** Message data size */
	size_t data_size;
} udp_msg_t;

/** Encoded PDU */
typedef struct {
	/** IP link (optional) */
	service_id_t iplink;
	/** Source address */
	inet_addr_t src;
	/** Destination address */
	inet_addr_t dest;
	/** Encoded PDU data including header */
	void *data;
	/** Encoded PDU data size */
	size_t data_size;
} udp_pdu_t;

/** Functions needed by associations module.
 *
 * Functions that need to be provided by the caller so that the associations
 * module can function.
 */
typedef struct {
	errno_t (*get_srcaddr)(inet_addr_t *, uint8_t, inet_addr_t *);
	errno_t (*transmit_msg)(inet_ep2_t *, udp_msg_t *);
} udp_assocs_dep_t;

/** Association callbacks.
 *
 * Callbacks for a particular association, to notify caller of events
 * on the association.
 */
typedef struct {
	/** Message received */
	void (*recv_msg)(void *, inet_ep2_t *, udp_msg_t *);
} udp_assoc_cb_t;

/** UDP association
 *
 * This is a rough equivalent of a TCP connection endpoint. It allows
 * sending and receiving UDP datagrams and it is uniquely identified
 * by an endpoint pair.
 */
typedef struct {
	char *name;
	link_t link;

	/** Association identification (endpoint pair) */
	inet_ep2_t ident;

	/** True if association was reset by user */
	bool reset;

	/** True if association was deleted by user */
	bool deleted;

	/** Protects access to association structure */
	fibril_mutex_t lock;
	/** Reference count */
	atomic_refcount_t refcnt;

	/** Receive queue */
	list_t rcv_queue;
	/** Receive queue CV. Broadcast when new datagram is inserted */
	fibril_condvar_t rcv_queue_cv;
	/** Allow sending messages with no local address */
	bool nolocal;

	udp_assoc_cb_t *cb;
	void *cb_arg;
} udp_assoc_t;

typedef struct {
} udp_assoc_status_t;

/** UDP receive queue entry */
typedef struct {
	/** Link to receive queue */
	link_t link;
	/** Endpoint pair */
	inet_ep2_t epp;
	/** Message */
	udp_msg_t *msg;
} udp_rcv_queue_entry_t;

/** UDP client association.
 *
 * Ties a UDP association into the namespace of a client
 */
typedef struct udp_cassoc {
	/** Association */
	udp_assoc_t *assoc;
	/** Association ID for the client */
	sysarg_t id;
	/** Client */
	struct udp_client *client;
	link_t lclient;
} udp_cassoc_t;

/** UDP client receive queue entry */
typedef struct {
	/** Link to receive queue */
	link_t link;
	/** Endpoint pair */
	inet_ep2_t epp;
	/** Message */
	udp_msg_t *msg;
	/** Client association */
	udp_cassoc_t *cassoc;
} udp_crcv_queue_entry_t;

/** UDP client */
typedef struct udp_client {
	/** Client callback session */
	async_sess_t *sess;
	/** Client assocations */
	list_t cassoc; /* of udp_cassoc_t */
	/** Client receive queue */
	list_t crcv_queue;
} udp_client_t;

#endif

/** @}
 */
