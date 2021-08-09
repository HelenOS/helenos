/*
 * Copyright (c) 2021 Jiri Svoboda
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

/** @addtogroup libinet
 * @{
 */
/** @file
 */

#ifndef LIBINET_INET_IPLINK_H
#define LIBINET_INET_IPLINK_H

#include <async.h>
#include <inet/addr.h>
#include <inet/eth_addr.h>

struct iplink_ev_ops;

typedef struct {
	async_sess_t *sess;
	struct iplink_ev_ops *ev_ops;
	void *arg;
} iplink_t;

/** IPv4 link Service Data Unit */
typedef struct {
	/** Local source address */
	addr32_t src;
	/** Local destination address */
	addr32_t dest;
	/** Serialized IP packet */
	void *data;
	/** Size of @c data in bytes */
	size_t size;
} iplink_sdu_t;

/** IPv6 link Service Data Unit */
typedef struct {
	/** Local MAC destination address */
	eth_addr_t dest;
	/** Serialized IP packet */
	void *data;
	/** Size of @c data in bytes */
	size_t size;
} iplink_sdu6_t;

/** Internet link receive Service Data Unit */
typedef struct {
	/** Serialized datagram */
	void *data;
	/** Size of @c data in bytes */
	size_t size;
} iplink_recv_sdu_t;

typedef struct iplink_ev_ops {
	errno_t (*recv)(iplink_t *, iplink_recv_sdu_t *, ip_ver_t);
	errno_t (*change_addr)(iplink_t *, eth_addr_t *);
} iplink_ev_ops_t;

extern errno_t iplink_open(async_sess_t *, iplink_ev_ops_t *, void *, iplink_t **);
extern void iplink_close(iplink_t *);
extern errno_t iplink_send(iplink_t *, iplink_sdu_t *);
extern errno_t iplink_send6(iplink_t *, iplink_sdu6_t *);
extern errno_t iplink_addr_add(iplink_t *, inet_addr_t *);
extern errno_t iplink_addr_remove(iplink_t *, inet_addr_t *);
extern errno_t iplink_get_mtu(iplink_t *, size_t *);
extern errno_t iplink_get_mac48(iplink_t *, eth_addr_t *);
extern errno_t iplink_set_mac48(iplink_t *, eth_addr_t *);
extern void *iplink_get_userptr(iplink_t *);

#endif

/** @}
 */
