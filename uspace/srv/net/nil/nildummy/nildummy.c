/*
 * Copyright (c) 2009 Lukas Mejdrech
 * Copyright (c) 2011 Radim Vansa
 * Copyright (c) 2011 Jiri Svoboda
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

/** @addtogroup nildummy
 * @{
 */

/** @file
 * Dummy network interface layer module implementation.
 * @see nildummy.h
 */

#include <assert.h>
#include <async.h>
#include <malloc.h>
#include <mem.h>
#include <stdio.h>
#include <str.h>
#include <ipc/nil.h>
#include <ipc/net.h>
#include <ipc/services.h>
#include <net/modules.h>
#include <net/device.h>
#include <il_remote.h>
#include <adt/measured_strings.h>
#include <net/packet.h>
#include <packet_remote.h>
#include <packet_client.h>
#include <device/nic.h>
#include <loc.h>
#include <nil_skel.h>
#include "nildummy.h"

/** The module name. */
#define NAME  "nildummy"

/** Default maximum transmission unit. */
#define NET_DEFAULT_MTU  1500

/** Network interface layer module global data. */
nildummy_globals_t nildummy_globals;

DEVICE_MAP_IMPLEMENT(nildummy_devices, nildummy_device_t);

static void nildummy_nic_cb_conn(ipc_callid_t iid, ipc_call_t *icall,
    void *arg);

static int nildummy_device_state(nildummy_device_t *device, sysarg_t state)
{
	fibril_rwlock_read_lock(&nildummy_globals.protos_lock);
	if (nildummy_globals.proto.sess)
		il_device_state_msg(nildummy_globals.proto.sess,
		    device->device_id, state, nildummy_globals.proto.service);
	fibril_rwlock_read_unlock(&nildummy_globals.protos_lock);
	
	return EOK;
}

static int nildummy_addr_changed(nildummy_device_t *device)
{
	return ENOTSUP;
}

int nil_initialize(async_sess_t *sess)
{
	fibril_rwlock_initialize(&nildummy_globals.devices_lock);
	fibril_rwlock_initialize(&nildummy_globals.protos_lock);
	fibril_rwlock_write_lock(&nildummy_globals.devices_lock);
	fibril_rwlock_write_lock(&nildummy_globals.protos_lock);
	
	nildummy_globals.net_sess = sess;
	nildummy_globals.proto.sess = NULL;
	int rc = nildummy_devices_initialize(&nildummy_globals.devices);
	
	fibril_rwlock_write_unlock(&nildummy_globals.protos_lock);
	fibril_rwlock_write_unlock(&nildummy_globals.devices_lock);
	
	return rc;
}

/** Register new device or updates the MTU of an existing one.
 *
 * Determine the device local hardware address.
 *
 * @param[in] device_id New device identifier.
 * @param[in] service   Device driver service.
 * @param[in] mtu       Device maximum transmission unit.
 *
 * @return EOK on success.
 * @return EEXIST if the device with the different service exists.
 * @return ENOMEM if there is not enough memory left.
 * @return Other error codes as defined for the
 *         netif_bind_service() function.
 * @return Other error codes as defined for the
 *         netif_get_addr_req() function.
 *
 */
static int nildummy_device_message(nic_device_id_t device_id,
    service_id_t sid, size_t mtu)
{
	fibril_rwlock_write_lock(&nildummy_globals.devices_lock);
	
	/* An existing device? */
	nildummy_device_t *device =
	    nildummy_devices_find(&nildummy_globals.devices, device_id);
	if (device) {
		if (device->sid != sid) {
			printf("Device %d exists, handles do not match\n",
			    device->device_id);
			fibril_rwlock_write_unlock(&nildummy_globals.devices_lock);
			return EEXIST;
		}
		
		/* Update MTU */
		if (mtu > 0)
			device->mtu = mtu;
		else
			device->mtu = NET_DEFAULT_MTU;
		
		printf("Device %d already exists (mtu: %zu)\n", device->device_id,
		    device->mtu);
		fibril_rwlock_write_unlock(&nildummy_globals.devices_lock);
		
		/* Notify the upper layer module */
		fibril_rwlock_read_lock(&nildummy_globals.protos_lock);
		if (nildummy_globals.proto.sess) {
			il_mtu_changed_msg(nildummy_globals.proto.sess,
			    device->device_id, device->mtu,
			    nildummy_globals.proto.service);
		}
		fibril_rwlock_read_unlock(&nildummy_globals.protos_lock);
		
		return EOK;
	}
	
	/* Create a new device */
	device = (nildummy_device_t *) malloc(sizeof(nildummy_device_t));
	if (!device)
		return ENOMEM;
	
	device->device_id = device_id;
	device->sid = sid;
	if (mtu > 0)
		device->mtu = mtu;
	else
		device->mtu = NET_DEFAULT_MTU;
	
	/* Bind the device driver */
	device->sess = loc_service_connect(EXCHANGE_SERIALIZE, sid,
	    IPC_FLAG_BLOCKING);
	if (device->sess == NULL) {
		fibril_rwlock_write_unlock(&nildummy_globals.devices_lock);
		free(device);
		return ENOENT;
	}
	
	int rc = nic_callback_create(device->sess, nildummy_nic_cb_conn,
	    device);
	if (rc != EOK) {
		async_hangup(device->sess);
		
		return ENOENT;
	}
	
	/* Get hardware address */
	rc = nic_get_address(device->sess, &device->addr);
	if (rc != EOK) {
		fibril_rwlock_write_unlock(&nildummy_globals.devices_lock);
		free(device);
		return rc;
	}
	
	device->addr_len = ETH_ADDR;
	
	/* Add to the cache */
	int index = nildummy_devices_add(&nildummy_globals.devices,
	    device->device_id, device);
	if (index < 0) {
		fibril_rwlock_write_unlock(&nildummy_globals.devices_lock);
		free(device);
		return index;
	}
	
	printf("%s: Device registered (id: %d, mtu: %zu)\n", NAME,
	    device->device_id, device->mtu);
	fibril_rwlock_write_unlock(&nildummy_globals.devices_lock);
	return EOK;
}

/** Return the device hardware address.
 *
 * @param[in]  device_id Device identifier.
 * @param[out] address   Device hardware address.
 *
 * @return EOK on success.
 * @return EBADMEM if the address parameter is NULL.
 * @return ENOENT if there no such device.
 *
 */
static int nildummy_addr_message(nic_device_id_t device_id, size_t *addrlen)
{
	if (!addrlen)
		return EBADMEM;
	
	fibril_rwlock_read_lock(&nildummy_globals.devices_lock);
	
	nildummy_device_t *device =
	    nildummy_devices_find(&nildummy_globals.devices, device_id);
	if (!device) {
		fibril_rwlock_read_unlock(&nildummy_globals.devices_lock);
		return ENOENT;
	}
	
	ipc_callid_t callid;
	size_t max_len;
	if (!async_data_read_receive(&callid, &max_len)) {
		fibril_rwlock_read_unlock(&nildummy_globals.devices_lock);
		return EREFUSED;
	}
	
	if (max_len < device->addr_len) {
		fibril_rwlock_read_unlock(&nildummy_globals.devices_lock);
		async_data_read_finalize(callid, NULL, 0);
		return ELIMIT;
	}
	
	int rc = async_data_read_finalize(callid,
	    (uint8_t *) &device->addr.address, device->addr_len);
	if (rc != EOK) {
		fibril_rwlock_read_unlock(&nildummy_globals.devices_lock);
		return rc;
	}
	
	*addrlen = device->addr_len;
	
	fibril_rwlock_read_unlock(&nildummy_globals.devices_lock);
	return EOK;
}

/** Return the device packet dimensions for sending.
 *
 * @param[in]  device_id Device identifier.
 * @param[out] addr_len  Minimum reserved address length.
 * @param[out] prefix    Minimum reserved prefix size.
 * @param[out] content   Maximum content size.
 * @param[out] suffix    Minimum reserved suffix size.
 *
 * @return EOK on success.
 * @return EBADMEM if either one of the parameters is NULL.
 * @return ENOENT if there is no such device.
 *
 */
static int nildummy_packet_space_message(nic_device_id_t device_id,
    size_t *addr_len, size_t *prefix, size_t *content, size_t *suffix)
{
	if ((!addr_len) || (!prefix) || (!content) || (!suffix))
		return EBADMEM;
	
	fibril_rwlock_read_lock(&nildummy_globals.devices_lock);
	
	nildummy_device_t *device =
	    nildummy_devices_find(&nildummy_globals.devices, device_id);
	if (!device) {
		fibril_rwlock_read_unlock(&nildummy_globals.devices_lock);
		return ENOENT;
	}
	
	*content = device->mtu;
	
	fibril_rwlock_read_unlock(&nildummy_globals.devices_lock);
	
	*addr_len = 0;
	*prefix = 0;
	*suffix = 0;
	return EOK;
}

int nil_received_msg_local(nic_device_id_t device_id, packet_t *packet)
{
	fibril_rwlock_read_lock(&nildummy_globals.protos_lock);
	
	if (nildummy_globals.proto.sess) {
		do {
			packet_t *next = pq_detach(packet);
			il_received_msg(nildummy_globals.proto.sess, device_id,
			    packet, nildummy_globals.proto.service);
			packet = next;
		} while (packet);
	}
	
	fibril_rwlock_read_unlock(&nildummy_globals.protos_lock);
	
	return EOK;
}

/** Register receiving module service.
 *
 * Pass received packets for this service.
 *
 * @param[in] service Module service.
 * @param[in] sess    Service session.
 *
 * @return EOK on success.
 * @return ENOENT if the service is not known.
 * @return ENOMEM if there is not enough memory left.
 *
 */
static int nildummy_register_message(services_t service, async_sess_t *sess)
{
	fibril_rwlock_write_lock(&nildummy_globals.protos_lock);
	nildummy_globals.proto.service = service;
	nildummy_globals.proto.sess = sess;
	
	printf("%s: Protocol registered (service: %#x)\n",
	    NAME, nildummy_globals.proto.service);
	
	fibril_rwlock_write_unlock(&nildummy_globals.protos_lock);
	return EOK;
}

/** Send the packet queue.
 *
 * @param[in] device_id Device identifier.
 * @param[in] packet    Packet queue.
 * @param[in] sender    Sending module service.
 *
 * @return EOK on success.
 * @return ENOENT if there no such device.
 * @return EINVAL if the service parameter is not known.
 *
 */
static int nildummy_send_message(nic_device_id_t device_id, packet_t *packet,
    services_t sender)
{
	fibril_rwlock_read_lock(&nildummy_globals.devices_lock);
	
	nildummy_device_t *device =
	    nildummy_devices_find(&nildummy_globals.devices, device_id);
	if (!device) {
		fibril_rwlock_read_unlock(&nildummy_globals.devices_lock);
		return ENOENT;
	}
	
	packet_t *p = packet;
	do {
		nic_send_frame(device->sess, packet_get_data(p),
		    packet_get_data_length(p));
		p = pq_next(p);
	} while (p != NULL);
	
	pq_release_remote(nildummy_globals.net_sess, packet_get_id(packet));
	
	fibril_rwlock_read_unlock(&nildummy_globals.devices_lock);
	
	return EOK;
}

static int nildummy_received(nildummy_device_t *device)
{
	void *data;
	size_t size;
	int rc;

	rc = async_data_write_accept(&data, false, 0, 0, 0, &size);
	if (rc != EOK)
		return rc;

	packet_t *packet = packet_get_1_remote(nildummy_globals.net_sess, size);
	if (packet == NULL)
		return ENOMEM;

	void *pdata = packet_suffix(packet, size);
	memcpy(pdata, data, size);
	free(pdata);

	return nil_received_msg_local(device->device_id, packet);
}

int nil_module_message(ipc_callid_t callid, ipc_call_t *call,
    ipc_call_t *answer, size_t *answer_count)
{
	packet_t *packet;
	size_t addrlen;
	size_t prefix;
	size_t suffix;
	size_t content;
	int rc;
	
	*answer_count = 0;
	
	if (!IPC_GET_IMETHOD(*call))
		return EOK;
	
	async_sess_t *callback =
	    async_callback_receive_start(EXCHANGE_SERIALIZE, call);
	if (callback)
		return nildummy_register_message(NIL_GET_PROTO(*call), callback);
	
	switch (IPC_GET_IMETHOD(*call)) {
	case NET_NIL_DEVICE:
		return nildummy_device_message(IPC_GET_DEVICE(*call),
		    IPC_GET_DEVICE_HANDLE(*call), IPC_GET_MTU(*call));
	
	case NET_NIL_SEND:
		rc = packet_translate_remote(nildummy_globals.net_sess,
		    &packet, IPC_GET_PACKET(*call));
		if (rc != EOK)
			return rc;
		return nildummy_send_message(IPC_GET_DEVICE(*call), packet,
		    IPC_GET_SERVICE(*call));
	
	case NET_NIL_PACKET_SPACE:
		rc = nildummy_packet_space_message(IPC_GET_DEVICE(*call),
		    &addrlen, &prefix, &content, &suffix);
		if (rc != EOK)
			return rc;
		IPC_SET_ADDR(*answer, addrlen);
		IPC_SET_PREFIX(*answer, prefix);
		IPC_SET_CONTENT(*answer, content);
		IPC_SET_SUFFIX(*answer, suffix);
		*answer_count = 4;
		return EOK;
	
	case NET_NIL_ADDR:
	case NET_NIL_BROADCAST_ADDR:
		rc = nildummy_addr_message(IPC_GET_DEVICE(*call), &addrlen);
		if (rc != EOK)
			return rc;
		
		IPC_SET_ADDR(*answer, addrlen);
		*answer_count = 1;
		return rc;
	}
	
	return ENOTSUP;
}

static void nildummy_nic_cb_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	nildummy_device_t *device = (nildummy_device_t *)arg;
	int rc;
	
	async_answer_0(iid, EOK);
	
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call))
			break;
		
		switch (IPC_GET_IMETHOD(call)) {
		case NIC_EV_DEVICE_STATE:
			rc = nildummy_device_state(device, IPC_GET_ARG1(call));
			async_answer_0(callid, (sysarg_t) rc);
			break;
		case NIC_EV_RECEIVED:
			rc = nildummy_received(device);
			async_answer_0(callid, (sysarg_t) rc);
			break;
		case NIC_EV_ADDR_CHANGED:
			rc = nildummy_addr_changed(device);
			async_answer_0(callid, (sysarg_t) rc);
			break;
		default:
			async_answer_0(callid, ENOTSUP);
		}
	}
}


int main(int argc, char *argv[])
{
	/* Start the module */
	return nil_module_start(SERVICE_NILDUMMY);
}

/** @}
 */
