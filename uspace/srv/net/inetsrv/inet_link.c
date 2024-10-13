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

/** @addtogroup inet
 * @{
 */
/**
 * @file
 * @brief
 */

#include <errno.h>
#include <fibril_synch.h>
#include <inet/dhcp.h>
#include <inet/eth_addr.h>
#include <inet/iplink.h>
#include <io/log.h>
#include <loc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>
#include "addrobj.h"
#include "inetsrv.h"
#include "inet_link.h"
#include "pdu.h"

static bool first_link = true;
static bool first_link6 = true;

static FIBRIL_MUTEX_INITIALIZE(ip_ident_lock);
static uint16_t ip_ident = 0;

static errno_t inet_iplink_recv(iplink_t *, iplink_recv_sdu_t *, ip_ver_t);
static errno_t inet_iplink_change_addr(iplink_t *, eth_addr_t *);
static inet_link_t *inet_link_get_by_id_locked(sysarg_t);

static iplink_ev_ops_t inet_iplink_ev_ops = {
	.recv = inet_iplink_recv,
	.change_addr = inet_iplink_change_addr,
};

static LIST_INITIALIZE(inet_links);
static FIBRIL_MUTEX_INITIALIZE(inet_links_lock);

static addr128_t link_local_node_ip =
    { 0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xfe, 0, 0, 0 };

static void inet_link_local_node_ip(eth_addr_t *mac_addr,
    addr128_t ip_addr)
{
	uint8_t b[ETH_ADDR_SIZE];

	memcpy(ip_addr, link_local_node_ip, 16);
	eth_addr_encode(mac_addr, b);

	ip_addr[8] = b[0] ^ 0x02;
	ip_addr[9] = b[1];
	ip_addr[10] = b[2];
	ip_addr[13] = b[3];
	ip_addr[14] = b[4];
	ip_addr[15] = b[5];
}

static errno_t inet_iplink_recv(iplink_t *iplink, iplink_recv_sdu_t *sdu, ip_ver_t ver)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_iplink_recv()");

	errno_t rc;
	inet_packet_t packet;
	inet_link_t *ilink;

	ilink = (inet_link_t *)iplink_get_userptr(iplink);

	switch (ver) {
	case ip_v4:
		rc = inet_pdu_decode(sdu->data, sdu->size, ilink->svc_id,
		    &packet);
		break;
	case ip_v6:
		rc = inet_pdu_decode6(sdu->data, sdu->size, ilink->svc_id,
		    &packet);
		break;
	default:
		log_msg(LOG_DEFAULT, LVL_DEBUG, "invalid IP version");
		return EINVAL;
	}

	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "failed decoding PDU");
		return rc;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_iplink_recv: link_id=%zu", packet.link_id);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "call inet_recv_packet()");
	rc = inet_recv_packet(&packet);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "call inet_recv_packet -> %s", str_error_name(rc));
	free(packet.data);

	return rc;
}

static errno_t inet_iplink_change_addr(iplink_t *iplink, eth_addr_t *mac)
{
	eth_addr_str_t saddr;

	eth_addr_format(mac, &saddr);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_iplink_change_addr(): "
	    "new addr=%s", saddr.str);

	list_foreach(inet_links, link_list, inet_link_t, ilink) {
		if (ilink->sess == iplink->sess)
			ilink->mac = *mac;
	}

	return EOK;
}

static inet_link_t *inet_link_new(void)
{
	inet_link_t *ilink = calloc(1, sizeof(inet_link_t));

	if (ilink == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed allocating link structure. "
		    "Out of memory.");
		return NULL;
	}

	link_initialize(&ilink->link_list);

	return ilink;
}

static void inet_link_delete(inet_link_t *ilink)
{
	if (ilink->svc_name != NULL)
		free(ilink->svc_name);

	free(ilink);
}

/** Open new IP link while inet_links_lock is held.
 *
 * @param sid IP link service ID
 * @return EOK on success or an error code
 */
static errno_t inet_link_open_locked(service_id_t sid)
{
	inet_link_t *ilink;
	inet_addr_t iaddr;
	errno_t rc;

	assert(fibril_mutex_is_locked(&inet_links_lock));

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_link_open_locked()");
	ilink = inet_link_new();
	if (ilink == NULL)
		return ENOMEM;

	ilink->svc_id = sid;
	ilink->iplink = NULL;

	rc = loc_service_get_name(sid, &ilink->svc_name);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed getting service name.");
		goto error;
	}

	ilink->sess = loc_service_connect(sid, INTERFACE_IPLINK, 0);
	if (ilink->sess == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed connecting '%s'", ilink->svc_name);
		goto error;
	}

	rc = iplink_open(ilink->sess, &inet_iplink_ev_ops, ilink, &ilink->iplink);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed opening IP link '%s'",
		    ilink->svc_name);
		goto error;
	}

	rc = iplink_get_mtu(ilink->iplink, &ilink->def_mtu);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed determinning MTU of link '%s'",
		    ilink->svc_name);
		goto error;
	}

	/*
	 * Get the MAC address of the link. If the link has a MAC
	 * address, we assume that it supports NDP.
	 */
	rc = iplink_get_mac48(ilink->iplink, &ilink->mac);
	ilink->mac_valid = (rc == EOK);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Opened IP link '%s'", ilink->svc_name);

	if (inet_link_get_by_id_locked(sid) != NULL) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Link %zu already open",
		    sid);
		rc = EEXIST;
		goto error;
	}

	list_append(&ilink->link_list, &inet_links);

	inet_addrobj_t *addr = NULL;

	/* XXX FIXME Cannot rely on loopback being the first IP link service!! */
	if (first_link) {
		addr = inet_addrobj_new();

		inet_naddr(&addr->naddr, 127, 0, 0, 1, 24);
		first_link = false;
	}

	if (addr != NULL) {
		addr->ilink = ilink;
		addr->name = str_dup("v4a");
		addr->temp = true;

		rc = inet_addrobj_add(addr);
		if (rc == EOK) {
			inet_naddr_addr(&addr->naddr, &iaddr);
			rc = iplink_addr_add(ilink->iplink, &iaddr);
			if (rc != EOK) {
				log_msg(LOG_DEFAULT, LVL_ERROR,
				    "Failed setting IPv4 address on internet link.");
				inet_addrobj_remove(addr);
				inet_addrobj_delete(addr);
			}
		} else {
			log_msg(LOG_DEFAULT, LVL_ERROR, "Failed adding IPv4 address.");
			inet_addrobj_delete(addr);
		}
	}

	inet_addrobj_t *addr6 = NULL;

	if (first_link6) {
		addr6 = inet_addrobj_new();

		inet_naddr6(&addr6->naddr, 0, 0, 0, 0, 0, 0, 0, 1, 128);
		first_link6 = false;
	} else if (ilink->mac_valid) {
		addr6 = inet_addrobj_new();

		addr128_t link_local;
		inet_link_local_node_ip(&ilink->mac, link_local);

		inet_naddr_set6(link_local, 64, &addr6->naddr);
	}

	if (addr6 != NULL) {
		addr6->ilink = ilink;
		addr6->name = str_dup("v6a");
		addr6->temp = true;

		rc = inet_addrobj_add(addr6);
		if (rc == EOK) {
			inet_naddr_addr(&addr6->naddr, &iaddr);
			rc = iplink_addr_add(ilink->iplink, &iaddr);
			if (rc != EOK) {
				log_msg(LOG_DEFAULT, LVL_ERROR,
				    "Failed setting IPv6 address on internet link.");
				inet_addrobj_remove(addr6);
				inet_addrobj_delete(addr6);
			}
		} else {
			log_msg(LOG_DEFAULT, LVL_ERROR, "Failed adding IPv6 address.");
			inet_addrobj_delete(addr6);
		}
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Configured link '%s'.", ilink->svc_name);
	return EOK;

error:
	if (ilink->iplink != NULL)
		iplink_close(ilink->iplink);

	inet_link_delete(ilink);
	return rc;
}

/** Open new IP link..
 *
 * @param sid IP link service ID
 * @return EOK on success or an error code
 */
errno_t inet_link_open(service_id_t sid)
{
	errno_t rc;

	fibril_mutex_lock(&inet_links_lock);
	rc = inet_link_open_locked(sid);
	fibril_mutex_unlock(&inet_links_lock);

	return rc;
}

/** Send IPv4 datagram over Internet link
 *
 * @param ilink Internet link
 * @param lsrc  Source IPv4 address
 * @param ldest Destination IPv4 address
 * @param dgram IPv4 datagram body
 * @param proto Protocol
 * @param ttl   Time-to-live
 * @param df    Do-not-Fragment flag
 *
 * @return EOK on success
 * @return ENOMEM when not enough memory to create the datagram
 * @return ENOTSUP if networking mode is not supported
 *
 */
errno_t inet_link_send_dgram(inet_link_t *ilink, addr32_t lsrc, addr32_t ldest,
    inet_dgram_t *dgram, uint8_t proto, uint8_t ttl, int df)
{
	addr32_t src_v4;
	ip_ver_t src_ver = inet_addr_get(&dgram->src, &src_v4, NULL);
	if (src_ver != ip_v4)
		return EINVAL;

	addr32_t dest_v4;
	ip_ver_t dest_ver = inet_addr_get(&dgram->dest, &dest_v4, NULL);
	if (dest_ver != ip_v4)
		return EINVAL;

	/*
	 * Fill packet structure. Fragmentation is performed by
	 * inet_pdu_encode().
	 */

	iplink_sdu_t sdu;

	sdu.src = lsrc;
	sdu.dest = ldest;

	inet_packet_t packet;

	packet.src = dgram->src;
	packet.dest = dgram->dest;
	packet.tos = dgram->tos;
	packet.proto = proto;
	packet.ttl = ttl;

	/* Allocate identifier */
	fibril_mutex_lock(&ip_ident_lock);
	packet.ident = ++ip_ident;
	fibril_mutex_unlock(&ip_ident_lock);

	packet.df = df;
	packet.data = dgram->data;
	packet.size = dgram->size;

	errno_t rc;
	size_t offs = 0;

	do {
		/* Encode one fragment */

		size_t roffs;
		rc = inet_pdu_encode(&packet, src_v4, dest_v4, offs, ilink->def_mtu,
		    &sdu.data, &sdu.size, &roffs);
		if (rc != EOK)
			return rc;

		/* Send the PDU */
		rc = iplink_send(ilink->iplink, &sdu);

		free(sdu.data);
		offs = roffs;
	} while (offs < packet.size);

	return rc;
}

/** Send IPv6 datagram over Internet link
 *
 * @param ilink Internet link
 * @param ldest Destination MAC address
 * @param dgram IPv6 datagram body
 * @param proto Next header
 * @param ttl   Hop limit
 * @param df    Do-not-Fragment flag (unused)
 *
 * @return EOK on success
 * @return ENOMEM when not enough memory to create the datagram
 *
 */
errno_t inet_link_send_dgram6(inet_link_t *ilink, eth_addr_t *ldest,
    inet_dgram_t *dgram, uint8_t proto, uint8_t ttl, int df)
{
	addr128_t src_v6;
	ip_ver_t src_ver = inet_addr_get(&dgram->src, NULL, &src_v6);
	if (src_ver != ip_v6)
		return EINVAL;

	addr128_t dest_v6;
	ip_ver_t dest_ver = inet_addr_get(&dgram->dest, NULL, &dest_v6);
	if (dest_ver != ip_v6)
		return EINVAL;

	iplink_sdu6_t sdu6;
	sdu6.dest = *ldest;

	/*
	 * Fill packet structure. Fragmentation is performed by
	 * inet_pdu_encode6().
	 */

	inet_packet_t packet;

	packet.src = dgram->src;
	packet.dest = dgram->dest;
	packet.tos = dgram->tos;
	packet.proto = proto;
	packet.ttl = ttl;

	/* Allocate identifier */
	fibril_mutex_lock(&ip_ident_lock);
	packet.ident = ++ip_ident;
	fibril_mutex_unlock(&ip_ident_lock);

	packet.df = df;
	packet.data = dgram->data;
	packet.size = dgram->size;

	errno_t rc;
	size_t offs = 0;

	do {
		/* Encode one fragment */

		size_t roffs;
		rc = inet_pdu_encode6(&packet, src_v6, dest_v6, offs, ilink->def_mtu,
		    &sdu6.data, &sdu6.size, &roffs);
		if (rc != EOK)
			return rc;

		/* Send the PDU */
		rc = iplink_send6(ilink->iplink, &sdu6);

		free(sdu6.data);
		offs = roffs;
	} while (offs < packet.size);

	return rc;
}

static inet_link_t *inet_link_get_by_id_locked(sysarg_t link_id)
{
	assert(fibril_mutex_is_locked(&inet_links_lock));

	list_foreach(inet_links, link_list, inet_link_t, ilink) {
		if (ilink->svc_id == link_id)
			return ilink;
	}

	return NULL;
}

inet_link_t *inet_link_get_by_id(sysarg_t link_id)
{
	inet_link_t *ilink;

	fibril_mutex_lock(&inet_links_lock);
	ilink = inet_link_get_by_id_locked(link_id);
	fibril_mutex_unlock(&inet_links_lock);

	return ilink;
}

/** Find link by service name while inet_links_lock is held.
 *
 * @param svc_name Service name
 * @return Link or @c NULL if not found
 */
static inet_link_t *inet_link_get_by_svc_name_locked(const char *svc_name)
{
	assert(fibril_mutex_is_locked(&inet_links_lock));

	list_foreach(inet_links, link_list, inet_link_t, ilink) {
		if (str_cmp(ilink->svc_name, svc_name) == 0)
			return ilink;
	}

	return NULL;
}

/** Find link by service name.
 *
 * @param svc_name Service name
 * @return Link or @c NULL if not found
 */
inet_link_t *inet_link_get_by_svc_name(const char *svc_name)
{
	inet_link_t *ilink;

	fibril_mutex_lock(&inet_links_lock);
	ilink = inet_link_get_by_svc_name_locked(svc_name);
	fibril_mutex_unlock(&inet_links_lock);

	return ilink;
}

/** Get IDs of all links. */
errno_t inet_link_get_id_list(sysarg_t **rid_list, size_t *rcount)
{
	sysarg_t *id_list;
	size_t count, i;

	fibril_mutex_lock(&inet_links_lock);
	count = list_count(&inet_links);

	id_list = calloc(count, sizeof(sysarg_t));
	if (id_list == NULL) {
		fibril_mutex_unlock(&inet_links_lock);
		return ENOMEM;
	}

	i = 0;
	list_foreach(inet_links, link_list, inet_link_t, ilink) {
		id_list[i++] = ilink->svc_id;
	}

	fibril_mutex_unlock(&inet_links_lock);

	*rid_list = id_list;
	*rcount = count;

	return EOK;
}

/** Check for new IP links.
 *
 * @return EOK on success or an error code
 */
static errno_t inet_link_check_new(void)
{
	bool already_known;
	category_id_t iplink_cat;
	service_id_t *svcs;
	inet_link_cfg_info_t info;
	size_t count, i;
	errno_t rc;

	fibril_mutex_lock(&inet_links_lock);

	rc = loc_category_get_id("iplink", &iplink_cat, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed resolving category "
		    "'iplink'.");
		fibril_mutex_unlock(&inet_links_lock);
		return ENOENT;
	}

	rc = loc_category_get_svcs(iplink_cat, &svcs, &count);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed getting list of IP "
		    "links.");
		fibril_mutex_unlock(&inet_links_lock);
		return EIO;
	}

	for (i = 0; i < count; i++) {
		already_known = false;

		list_foreach(inet_links, link_list, inet_link_t, ilink) {
			if (ilink->svc_id == svcs[i]) {
				already_known = true;
				break;
			}
		}

		if (!already_known) {
			log_msg(LOG_DEFAULT, LVL_NOTE, "Found IP link '%lu'",
			    (unsigned long) svcs[i]);
			rc = inet_link_open_locked(svcs[i]);
			if (rc != EOK) {
				log_msg(LOG_DEFAULT, LVL_ERROR, "Could not "
				    "add IP link.");
			}
		} else {
			/* Clear so it won't be autoconfigured below */
			svcs[i] = 0;
		}
	}

	fibril_mutex_unlock(&inet_links_lock);

	/*
	 * Auto-configure new links. Note that newly discovered links
	 * cannot have any configured address objects, because we only
	 * retain configuration for present links.
	 */
	for (i = 0; i < count; i++) {
		if (svcs[i] != 0) {
			info.svc_id = svcs[i];
			rc = loc_service_get_name(info.svc_id, &info.svc_name);
			if (rc != EOK) {
				log_msg(LOG_DEFAULT, LVL_ERROR, "Failed "
				    "getting service name.");
				return rc;
			}

			inet_link_autoconf_link(&info);
			free(info.svc_name);
			info.svc_name = NULL;
		}
	}

	return EOK;
}

/** IP link category change callback.
 *
 * @param arg Not used
 */
static void inet_link_cat_change_cb(void *arg)
{
	(void) inet_link_check_new();
}

/** Start IP link discovery.
 *
 * @return EOK on success or an error code
 */
errno_t inet_link_discovery_start(void)
{
	errno_t rc;

	rc = loc_register_cat_change_cb(inet_link_cat_change_cb, NULL);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering callback "
		    "for IP link discovery: %s.", str_error(rc));
		return rc;
	}

	return inet_link_check_new();
}

/** Start DHCP autoconfiguration on IP link.
 *
 * @param info Link information
 */
void inet_link_autoconf_link(inet_link_cfg_info_t *info)
{
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "inet_link_autoconf_link");

	if (str_lcmp(info->svc_name, "net/eth", str_length("net/eth")) == 0) {
		log_msg(LOG_DEFAULT, LVL_NOTE, "inet_link_autoconf_link : dhcp link add for link '%s' (%u)",
		    info->svc_name, (unsigned)info->svc_id);
		rc = dhcp_link_add(info->svc_id);
		log_msg(LOG_DEFAULT, LVL_NOTE, "inet_link_autoconf_link : dhcp link add for link '%s' (%u) DONE",
		    info->svc_name, (unsigned)info->svc_id);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_WARN, "Failed configuring "
			    "DHCP on  link '%s'.\n", info->svc_name);
		}
	}
}

/** Start DHCP autoconfiguration on IP links. */
errno_t inet_link_autoconf(void)
{
	inet_link_cfg_info_t *link_info;
	size_t link_cnt;
	size_t acnt;
	size_t i;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "inet_link_autoconf : initialize DHCP");
	rc = dhcp_init();
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_WARN, "Failed initializing DHCP "
		    "service.");
		return rc;
	}

	log_msg(LOG_DEFAULT, LVL_NOTE, "inet_link_autoconf : initialize DHCP done");

	fibril_mutex_lock(&inet_links_lock);
	link_cnt = list_count(&inet_links);

	link_info = calloc(link_cnt, sizeof(inet_link_cfg_info_t));
	if (link_info == NULL) {
		fibril_mutex_unlock(&inet_links_lock);
		return ENOMEM;
	}

	i = 0;
	list_foreach(inet_links, link_list, inet_link_t, ilink) {
		assert(i < link_cnt);

		acnt = inet_addrobj_cnt_by_link(ilink);
		if (acnt != 0) {
			/*
			 * No autoconfiguration if link has configured
			 * addresses.
			 */
			continue;
		}

		link_info[i].svc_id = ilink->svc_id;
		link_info[i].svc_name = str_dup(ilink->svc_name);
		if (link_info[i].svc_name == NULL) {
			fibril_mutex_unlock(&inet_links_lock);
			goto error;
		}

		++i;
	}

	fibril_mutex_unlock(&inet_links_lock);

	/* Update link_cnt to include only links slated for autoconfig. */
	link_cnt = i;

	log_msg(LOG_DEFAULT, LVL_NOTE, "inet_link_autoconf : autoconf links...");

	for (i = 0; i < link_cnt; i++)
		inet_link_autoconf_link(&link_info[i]);

	for (i = 0; i < link_cnt; i++) {
		if (link_info[i].svc_name != NULL)
			free(link_info[i].svc_name);
	}

	log_msg(LOG_DEFAULT, LVL_NOTE, "inet_link_autoconf : autoconf links done");
	return EOK;
error:
	for (i = 0; i < link_cnt; i++) {
		if (link_info[i].svc_name != NULL)
			free(link_info[i].svc_name);
	}
	free(link_info);
	return ENOMEM;
}

/** @}
 */
