/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
