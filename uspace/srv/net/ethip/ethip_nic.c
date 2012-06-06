/*
 * Copyright (c) 2012 Jiri Svoboda
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
#include <bool.h>
#include <errno.h>
#include <fibril_synch.h>
#include <inet/iplink_srv.h>
#include <io/log.h>
#include <loc.h>
#include <device/nic.h>
#include <stdlib.h>

#include "ethip.h"
#include "ethip_nic.h"
#include "pdu.h"

static int ethip_nic_open(service_id_t sid);
static void ethip_nic_cb_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg);

static LIST_INITIALIZE(ethip_nic_list);
static FIBRIL_MUTEX_INITIALIZE(ethip_discovery_lock);

static int ethip_nic_check_new(void)
{
	bool already_known;
	category_id_t iplink_cat;
	service_id_t *svcs;
	size_t count, i;
	int rc;

	fibril_mutex_lock(&ethip_discovery_lock);

	rc = loc_category_get_id("nic", &iplink_cat, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		log_msg(LVL_ERROR, "Failed resolving category 'nic'.");
		fibril_mutex_unlock(&ethip_discovery_lock);
		return ENOENT;
	}

	rc = loc_category_get_svcs(iplink_cat, &svcs, &count);
	if (rc != EOK) {
		log_msg(LVL_ERROR, "Failed getting list of IP links.");
		fibril_mutex_unlock(&ethip_discovery_lock);
		return EIO;
	}

	for (i = 0; i < count; i++) {
		already_known = false;

		list_foreach(ethip_nic_list, nic_link) {
			ethip_nic_t *nic = list_get_instance(nic_link,
			    ethip_nic_t, nic_list);
			if (nic->svc_id == svcs[i]) {
				already_known = true;
				break;
			}
		}

		if (!already_known) {
			log_msg(LVL_DEBUG, "Found NIC '%lu'",
			    (unsigned long) svcs[i]);
			rc = ethip_nic_open(svcs[i]);
			if (rc != EOK)
				log_msg(LVL_ERROR, "Could not open NIC.");
		}
	}

	fibril_mutex_unlock(&ethip_discovery_lock);
	return EOK;
}

static ethip_nic_t *ethip_nic_new(void)
{
	ethip_nic_t *nic = calloc(1, sizeof(ethip_nic_t));

	if (nic == NULL) {
		log_msg(LVL_ERROR, "Failed allocating NIC structure. "
		    "Out of memory.");
		return NULL;
	}

	link_initialize(&nic->nic_list);
	list_initialize(&nic->addr_list);

	return nic;
}

static ethip_link_addr_t *ethip_nic_addr_new(iplink_srv_addr_t *addr)
{
	ethip_link_addr_t *laddr = calloc(1, sizeof(ethip_link_addr_t));

	if (laddr == NULL) {
		log_msg(LVL_ERROR, "Failed allocating NIC address structure. "
		    "Out of memory.");
		return NULL;
	}

	link_initialize(&laddr->addr_list);
	laddr->addr.ipv4 = addr->ipv4;
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

static int ethip_nic_open(service_id_t sid)
{
	bool in_list = false;
	nic_address_t nic_address;
	
	log_msg(LVL_DEBUG, "ethip_nic_open()");
	ethip_nic_t *nic = ethip_nic_new();
	if (nic == NULL)
		return ENOMEM;
	
	int rc = loc_service_get_name(sid, &nic->svc_name);
	if (rc != EOK) {
		log_msg(LVL_ERROR, "Failed getting service name.");
		goto error;
	}
	
	nic->sess = loc_service_connect(EXCHANGE_SERIALIZE, sid, 0);
	if (nic->sess == NULL) {
		log_msg(LVL_ERROR, "Failed connecting '%s'", nic->svc_name);
		goto error;
	}
	
	nic->svc_id = sid;
	
	rc = nic_callback_create(nic->sess, ethip_nic_cb_conn, nic);
	if (rc != EOK) {
		log_msg(LVL_ERROR, "Failed creating callback connection "
		    "from '%s'", nic->svc_name);
		goto error;
	}

	log_msg(LVL_DEBUG, "Opened NIC '%s'", nic->svc_name);
	list_append(&nic->nic_list, &ethip_nic_list);
	in_list = true;

	rc = ethip_iplink_init(nic);
	if (rc != EOK)
		goto error;

	rc = nic_get_address(nic->sess, &nic_address);
	if (rc != EOK) {
		log_msg(LVL_ERROR, "Error getting MAC address of NIC '%s'.",
		    nic->svc_name);
		goto error;
	}

	mac48_decode(nic_address.address, &nic->mac_addr);

	rc = nic_set_state(nic->sess, NIC_STATE_ACTIVE);
	if (rc != EOK) {
		log_msg(LVL_ERROR, "Error activating NIC '%s'.",
		    nic->svc_name);
		goto error;
	}

	log_msg(LVL_DEBUG, "Initialized IP link service, MAC = 0x%" PRIx64,
	    nic->mac_addr.addr);

	return EOK;

error:
	if (in_list)
		list_remove(&nic->nic_list);
	if (nic->sess != NULL)
		async_hangup(nic->sess);
	ethip_nic_delete(nic);
	return rc;
}

static void ethip_nic_cat_change_cb(void)
{
	(void) ethip_nic_check_new();
}

static void ethip_nic_addr_changed(ethip_nic_t *nic, ipc_callid_t callid,
    ipc_call_t *call)
{
	log_msg(LVL_DEBUG, "ethip_nic_addr_changed()");
	async_answer_0(callid, ENOTSUP);
}

static void ethip_nic_received(ethip_nic_t *nic, ipc_callid_t callid,
    ipc_call_t *call)
{
	int rc;
	void *data;
	size_t size;

	log_msg(LVL_DEBUG, "ethip_nic_received() nic=%p", nic);

	rc = async_data_write_accept(&data, false, 0, 0, 0, &size);
	if (rc != EOK) {
		log_msg(LVL_DEBUG, "data_write_accept() failed");
		return;
	}

	log_msg(LVL_DEBUG, "Ethernet PDU contents (%zu bytes)",
	    size);

	log_msg(LVL_DEBUG, "call ethip_received");
	rc = ethip_received(&nic->iplink, data, size);
	log_msg(LVL_DEBUG, "free data");
	free(data);

	log_msg(LVL_DEBUG, "ethip_nic_received() done, rc=%d", rc);
	async_answer_0(callid, rc);
}

static void ethip_nic_device_state(ethip_nic_t *nic, ipc_callid_t callid,
    ipc_call_t *call)
{
	log_msg(LVL_DEBUG, "ethip_nic_device_state()");
	async_answer_0(callid, ENOTSUP);
}

static void ethip_nic_cb_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	ethip_nic_t *nic = (ethip_nic_t *)arg;

	log_msg(LVL_DEBUG, "ethnip_nic_cb_conn()");

	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);

		if (!IPC_GET_IMETHOD(call)) {
			/* TODO: Handle hangup */
			return;
		}

		switch (IPC_GET_IMETHOD(call)) {
		case NIC_EV_ADDR_CHANGED:
			ethip_nic_addr_changed(nic, callid, &call);
			break;
		case NIC_EV_RECEIVED:
			ethip_nic_received(nic, callid, &call);
			break;
		case NIC_EV_DEVICE_STATE:
			ethip_nic_device_state(nic, callid, &call);
			break;
		default:
			async_answer_0(callid, ENOTSUP);
		}
	}
}

int ethip_nic_discovery_start(void)
{
	int rc = loc_register_cat_change_cb(ethip_nic_cat_change_cb);
	if (rc != EOK) {
		log_msg(LVL_ERROR, "Failed registering callback for NIC "
		    "discovery (%d).", rc);
		return rc;
	}
	
	return ethip_nic_check_new();
}

ethip_nic_t *ethip_nic_find_by_iplink_sid(service_id_t iplink_sid)
{
	log_msg(LVL_DEBUG, "ethip_nic_find_by_iplink_sid(%u)",
	    (unsigned) iplink_sid);

	list_foreach(ethip_nic_list, link) {
		log_msg(LVL_DEBUG, "ethip_nic_find_by_iplink_sid - element");
		ethip_nic_t *nic = list_get_instance(link, ethip_nic_t,
		    nic_list);

		if (nic->iplink_sid == iplink_sid) {
			log_msg(LVL_DEBUG, "ethip_nic_find_by_iplink_sid - found %p", nic);
			return nic;
		}
	}

	log_msg(LVL_DEBUG, "ethip_nic_find_by_iplink_sid - not found");
	return NULL;
}

int ethip_nic_send(ethip_nic_t *nic, void *data, size_t size)
{
	int rc;
	log_msg(LVL_DEBUG, "ethip_nic_send(size=%zu)", size);
	rc = nic_send_frame(nic->sess, data, size);
	log_msg(LVL_DEBUG, "nic_send_frame -> %d", rc);
	return rc;
}

int ethip_nic_addr_add(ethip_nic_t *nic, iplink_srv_addr_t *addr)
{
	ethip_link_addr_t *laddr;

	log_msg(LVL_DEBUG, "ethip_nic_addr_add()");
	laddr = ethip_nic_addr_new(addr);
	if (laddr == NULL)
		return ENOMEM;

	list_append(&laddr->addr_list, &nic->addr_list);
	return EOK;
}

int ethip_nic_addr_remove(ethip_nic_t *nic, iplink_srv_addr_t *addr)
{
	ethip_link_addr_t *laddr;

	log_msg(LVL_DEBUG, "ethip_nic_addr_remove()");

	laddr = ethip_nic_addr_find(nic, addr);
	if (laddr == NULL)
		return ENOENT;

	list_remove(&laddr->addr_list);
	ethip_link_addr_delete(laddr);
	return EOK;
}

ethip_link_addr_t *ethip_nic_addr_find(ethip_nic_t *nic,
    iplink_srv_addr_t *addr)
{
	log_msg(LVL_DEBUG, "ethip_nic_addr_find()");

	list_foreach(nic->addr_list, link) {
		ethip_link_addr_t *laddr = list_get_instance(link,
		    ethip_link_addr_t, addr_list);

		if (addr->ipv4 == laddr->addr.ipv4)
			return laddr;
	}

	return NULL;
}

/** @}
 */
