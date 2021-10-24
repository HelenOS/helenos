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
