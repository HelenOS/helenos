/*
 * Copyright (c) 2024 Jiri Svoboda
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

#include <adt/list.h>
#include <bitops.h>
#include <byteorder.h>
#include <errno.h>
#include <str_error.h>
#include <fibril_synch.h>
#include <inet/addr.h>
#include <inet/eth_addr.h>
#include <inet/dnsr.h>
#include <inet/inetcfg.h>
#include <io/log.h>
#include <loc.h>
#include <rndgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>

#include "dhcp.h"
#include "dhcp_std.h"
#include "transport.h"

enum {
	/** In microseconds */
	dhcp_discover_timeout_val = 5 * 1000 * 1000,
	/** In microseconds */
	dhcp_request_timeout_val = 1 * 1000 * 1000,
	dhcp_discover_retries = 5,
	dhcp_request_retries = 3
};

#define MAX_MSG_SIZE 1024
static uint8_t msgbuf[MAX_MSG_SIZE];

/** List of registered links (of dhcp_link_t) */
static list_t dhcp_links;

bool inetcfg_inited = false;

static void dhcpsrv_discover_timeout(void *);
static void dhcpsrv_request_timeout(void *);

typedef enum {
	ds_bound,
	ds_fail,
	ds_init,
	ds_init_reboot,
	ds_rebinding,
	ds_renewing,
	ds_requesting,
	ds_selecting
} dhcp_state_t;

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
	/** Transaction ID */
	uint32_t xid;
} dhcp_offer_t;

typedef struct {
	/** Link to dhcp_links list */
	link_t links;
	/** Link service ID */
	service_id_t link_id;
	/** Link info */
	inet_link_info_t link_info;
	/** Transport */
	dhcp_transport_t dt;
	/** Transport timeout */
	fibril_timer_t *timeout;
	/** Number of retries */
	int retries_left;
	/** Link state */
	dhcp_state_t state;
	/** Last received offer */
	dhcp_offer_t offer;
	/** Random number generator */
	rndgen_t *rndgen;
} dhcp_link_t;

static void dhcpsrv_recv(void *, void *, size_t);

/** Decode subnet mask into subnet prefix length. */
static errno_t subnet_mask_decode(uint32_t mask, int *bits)
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

static errno_t dhcp_send_discover(dhcp_link_t *dlink)
{
	dhcp_hdr_t *hdr = (dhcp_hdr_t *)msgbuf;
	uint8_t *opt = msgbuf + sizeof(dhcp_hdr_t);
	uint32_t xid;
	errno_t rc;
	size_t i;

	rc = rndgen_uint32(dlink->rndgen, &xid);
	if (rc != EOK)
		return rc;

	memset(msgbuf, 0, MAX_MSG_SIZE);
	hdr->op = op_bootrequest;
	hdr->htype = 1; /* AHRD_ETHERNET */
	hdr->hlen = ETH_ADDR_SIZE;
	hdr->xid = host2uint32_t_be(xid);
	hdr->flags = host2uint16_t_be(flag_broadcast);

	eth_addr_encode(&dlink->link_info.mac_addr, hdr->chaddr);
	hdr->opt_magic = host2uint32_t_be(dhcp_opt_magic);

	i = 0;

	opt[i++] = opt_msg_type;
	opt[i++] = 1;
	opt[i++] = msg_dhcpdiscover;

	opt[i++] = opt_param_req_list;
	opt[i++] = 3;
	opt[i++] = 1; /* subnet mask */
	opt[i++] = 6; /* DNS server */
	opt[i++] = 3; /* router */

	opt[i++] = opt_end;

	return dhcp_send(&dlink->dt, msgbuf, sizeof(dhcp_hdr_t) + i);
}

static errno_t dhcp_send_request(dhcp_link_t *dlink, dhcp_offer_t *offer)
{
	dhcp_hdr_t *hdr = (dhcp_hdr_t *)msgbuf;
	uint8_t *opt = msgbuf + sizeof(dhcp_hdr_t);
	size_t i;

	memset(msgbuf, 0, MAX_MSG_SIZE);
	hdr->op = op_bootrequest;
	hdr->htype = 1; /* AHRD_ETHERNET */
	hdr->hlen = 6;
	hdr->xid = host2uint32_t_be(offer->xid);
	hdr->flags = host2uint16_t_be(flag_broadcast);
	eth_addr_encode(&dlink->link_info.mac_addr, hdr->chaddr);
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

	return dhcp_send(&dlink->dt, msgbuf, sizeof(dhcp_hdr_t) + i);
}

static errno_t dhcp_parse_reply(void *msg, size_t size, dhcp_offer_t *offer)
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
	errno_t rc;
	size_t i;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Receive reply");
	memset(offer, 0, sizeof(*offer));

	inet_addr_set(uint32_t_be2host(hdr->yiaddr), &yiaddr);
	rc = inet_addr_format(&yiaddr, &saddr);
	if (rc != EOK)
		return rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Your IP address: %s", saddr);
	free(saddr);

	inet_addr_set(uint32_t_be2host(hdr->siaddr), &siaddr);
	rc = inet_addr_format(&siaddr, &saddr);
	if (rc != EOK)
		return rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Next server IP address: %s", saddr);
	free(saddr);

	inet_addr_set(uint32_t_be2host(hdr->giaddr), &giaddr);
	rc = inet_addr_format(&giaddr, &saddr);
	if (rc != EOK)
		return rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Relay agent IP address: %s", saddr);
	free(saddr);

	inet_naddr_set(yiaddr.addr, 0, &offer->oaddr);
	offer->xid = uint32_t_be2host(hdr->xid);

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
			inet_addr_set(dhcp_uint32_decode(&msgb[i]),
			    &offer->srv_addr);
			have_server_id = true;
			break;
		case opt_router:
			if (opt_len != 4)
				return EINVAL;
			inet_addr_set(dhcp_uint32_decode(&msgb[i]),
			    &offer->router);
			break;
		case opt_dns_server:
			if (opt_len < 4 || opt_len % 4 != 0)
				return EINVAL;
			/* XXX Handle multiple DNS servers properly */
			inet_addr_set(dhcp_uint32_decode(&msgb[i]),
			    &offer->dns_server);
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

static errno_t dhcp_cfg_create(service_id_t iplink, dhcp_offer_t *offer)
{
	errno_t rc;
	service_id_t addr_id;
	service_id_t sroute_id;
	inet_naddr_t defr;

	rc = inetcfg_addr_create_static("dhcp4a", &offer->oaddr, iplink,
	    &addr_id);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Error creating IP address %s: %s", "dhcp4a", str_error(rc));
		return rc;
	}

	if (offer->router.addr != 0) {
		inet_naddr_set(0, 0, &defr);

		rc = inetcfg_sroute_create("dhcpdef", &defr, &offer->router, &sroute_id);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR, "Error creating "
			    "default route %s: %s.", "dhcpdef", str_error(rc));
			return rc;
		}
	}

	if (offer->dns_server.addr != 0) {
		rc = dnsr_set_srvaddr(&offer->dns_server);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR, "Error setting "
			    "nameserver address: %s)", str_error(rc));
			return rc;
		}
	}

	return EOK;
}

void dhcpsrv_links_init(void)
{
	list_initialize(&dhcp_links);
}

static dhcp_link_t *dhcpsrv_link_find(service_id_t link_id)
{
	list_foreach(dhcp_links, links, dhcp_link_t, dlink) {
		if (dlink->link_id == link_id)
			return dlink;
	}

	return NULL;
}

static void dhcp_link_set_failed(dhcp_link_t *dlink)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "Giving up on link %s",
	    dlink->link_info.name);
	dlink->state = ds_fail;
}

static errno_t dhcp_discover_proc(dhcp_link_t *dlink)
{
	dlink->state = ds_selecting;

	errno_t rc = dhcp_send_discover(dlink);
	if (rc != EOK)
		return EIO;

	dlink->retries_left = dhcp_discover_retries;

	if ((dlink->timeout->state == fts_not_set) ||
	    (dlink->timeout->state == fts_fired))
		fibril_timer_set(dlink->timeout, dhcp_discover_timeout_val,
		    dhcpsrv_discover_timeout, dlink);

	return rc;
}

errno_t dhcpsrv_link_add(service_id_t link_id)
{
	dhcp_link_t *dlink;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "dhcpsrv_link_add(%zu)", link_id);

	if (!inetcfg_inited) {
		rc = inetcfg_init();
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR, "Error contacting "
			    "inet configuration service.\n");
			return EIO;
		}

		inetcfg_inited = true;
	}

	if (dhcpsrv_link_find(link_id) != NULL) {
		log_msg(LOG_DEFAULT, LVL_NOTE, "Link %zu already added",
		    link_id);
		return EEXIST;
	}

	dlink = calloc(1, sizeof(dhcp_link_t));
	if (dlink == NULL)
		return ENOMEM;

	rc = rndgen_create(&dlink->rndgen);
	if (rc != EOK)
		goto error;

	dlink->link_id = link_id;
	dlink->timeout = fibril_timer_create(NULL);
	if (dlink->timeout == NULL) {
		rc = ENOMEM;
		goto error;
	}

	/* Get link hardware address */
	rc = inetcfg_link_get(link_id, &dlink->link_info);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error getting properties "
		    "for link %zu.", link_id);
		rc = EIO;
		goto error;
	}

	rc = dhcp_transport_init(&dlink->dt, link_id, dhcpsrv_recv, dlink);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error initializing DHCP "
		    "transport for link %s.", dlink->link_info.name);
		rc = EIO;
		goto error;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Send DHCPDISCOVER");
	rc = dhcp_discover_proc(dlink);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error sending DHCPDISCOVER.");
		dhcp_link_set_failed(dlink);
		rc = EIO;
		goto error;
	}

	list_append(&dlink->links, &dhcp_links);

	return EOK;
error:
	if (dlink != NULL && dlink->rndgen != NULL)
		rndgen_destroy(dlink->rndgen);
	if (dlink != NULL && dlink->timeout != NULL)
		fibril_timer_destroy(dlink->timeout);
	free(dlink);
	return rc;
}

errno_t dhcpsrv_link_remove(service_id_t link_id)
{
	return ENOTSUP;
}

errno_t dhcpsrv_discover(service_id_t link_id)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "dhcpsrv_link_add(%zu)", link_id);

	dhcp_link_t *dlink = dhcpsrv_link_find(link_id);

	if (dlink == NULL) {
		log_msg(LOG_DEFAULT, LVL_NOTE, "Link %zu doesn't exist",
		    link_id);
		return EINVAL;
	}

	return dhcp_discover_proc(dlink);
}

static void dhcpsrv_recv_offer(dhcp_link_t *dlink, dhcp_offer_t *offer)
{
	errno_t rc;

	if (dlink->state != ds_selecting) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Received offer in state "
		    " %d, ignoring.", (int)dlink->state);
		return;
	}

	fibril_timer_clear(dlink->timeout);
	dlink->offer = *offer;
	dlink->state = ds_requesting;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Send DHCPREQUEST");
	rc = dhcp_send_request(dlink, offer);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Error sending request.");
		return;
	}

	dlink->retries_left = dhcp_request_retries;
	fibril_timer_set(dlink->timeout, dhcp_request_timeout_val,
	    dhcpsrv_request_timeout, dlink);
}

static void dhcpsrv_recv_ack(dhcp_link_t *dlink, dhcp_offer_t *offer)
{
	errno_t rc;

	if (dlink->state != ds_requesting) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Received ack in state "
		    " %d, ignoring.", (int)dlink->state);
		return;
	}

	fibril_timer_clear(dlink->timeout);
	dlink->offer = *offer;
	dlink->state = ds_bound;

	rc = dhcp_cfg_create(dlink->link_id, offer);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Error creating configuration.");
		return;
	}

	log_msg(LOG_DEFAULT, LVL_NOTE, "%s: Successfully configured.",
	    dlink->link_info.name);
}

static void dhcpsrv_recv(void *arg, void *msg, size_t size)
{
	dhcp_link_t *dlink = (dhcp_link_t *)arg;
	dhcp_offer_t offer;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: dhcpsrv_recv() %zu bytes",
	    dlink->link_info.name, size);

	rc = dhcp_parse_reply(msg, size, &offer);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Error parsing reply");
		return;
	}

	switch (offer.msg_type) {
	case msg_dhcpoffer:
		dhcpsrv_recv_offer(dlink, &offer);
		break;
	case msg_dhcpack:
		dhcpsrv_recv_ack(dlink, &offer);
		break;
	default:
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Received unexpected "
		    "message type. %d", (int)offer.msg_type);
		break;
	}
}

static void dhcpsrv_discover_timeout(void *arg)
{
	dhcp_link_t *dlink = (dhcp_link_t *)arg;
	errno_t rc;

	assert(dlink->state == ds_selecting);
	log_msg(LOG_DEFAULT, LVL_NOTE, "%s: dhcpsrv_discover_timeout",
	    dlink->link_info.name);

	if (dlink->retries_left == 0) {
		log_msg(LOG_DEFAULT, LVL_NOTE, "Retries exhausted");
		dhcp_link_set_failed(dlink);
		return;
	}
	--dlink->retries_left;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Send DHCPDISCOVER");
	rc = dhcp_send_discover(dlink);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error sending DHCPDISCOVER");
		dhcp_link_set_failed(dlink);
		return;
	}

	fibril_timer_set(dlink->timeout, dhcp_discover_timeout_val,
	    dhcpsrv_discover_timeout, dlink);
}

static void dhcpsrv_request_timeout(void *arg)
{
	dhcp_link_t *dlink = (dhcp_link_t *)arg;
	errno_t rc;

	assert(dlink->state == ds_requesting);
	log_msg(LOG_DEFAULT, LVL_NOTE, "%s: dhcpsrv_request_timeout",
	    dlink->link_info.name);

	if (dlink->retries_left == 0) {
		log_msg(LOG_DEFAULT, LVL_NOTE, "Retries exhausted");
		dhcp_link_set_failed(dlink);
		return;
	}
	--dlink->retries_left;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Send DHCPREQUEST");
	rc = dhcp_send_request(dlink, &dlink->offer);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Error sending request.");
		dhcp_link_set_failed(dlink);
		return;
	}

	fibril_timer_set(dlink->timeout, dhcp_request_timeout_val,
	    dhcpsrv_request_timeout, dlink);
}

/** @}
 */
