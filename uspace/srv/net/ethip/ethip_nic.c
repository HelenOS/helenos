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

/** @addtogroup ethip
 * @{
 */
/**
 * @file
 * @brief
 */

#include <adt/list.h>
#include <async.h>
#include <errno.h>
#include <fibril_synch.h>
#include <inet/eth_addr.h>
#include <inet/iplink_srv.h>
#include <io/log.h>
#include <loc.h>
#include <mem.h>
#include <nic_iface.h>
#include <stdbool.h>
#include <stdlib.h>
#include <str_error.h>
#include "ethip.h"
#include "ethip_nic.h"
#include "pdu.h"

static errno_t ethip_nic_open(service_id_t sid);
static void ethip_nic_cb_conn(ipc_call_t *icall, void *arg);

static LIST_INITIALIZE(ethip_nic_list);
static FIBRIL_MUTEX_INITIALIZE(ethip_discovery_lock);

static errno_t ethip_nic_check_new(void)
{
	bool already_known;
	category_id_t iplink_cat;
	service_id_t *svcs;
	size_t count, i;
	errno_t rc;

	fibril_mutex_lock(&ethip_discovery_lock);

	rc = loc_category_get_id("nic", &iplink_cat, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed resolving category 'nic'.");
		fibril_mutex_unlock(&ethip_discovery_lock);
		return ENOENT;
	}

	rc = loc_category_get_svcs(iplink_cat, &svcs, &count);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed getting list of IP links.");
		fibril_mutex_unlock(&ethip_discovery_lock);
		return EIO;
	}

	for (i = 0; i < count; i++) {
		already_known = false;

		list_foreach(ethip_nic_list, link, ethip_nic_t, nic) {
			if (nic->svc_id == svcs[i]) {
				already_known = true;
				break;
			}
		}

		if (!already_known) {
			log_msg(LOG_DEFAULT, LVL_DEBUG, "Found NIC '%lu'",
			    (unsigned long) svcs[i]);
			rc = ethip_nic_open(svcs[i]);
			if (rc != EOK)
				log_msg(LOG_DEFAULT, LVL_ERROR, "Could not open NIC.");
		}
	}

	fibril_mutex_unlock(&ethip_discovery_lock);
	return EOK;
}

static ethip_nic_t *ethip_nic_new(void)
{
	ethip_nic_t *nic = calloc(1, sizeof(ethip_nic_t));
	if (nic == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed allocating NIC structure. "
		    "Out of memory.");
		return NULL;
	}

	link_initialize(&nic->link);
	list_initialize(&nic->addr_list);

	return nic;
}

static ethip_link_addr_t *ethip_nic_addr_new(inet_addr_t *addr)
{
	ethip_link_addr_t *laddr = calloc(1, sizeof(ethip_link_addr_t));
	if (laddr == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed allocating NIC address structure. "
		    "Out of memory.");
		return NULL;
	}

	link_initialize(&laddr->link);
	laddr->addr = *addr;

	return laddr;
}

static void ethip_nic_delete(ethip_nic_t *nic)
{
	if (nic->svc_name != NULL)
		free(nic->svc_name);

	free(nic);
}

static void ethip_link_addr_delete(ethip_link_addr_t *laddr)
{
	free(laddr);
}

static errno_t ethip_nic_open(service_id_t sid)
{
	bool in_list = false;
	nic_address_t nic_address;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ethip_nic_open()");
	ethip_nic_t *nic = ethip_nic_new();
	if (nic == NULL)
		return ENOMEM;

	errno_t rc = loc_service_get_name(sid, &nic->svc_name);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed getting service name.");
		goto error;
	}

	nic->sess = loc_service_connect(sid, INTERFACE_DDF, 0);
	if (nic->sess == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed connecting '%s'", nic->svc_name);
		goto error;
	}

	nic->svc_id = sid;

	rc = nic_callback_create(nic->sess, ethip_nic_cb_conn, nic);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed creating callback connection "
		    "from '%s'", nic->svc_name);
		goto error;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Opened NIC '%s'", nic->svc_name);
	list_append(&nic->link, &ethip_nic_list);
	in_list = true;

	rc = ethip_iplink_init(nic);
	if (rc != EOK)
		goto error;

	rc = nic_get_address(nic->sess, &nic_address);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error getting MAC address of NIC '%s'.",
		    nic->svc_name);
		goto error;
	}

	eth_addr_decode(nic_address.address, &nic->mac_addr);

	rc = nic_set_state(nic->sess, NIC_STATE_ACTIVE);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error activating NIC '%s'.",
		    nic->svc_name);
		goto error;
	}

	rc = nic_broadcast_set_mode(nic->sess, NIC_BROADCAST_ACCEPTED);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error enabling "
		    "reception of broadcast frames on '%s'.", nic->svc_name);
		goto error;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Initialized IP link service,");

	return EOK;

error:
	if (in_list)
		list_remove(&nic->link);

	if (nic->sess != NULL)
		async_hangup(nic->sess);

	ethip_nic_delete(nic);
	return rc;
}

static void ethip_nic_cat_change_cb(void *arg)
{
	(void) ethip_nic_check_new();
}

static void ethip_nic_addr_changed(ethip_nic_t *nic, ipc_call_t *call)
{
	uint8_t *addr;
	size_t size;
	eth_addr_str_t saddr;
	errno_t rc;

	rc = async_data_write_accept((void **) &addr, false, 0, 0, 0, &size);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "data_write_accept() failed");
		return;
	}

	eth_addr_decode(addr, &nic->mac_addr);
	eth_addr_format(&nic->mac_addr, &saddr);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ethip_nic_addr_changed(): "
	    "new addr=%s", saddr.str);

	rc = iplink_ev_change_addr(&nic->iplink, &nic->mac_addr);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "iplink_ev_change_addr() failed");
		return;
	}

	free(addr);
	async_answer_0(call, EOK);
}

static void ethip_nic_received(ethip_nic_t *nic, ipc_call_t *call)
{
	errno_t rc;
	void *data;
	size_t size;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ethip_nic_received() nic=%p", nic);

	rc = async_data_write_accept(&data, false, 0, 0, 0, &size);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "data_write_accept() failed");
		return;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Ethernet PDU contents (%zu bytes)",
	    size);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "call ethip_received");
	rc = ethip_received(&nic->iplink, data, size);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "free data");
	free(data);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ethip_nic_received() done, rc=%s", str_error_name(rc));
	async_answer_0(call, rc);
}

static void ethip_nic_device_state(ethip_nic_t *nic, ipc_call_t *call)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "ethip_nic_device_state()");
	async_answer_0(call, ENOTSUP);
}

static void ethip_nic_cb_conn(ipc_call_t *icall, void *arg)
{
	ethip_nic_t *nic = (ethip_nic_t *)arg;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ethnip_nic_cb_conn()");

	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			async_answer_0(&call, EOK);
			return;
		}

		switch (ipc_get_imethod(&call)) {
		case NIC_EV_ADDR_CHANGED:
			ethip_nic_addr_changed(nic, &call);
			break;
		case NIC_EV_RECEIVED:
			ethip_nic_received(nic, &call);
			break;
		case NIC_EV_DEVICE_STATE:
			ethip_nic_device_state(nic, &call);
			break;
		default:
			log_msg(LOG_DEFAULT, LVL_DEBUG, "unknown IPC method: %" PRIun, ipc_get_imethod(&call));
			async_answer_0(&call, ENOTSUP);
		}
	}
}

errno_t ethip_nic_discovery_start(void)
{
	errno_t rc = loc_register_cat_change_cb(ethip_nic_cat_change_cb, NULL);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering callback for NIC "
		    "discovery: %s.", str_error(rc));
		return rc;
	}

	return ethip_nic_check_new();
}

ethip_nic_t *ethip_nic_find_by_iplink_sid(service_id_t iplink_sid)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "ethip_nic_find_by_iplink_sid(%u)",
	    (unsigned) iplink_sid);

	list_foreach(ethip_nic_list, link, ethip_nic_t, nic) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "ethip_nic_find_by_iplink_sid - element");
		if (nic->iplink_sid == iplink_sid) {
			log_msg(LOG_DEFAULT, LVL_DEBUG, "ethip_nic_find_by_iplink_sid - found %p", nic);
			return nic;
		}
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ethip_nic_find_by_iplink_sid - not found");
	return NULL;
}

errno_t ethip_nic_send(ethip_nic_t *nic, void *data, size_t size)
{
	errno_t rc;
	log_msg(LOG_DEFAULT, LVL_DEBUG, "ethip_nic_send(size=%zu)", size);
	rc = nic_send_frame(nic->sess, data, size);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "nic_send_frame -> %s", str_error_name(rc));
	return rc;
}

/** Setup accepted multicast addresses
 *
 * Currently the set of accepted multicast addresses is
 * determined only based on IPv6 addresses.
 *
 */
static errno_t ethip_nic_setup_multicast(ethip_nic_t *nic)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "ethip_nic_setup_multicast()");

	/* Count the number of multicast addresses */

	size_t count = 0;

	list_foreach(nic->addr_list, link, ethip_link_addr_t, laddr) {
		ip_ver_t ver = inet_addr_get(&laddr->addr, NULL, NULL);
		if (ver == ip_v6)
			count++;
	}

	if (count == 0)
		return nic_multicast_set_mode(nic->sess, NIC_MULTICAST_BLOCKED,
		    NULL, 0);

	nic_address_t *mac_list = calloc(count, sizeof(nic_address_t));
	if (mac_list == NULL)
		return ENOMEM;

	/* Create the multicast MAC list */

	size_t i = 0;

	list_foreach(nic->addr_list, link, ethip_link_addr_t, laddr) {
		addr128_t v6;
		ip_ver_t ver = inet_addr_get(&laddr->addr, NULL, &v6);
		if (ver != ip_v6)
			continue;

		assert(i < count);

		eth_addr_t mac;
		eth_addr_solicited_node(v6, &mac);

		/* Avoid duplicate addresses in the list */

		bool found = false;

		for (size_t j = 0; j < i; j++) {
			eth_addr_t mac_entry;
			eth_addr_decode(mac_list[j].address, &mac_entry);
			if (eth_addr_compare(&mac_entry, &mac)) {
				found = true;
				break;
			}
		}

		if (!found) {
			eth_addr_encode(&mac, mac_list[i].address);
			i++;
		} else {
			count--;
		}
	}

	/* Setup the multicast MAC list */

	errno_t rc = nic_multicast_set_mode(nic->sess, NIC_MULTICAST_LIST,
	    mac_list, count);

	free(mac_list);
	return rc;
}

errno_t ethip_nic_addr_add(ethip_nic_t *nic, inet_addr_t *addr)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "ethip_nic_addr_add()");

	ethip_link_addr_t *laddr = ethip_nic_addr_new(addr);
	if (laddr == NULL)
		return ENOMEM;

	list_append(&laddr->link, &nic->addr_list);

	return ethip_nic_setup_multicast(nic);
}

errno_t ethip_nic_addr_remove(ethip_nic_t *nic, inet_addr_t *addr)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "ethip_nic_addr_remove()");

	ethip_link_addr_t *laddr = ethip_nic_addr_find(nic, addr);
	if (laddr == NULL)
		return ENOENT;

	list_remove(&laddr->link);
	ethip_link_addr_delete(laddr);

	return ethip_nic_setup_multicast(nic);
}

ethip_link_addr_t *ethip_nic_addr_find(ethip_nic_t *nic,
    inet_addr_t *addr)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "ethip_nic_addr_find()");

	list_foreach(nic->addr_list, link, ethip_link_addr_t, laddr) {
		if (inet_addr_compare(addr, &laddr->addr))
			return laddr;
	}

	return NULL;
}

/** @}
 */
