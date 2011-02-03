/*
 * Copyright (c) 2009 Lukas Mejdrech
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
#include <netif_remote.h>
#include <nil_skel.h>

#include "nildummy.h"

/** The module name. */
#define NAME  "nildummy"

/** Default maximum transmission unit. */
#define NET_DEFAULT_MTU  1500

/** Network interface layer module global data. */
nildummy_globals_t nildummy_globals;

DEVICE_MAP_IMPLEMENT(nildummy_devices, nildummy_device_t);

int nil_device_state_msg_local(int nil_phone, device_id_t device_id, int state)
{
	fibril_rwlock_read_lock(&nildummy_globals.protos_lock);
	if (nildummy_globals.proto.phone)
		il_device_state_msg(nildummy_globals.proto.phone, device_id,
		    state, nildummy_globals.proto.service);
	fibril_rwlock_read_unlock(&nildummy_globals.protos_lock);
	
	return EOK;
}

int nil_initialize(int net_phone)
{
	fibril_rwlock_initialize(&nildummy_globals.devices_lock);
	fibril_rwlock_initialize(&nildummy_globals.protos_lock);
	fibril_rwlock_write_lock(&nildummy_globals.devices_lock);
	fibril_rwlock_write_lock(&nildummy_globals.protos_lock);
	
	nildummy_globals.net_phone = net_phone;
	nildummy_globals.proto.phone = 0;
	int rc = nildummy_devices_initialize(&nildummy_globals.devices);
	
	fibril_rwlock_write_unlock(&nildummy_globals.protos_lock);
	fibril_rwlock_write_unlock(&nildummy_globals.devices_lock);
	
	return rc;
}

/** Process IPC messages from the registered device driver modules
 *
 * @param[in]     iid   Message identifier.
 * @param[in,out] icall Message parameters.
 *
 */
static void nildummy_receiver(ipc_callid_t iid, ipc_call_t *icall)
{
	packet_t *packet;
	int rc;
	
	while (true) {
		switch (IPC_GET_IMETHOD(*icall)) {
		case NET_NIL_DEVICE_STATE:
			rc = nil_device_state_msg_local(0,
			    IPC_GET_DEVICE(*icall), IPC_GET_STATE(*icall));
			async_answer_0(iid, (sysarg_t) rc);
			break;
		
		case NET_NIL_RECEIVED:
			rc = packet_translate_remote(nildummy_globals.net_phone,
			    &packet, IPC_GET_PACKET(*icall));
			if (rc == EOK)
				rc = nil_received_msg_local(0,
				    IPC_GET_DEVICE(*icall), packet, 0);
			
			async_answer_0(iid, (sysarg_t) rc);
			break;
		
		default:
			async_answer_0(iid, (sysarg_t) ENOTSUP);
		}
		
		iid = async_get_call(icall);
	}
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
static int nildummy_device_message(device_id_t device_id, services_t service,
    size_t mtu)
{
	fibril_rwlock_write_lock(&nildummy_globals.devices_lock);
	
	/* An existing device? */
	nildummy_device_t *device =
	    nildummy_devices_find(&nildummy_globals.devices, device_id);
	if (device) {
		if (device->service != service) {
			printf("Device %d already exists\n", device->device_id);
			fibril_rwlock_write_unlock(
			    &nildummy_globals.devices_lock);
			return EEXIST;
		}
		
		/* Update MTU */
		if (mtu > 0)
			device->mtu = mtu;
		else
			device->mtu = NET_DEFAULT_MTU;
		
		printf("Device %d already exists:\tMTU\t= %zu\n",
		    device->device_id, device->mtu);
		fibril_rwlock_write_unlock(&nildummy_globals.devices_lock);
		
		/* Notify the upper layer module */
		fibril_rwlock_read_lock(&nildummy_globals.protos_lock);
		if (nildummy_globals.proto.phone) {
			il_mtu_changed_msg(nildummy_globals.proto.phone,
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
	device->service = service;
	if (mtu > 0)
		device->mtu = mtu;
	else
		device->mtu = NET_DEFAULT_MTU;

	/* Bind the device driver */
	device->phone = netif_bind_service(device->service, device->device_id,
	    SERVICE_ETHERNET, nildummy_receiver);
	if (device->phone < 0) {
		fibril_rwlock_write_unlock(&nildummy_globals.devices_lock);
		free(device);
		return device->phone;
	}
	
	/* Get hardware address */
	int rc = netif_get_addr_req(device->phone, device->device_id,
	    &device->addr, &device->addr_data);
	if (rc != EOK) {
		fibril_rwlock_write_unlock(&nildummy_globals.devices_lock);
		free(device);
		return rc;
	}
	
	/* Add to the cache */
	int index = nildummy_devices_add(&nildummy_globals.devices,
	    device->device_id, device);
	if (index < 0) {
		fibril_rwlock_write_unlock(&nildummy_globals.devices_lock);
		free(device->addr);
		free(device->addr_data);
		free(device);
		return index;
	}
	
	printf("%s: Device registered (id: %d, service: %d, mtu: %zu)\n",
	    NAME, device->device_id, device->service, device->mtu);
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
static int nildummy_addr_message(device_id_t device_id,
    measured_string_t **address)
{
	if (!address)
		return EBADMEM;
	
	fibril_rwlock_read_lock(&nildummy_globals.devices_lock);
	
	nildummy_device_t *device =
	    nildummy_devices_find(&nildummy_globals.devices, device_id);
	if (!device) {
		fibril_rwlock_read_unlock(&nildummy_globals.devices_lock);
		return ENOENT;
	}
	
	*address = device->addr;
	
	fibril_rwlock_read_unlock(&nildummy_globals.devices_lock);
	
	return (*address) ? EOK : ENOENT;
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
static int nildummy_packet_space_message(device_id_t device_id, size_t *addr_len,
    size_t *prefix, size_t *content, size_t *suffix)
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

int nil_received_msg_local(int nil_phone, device_id_t device_id,
    packet_t *packet, services_t target)
{
	fibril_rwlock_read_lock(&nildummy_globals.protos_lock);
	
	if (nildummy_globals.proto.phone) {
		do {
			packet_t *next = pq_detach(packet);
			il_received_msg(nildummy_globals.proto.phone, device_id,
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
 * @param[in] phone   Service phone.
 *
 * @return EOK on success.
 * @return ENOENT if the service is not known.
 * @return ENOMEM if there is not enough memory left.
 *
 */
static int nildummy_register_message(services_t service, int phone)
{
	fibril_rwlock_write_lock(&nildummy_globals.protos_lock);
	nildummy_globals.proto.service = service;
	nildummy_globals.proto.phone = phone;
	
	printf("%s: Protocol registered (service: %d, phone: %d)\n",
	    NAME, nildummy_globals.proto.service, nildummy_globals.proto.phone);
	
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
static int nildummy_send_message(device_id_t device_id, packet_t *packet,
    services_t sender)
{
	fibril_rwlock_read_lock(&nildummy_globals.devices_lock);
	
	nildummy_device_t *device =
	    nildummy_devices_find(&nildummy_globals.devices, device_id);
	if (!device) {
		fibril_rwlock_read_unlock(&nildummy_globals.devices_lock);
		return ENOENT;
	}
	
	/* Send packet queue */
	if (packet)
		netif_send_msg(device->phone, device_id, packet,
		    SERVICE_NILDUMMY);
	
	fibril_rwlock_read_unlock(&nildummy_globals.devices_lock);
	
	return EOK;
}

int nil_module_message(ipc_callid_t callid, ipc_call_t *call,
    ipc_call_t *answer, size_t *answer_count)
{
	measured_string_t *address;
	packet_t *packet;
	size_t addrlen;
	size_t prefix;
	size_t suffix;
	size_t content;
	int rc;
	
	*answer_count = 0;
	switch (IPC_GET_IMETHOD(*call)) {
	case IPC_M_PHONE_HUNGUP:
		return EOK;
	
	case NET_NIL_DEVICE:
		return nildummy_device_message(IPC_GET_DEVICE(*call),
		    IPC_GET_SERVICE(*call), IPC_GET_MTU(*call));
	
	case NET_NIL_SEND:
		rc = packet_translate_remote(nildummy_globals.net_phone,
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
		rc = nildummy_addr_message(IPC_GET_DEVICE(*call), &address);
		if (rc != EOK)
			return rc;
		return measured_strings_reply(address, 1);
	
	case NET_NIL_BROADCAST_ADDR:
		rc = nildummy_addr_message(IPC_GET_DEVICE(*call), &address);
		if (rc != EOK)
			return rc;
		return measured_strings_reply(address, 1);
	
	case IPC_M_CONNECT_TO_ME:
		return nildummy_register_message(NIL_GET_PROTO(*call),
		    IPC_GET_PHONE(*call));
	}
	
	return ENOTSUP;
}

int main(int argc, char *argv[])
{
	/* Start the module */
	return nil_module_start(SERVICE_NILDUMMY);
}

/** @}
 */
