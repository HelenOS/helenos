/*
 * Copyright (c) 2013 Jiri Svoboda
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

/** @addtogroup dhcp
 * @{
 */
/**
 * @file
 * @brief DHCP client
 */

#include <bitops.h>
#include <errno.h>
#include <inet/addr.h>
#include <inet/dnsr.h>
#include <inet/inetcfg.h>
#include <io/log.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>

#include "dhcp.h"
#include "dhcp_std.h"
#include "transport.h"

#define MAX_MSG_SIZE 1024
static uint8_t msgbuf[MAX_MSG_SIZE];

typedef struct {
	/** Message type */
	enum dhcp_msg_type msg_type;
	/** Offered address */
	inet_naddr_t oaddr;
	/** Server address */
	inet_addr_t srv_addr;
	/** Router address */
	inet_addr_t router;
	/** DNS server */
	inet_addr_t dns_server;
} dhcp_offer_t;

static void dhcp_recv_msg(udp_assoc_t *, udp_rmsg_t *);
static void dhcp_recv_err(udp_assoc_t *, udp_rerr_t *);
static void dhcp_link_state(udp_assoc_t *, udp_link_state_t);

static udp_cb_t dhcp_transport_cb = {
	.recv_msg = dhcp_recv_msg,
	.recv_err = dhcp_recv_err,
	.link_state = dhcp_link_state
};

errno_t dhcp_send(dhcp_transport_t *dt, void *msg, size_t size)
{
	inet_ep_t ep;
	errno_t rc;

	inet_ep_init(&ep);
	ep.port = dhcp_server_port;
	inet_addr_set(addr32_broadcast_all_hosts, &ep.addr);

	rc = udp_assoc_send_msg(dt->assoc, &ep, msg, size);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed sending message");
		return rc;
	}

	return EOK;
}

static void dhcp_recv_msg(udp_assoc_t *assoc, udp_rmsg_t *rmsg)
{
	dhcp_transport_t *dt;
	size_t s;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "dhcp_recv_msg()");

	dt = (dhcp_transport_t *)udp_assoc_userptr(assoc);
	s = udp_rmsg_size(rmsg);
	if (s > MAX_MSG_SIZE)
		s = MAX_MSG_SIZE; /* XXX */

	rc = udp_rmsg_read(rmsg, 0, msgbuf, s);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error receiving message.");
		return;
	}

	log_msg(LOG_DEFAULT, LVL_NOTE, "dhcp_recv_msg() - call recv_cb");
	dt->recv_cb(dt->cb_arg, msgbuf, s);
}

static void dhcp_recv_err(udp_assoc_t *assoc, udp_rerr_t *rerr)
{
	log_msg(LOG_DEFAULT, LVL_WARN, "Ignoring ICMP error");
}

static void dhcp_link_state(udp_assoc_t *assoc, udp_link_state_t ls)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "Link state change");
}

errno_t dhcp_transport_init(dhcp_transport_t *dt, service_id_t link_id,
    dhcp_recv_cb_t recv_cb, void *arg)
{
	udp_t *udp = NULL;
	udp_assoc_t *assoc = NULL;
	inet_ep2_t epp;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "dhcp_transport_init()");

	inet_ep2_init(&epp);
	epp.local.addr.version = ip_v4;
	epp.local.port = dhcp_client_port;
	epp.local_link = link_id;

	rc = udp_create(&udp);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	rc = udp_assoc_create(udp, &epp, &dhcp_transport_cb, dt, &assoc);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	rc = udp_assoc_set_nolocal(assoc);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	dt->udp = udp;
	dt->assoc = assoc;
	dt->recv_cb = recv_cb;
	dt->cb_arg = arg;

	return EOK;
error:
	udp_assoc_destroy(assoc);
	udp_destroy(udp);
	return rc;
}

void dhcp_transport_fini(dhcp_transport_t *dt)
{
	udp_assoc_destroy(dt->assoc);
	udp_destroy(dt->udp);
}

/** @}
 */
