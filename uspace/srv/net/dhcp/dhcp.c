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

#define NAME "dhcp"

#define MAX_MSG_SIZE 1024

static int transport_fd = -1;
static inet_link_info_t link_info;
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

/** Decode subnet mask into subnet prefix length. */
static int subnet_mask_decode(uint32_t mask, int *bits)
{
	int zbits;
	uint32_t nmask;

	if (mask == 0xffffffff) {
		*bits = 32;
		return EOK;
	}

	zbits = 1 + fnzb32(mask ^ 0xffffffff);
	nmask = BIT_RRANGE(uint32_t, zbits);

	if ((mask ^ nmask) != 0xffffffff) {
		/* The mask is not in the form 1**n,0**m */
		return EINVAL;
	}

	*bits = 32 - zbits;
	return EOK;
}

static uint32_t dhcp_uint32_decode(uint8_t *data)
{
	return
	    ((uint32_t)data[0] << 24) |
	    ((uint32_t)data[1] << 16) |
	    ((uint32_t)data[2] << 8) |
	    ((uint32_t)data[3]);
}

static int dhcp_send(void *msg, size_t size)
{
	struct sockaddr_in addr;
	int rc;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(dhcp_server_port);
	addr.sin_addr.s_addr = htonl(addr32_broadcast_all_hosts);

	rc = sendto(transport_fd, msg, size, 0,
	    (struct sockaddr *)&addr, sizeof(addr));
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Sending failed");
		return rc;
	}

	return EOK;
}

static int dhcp_send_discover(void)
{
	dhcp_hdr_t *hdr = (dhcp_hdr_t *)msgbuf;
	uint8_t *opt = msgbuf + sizeof(dhcp_hdr_t);

	memset(msgbuf, 0, MAX_MSG_SIZE);
	hdr->op = op_bootrequest;
	hdr->htype = 1; /* AHRD_ETHERNET */
	hdr->hlen = sizeof(addr48_t);
	hdr->xid = host2uint32_t_be(42);
	hdr->flags = flag_broadcast;

	addr48(link_info.mac_addr, hdr->chaddr);
	hdr->opt_magic = host2uint32_t_be(dhcp_opt_magic);

	opt[0] = opt_msg_type;
	opt[1] = 1;
	opt[2] = msg_dhcpdiscover;
	opt[3] = opt_end;

	return dhcp_send(msgbuf, sizeof(dhcp_hdr_t) + 4);
}

static int dhcp_recv_msg(void **rmsg, size_t *rsize)
{
	struct sockaddr_in src_addr;
	socklen_t src_addr_size;
	size_t recv_size;
	int rc;

	src_addr_size = sizeof(src_addr);
	rc = recvfrom(transport_fd, msgbuf, MAX_MSG_SIZE, 0,
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

static int dhcp_send_request(dhcp_offer_t *offer)
{
	dhcp_hdr_t *hdr = (dhcp_hdr_t *)msgbuf;
	uint8_t *opt = msgbuf + sizeof(dhcp_hdr_t);
	size_t i;

	memset(msgbuf, 0, MAX_MSG_SIZE);
	hdr->op = op_bootrequest;
	hdr->htype = 1; /* AHRD_ETHERNET */
	hdr->hlen = 6;
	hdr->xid = host2uint32_t_be(42);
	hdr->flags = flag_broadcast;
	hdr->ciaddr = host2uint32_t_be(offer->oaddr.addr);
	addr48(link_info.mac_addr, hdr->chaddr);
	hdr->opt_magic = host2uint32_t_be(dhcp_opt_magic);

	i = 0;

	opt[i++] = opt_msg_type;
	opt[i++] = 1;
	opt[i++] = msg_dhcprequest;

	opt[i++] = opt_req_ip_addr;
	opt[i++] = 4;
	opt[i++] = offer->oaddr.addr >> 24;
	opt[i++] = (offer->oaddr.addr >> 16) & 0xff;
	opt[i++] = (offer->oaddr.addr >> 8) & 0xff;
	opt[i++] = offer->oaddr.addr & 0xff;

	opt[i++] = opt_server_id;
	opt[i++] = 4;
	opt[i++] = offer->srv_addr.addr >> 24;
	opt[i++] = (offer->srv_addr.addr >> 16) & 0xff;
	opt[i++] = (offer->srv_addr.addr >> 8) & 0xff;
	opt[i++] = offer->srv_addr.addr & 0xff;

	opt[i++] = opt_end;

	return dhcp_send(msgbuf, sizeof(dhcp_hdr_t) + i);
}

static int dhcp_recv_reply(void *msg, size_t size, dhcp_offer_t *offer)
{
	dhcp_hdr_t *hdr = (dhcp_hdr_t *)msg;
	inet_addr_t yiaddr;
	inet_addr_t siaddr;
	inet_addr_t giaddr;
	uint32_t subnet_mask;
	bool have_subnet_mask = false;
	bool have_server_id = false;
	int subnet_bits;
	char *saddr;
	uint8_t opt_type, opt_len;
	uint8_t *msgb;
	int rc;
	size_t i;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Receive reply");
	memset(offer, 0, sizeof(*offer));

	yiaddr.family = AF_INET;
	yiaddr.addr = uint32_t_be2host(hdr->yiaddr);
	rc = inet_addr_format(&yiaddr, &saddr);
	if (rc != EOK)
		return rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Your IP address: %s", saddr);
	free(saddr);

	siaddr.family = AF_INET;
	siaddr.addr = uint32_t_be2host(hdr->siaddr);
	rc = inet_addr_format(&siaddr, &saddr);
	if (rc != EOK)
		return rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Next server IP address: %s", saddr);
	free(saddr);

	giaddr.family = AF_INET;
	giaddr.addr = uint32_t_be2host(hdr->giaddr);
	rc = inet_addr_format(&giaddr, &saddr);
	if (rc != EOK)
		return rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Relay agent IP address: %s", saddr);
	free(saddr);

	offer->oaddr.family = AF_INET;
	offer->oaddr.addr = yiaddr.addr;

	msgb = (uint8_t *)msg;

	i = sizeof(dhcp_hdr_t);
	while (i < size) {
		opt_type = msgb[i++];

		if (opt_type == opt_pad)
			continue;
		if (opt_type == opt_end)
			break;

		if (i >= size)
			return EINVAL;

		opt_len = msgb[i++];

		if (i + opt_len > size)
			return EINVAL;

		switch (opt_type) {
		case opt_subnet_mask:
			if (opt_len != 4)
				return EINVAL;
			subnet_mask = dhcp_uint32_decode(&msgb[i]);
			rc = subnet_mask_decode(subnet_mask, &subnet_bits);
			if (rc != EOK)
				return EINVAL;
			offer->oaddr.prefix = subnet_bits;
			have_subnet_mask = true;
			break;
		case opt_msg_type:
			if (opt_len != 1)
				return EINVAL;
			offer->msg_type = msgb[i];
			break;
		case opt_server_id:
			if (opt_len != 4)
				return EINVAL;
			offer->srv_addr.family = AF_INET;
			offer->srv_addr.addr = dhcp_uint32_decode(&msgb[i]);
			have_server_id = true;
			break;
		case opt_router:
			if (opt_len != 4)
				return EINVAL;
			offer->router.family = AF_INET;
			offer->router.addr = dhcp_uint32_decode(&msgb[i]);
			break;
		case opt_dns_server:
			if (opt_len != 4)
				return EINVAL;
			offer->dns_server.family = AF_INET;
			offer->dns_server.addr = dhcp_uint32_decode(&msgb[i]);
			break;
		case opt_end:
			break;
		default:
			break;
		}

		/* Advance to the next option */
		i = i + opt_len;
	}

	if (!have_server_id) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Missing server ID option.");
		return rc;
	}

	if (!have_subnet_mask) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Missing subnet mask option.");
		return rc;
	}

	rc = inet_naddr_format(&offer->oaddr, &saddr);
	if (rc != EOK)
		return rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Offered network address: %s", saddr);
	free(saddr);

	if (offer->router.addr != 0) {
		rc = inet_addr_format(&offer->router, &saddr);
		if (rc != EOK)
			return rc;

		log_msg(LOG_DEFAULT, LVL_DEBUG, "Router address: %s", saddr);
		free(saddr);
	}

	if (offer->dns_server.addr != 0) {
		rc = inet_addr_format(&offer->dns_server, &saddr);
		if (rc != EOK)
			return rc;

		log_msg(LOG_DEFAULT, LVL_DEBUG, "DNS server: %s", saddr);
		free(saddr);
	}

	return EOK;
}

static int dhcp_cfg_create(service_id_t iplink, dhcp_offer_t *offer)
{
	int rc;
	service_id_t addr_id;
	service_id_t sroute_id;
	inet_naddr_t defr;

	rc = inetcfg_addr_create_static("dhcp4a", &offer->oaddr, iplink,
	    &addr_id);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Error creating IP address %s (%d)", "dhcp4a", rc);
		return rc;
	}

	if (offer->router.addr != 0) {
		defr.family = AF_INET;
		defr.addr = 0;
		defr.prefix = 0;

		rc = inetcfg_sroute_create("dhcpdef", &defr, &offer->router, &sroute_id);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR, "Error creating "
			    "default route %s (%d).", "dhcpdef", rc);
			return rc;
		}
	}

	if (offer->dns_server.addr != 0) {
		rc = dnsr_set_srvaddr(&offer->dns_server);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR, "%s: Error setting "
			    "nameserver address (%d))", NAME, rc);
			return rc;
		}
	}

	return EOK;
}

int dhcpsrv_link_add(service_id_t link_id)
{
	int fd;
	struct sockaddr_in laddr;
	void *msg;
	size_t msg_size;
	dhcp_offer_t offer;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "dhcpsrv_link_add(%zu)", link_id);

	/* Get link hardware address */
	rc = inetcfg_link_get(link_id, &link_info);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error getting properties "
		    "for link %zu.", link_id);
		return EIO;
	}

	laddr.sin_family = AF_INET;
	laddr.sin_port = htons(dhcp_client_port);
	laddr.sin_addr.s_addr = INADDR_ANY;

	fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0)
		return EIO;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Bind socket.");
	rc = bind(fd, (struct sockaddr *)&laddr, sizeof(laddr));
	if (rc != EOK)
		return EIO;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Set socket options");
	rc = setsockopt(fd, SOL_SOCKET, SO_IPLINK, &link_id, sizeof(link_id));
	if (rc != EOK)
		return EIO;

	transport_fd = fd;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Send DHCPDISCOVER");
	rc = dhcp_send_discover();
	if (rc != EOK)
		return EIO;

	rc = dhcp_recv_msg(&msg, &msg_size);
	if (rc != EOK)
		return EIO;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Received %zu bytes", msg_size);

	rc = dhcp_recv_reply(msg, msg_size, &offer);
	if (rc != EOK)
		return EIO;

	rc = dhcp_send_request(&offer);
	if (rc != EOK)
		return EIO;

	rc = dhcp_recv_msg(&msg, &msg_size);
	if (rc != EOK)
		return EIO;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Received %zu bytes", msg_size);

	rc = dhcp_recv_reply(msg, msg_size, &offer);
	if (rc != EOK)
		return EIO;

	rc = dhcp_cfg_create(link_id, &offer);
	if (rc != EOK)
		return EIO;

	closesocket(fd);
	return EOK;
}

int dhcpsrv_link_remove(service_id_t link_id)
{
	return ENOTSUP;
}


/** @}
 */
