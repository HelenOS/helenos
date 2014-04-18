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
#include <inet/addr.h>
#include <inet/dnsr.h>
#include <inet/inetcfg.h>
#include <io/log.h>
#include <loc.h>
#include <net/in.h>
#include <net/inet.h>
#include <net/socket.h>
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

static int dhcp_recv_fibril(void *);

int dhcp_send(dhcp_transport_t *dt, void *msg, size_t size)
{
	struct sockaddr_in addr;
	int rc;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(dhcp_server_port);
	addr.sin_addr.s_addr = htonl(addr32_broadcast_all_hosts);

	rc = sendto(dt->fd, msg, size, 0,
	    (struct sockaddr *)&addr, sizeof(addr));
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Sending failed");
		return rc;
	}

	return EOK;
}

static int dhcp_recv_msg(dhcp_transport_t *dt, void **rmsg, size_t *rsize)
{
	struct sockaddr_in src_addr;
	socklen_t src_addr_size;
	size_t recv_size;
	int rc;

	src_addr_size = sizeof(src_addr);
	rc = recvfrom(dt->fd, msgbuf, MAX_MSG_SIZE, 0,
	    (struct sockaddr *)&src_addr, &src_addr_size);
	if (rc < 0) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "recvfrom failed (%d)", rc);
		return rc;
	}

	recv_size = (size_t)rc;
	*rmsg = msgbuf;
	*rsize = recv_size;

	return EOK;
}

int dhcp_transport_init(dhcp_transport_t *dt, service_id_t link_id,
    dhcp_recv_cb_t recv_cb, void *arg)
{
	int fd;
	struct sockaddr_in laddr;
	int fid;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "dhcptransport_init()");

	laddr.sin_family = AF_INET;
	laddr.sin_port = htons(dhcp_client_port);
	laddr.sin_addr.s_addr = INADDR_ANY;

	fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0) {
		rc = EIO;
		goto error;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Bind socket.");
	rc = bind(fd, (struct sockaddr *)&laddr, sizeof(laddr));
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Set socket options");
	rc = setsockopt(fd, SOL_SOCKET, SO_IPLINK, &link_id, sizeof(link_id));
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	dt->fd = fd;
	dt->recv_cb = recv_cb;
	dt->cb_arg = arg;

	fid = fibril_create(dhcp_recv_fibril, dt);
	if (fid == 0) {
		rc = ENOMEM;
		goto error;
	}

	dt->recv_fid = fid;
	fibril_add_ready(fid);

	return EOK;
error:
	closesocket(fd);
	return rc;
}

void dhcp_transport_fini(dhcp_transport_t *dt)
{
	closesocket(dt->fd);
}

static int dhcp_recv_fibril(void *arg)
{
	dhcp_transport_t *dt = (dhcp_transport_t *)arg;
	void *msg;
	size_t size = (size_t) -1;
	int rc;

	while (true) {
		rc = dhcp_recv_msg(dt, &msg, &size);
		if (rc != EOK)
			break;

		assert(size != (size_t) -1);

		dt->recv_cb(dt->cb_arg, msg, size);
	}

	return EOK;
}

/** @}
 */
