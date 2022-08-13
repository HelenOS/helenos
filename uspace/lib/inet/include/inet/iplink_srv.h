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

#ifndef LIBINET_INET_IPLINK_SRV_H
#define LIBINET_INET_IPLINK_SRV_H

#include <async.h>
#include <fibril_synch.h>
#include <inet/addr.h>
#include <inet/eth_addr.h>
#include <inet/iplink.h>
#include <stdbool.h>

struct iplink_ops;

typedef struct {
	fibril_mutex_t lock;
	bool connected;
	struct iplink_ops *ops;
	void *arg;
	async_sess_t *client_sess;
} iplink_srv_t;

typedef struct iplink_ops {
	errno_t (*open)(iplink_srv_t *);
	errno_t (*close)(iplink_srv_t *);
	errno_t (*send)(iplink_srv_t *, iplink_sdu_t *);
	errno_t (*send6)(iplink_srv_t *, iplink_sdu6_t *);
	errno_t (*get_mtu)(iplink_srv_t *, size_t *);
	errno_t (*get_mac48)(iplink_srv_t *, eth_addr_t *);
	errno_t (*set_mac48)(iplink_srv_t *, eth_addr_t *);
	errno_t (*addr_add)(iplink_srv_t *, inet_addr_t *);
	errno_t (*addr_remove)(iplink_srv_t *, inet_addr_t *);
} iplink_ops_t;

extern void iplink_srv_init(iplink_srv_t *);

extern errno_t iplink_conn(ipc_call_t *, void *);
extern errno_t iplink_ev_recv(iplink_srv_t *, iplink_recv_sdu_t *, ip_ver_t);
extern errno_t iplink_ev_change_addr(iplink_srv_t *, eth_addr_t *);

#endif

/** @}
 */
