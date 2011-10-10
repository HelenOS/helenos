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

/** @addtogroup arp
 * @{
 */

/** @file
 * ARP module implementation.
 * @see arp.h
 */

#include <async.h>
#include <malloc.h>
#include <mem.h>
#include <fibril_synch.h>
#include <assert.h>
#include <stdio.h>
#include <str.h>
#include <task.h>
#include <adt/measured_strings.h>
#include <ipc/services.h>
#include <ipc/net.h>
#include <ipc/arp.h>
#include <ipc/il.h>
#include <ipc/nil.h>
#include <byteorder.h>
#include <errno.h>
#include <net/modules.h>
#include <net/device.h>
#include <net/packet.h>
#include <nil_remote.h>
#include <protocol_map.h>
#include <packet_client.h>
#include <packet_remote.h>
#include <il_remote.h>
#include <il_skel.h>
#include "arp.h"

/** ARP module name. */
#define NAME  "arp"

/** Number of microseconds to wait for an ARP reply. */
#define ARP_TRANS_WAIT  1000000

/** @name ARP operation codes definitions */
/*@{*/

/** REQUEST operation code. */
#define ARPOP_REQUEST  1

/** REPLY operation code. */
#define ARPOP_REPLY  2

/*@}*/

/** Type definition of an ARP protocol header.
 * @see arp_header
 */
typedef struct arp_header arp_header_t;

/** ARP protocol header. */
struct arp_header {
	/**
	 * Hardware type identifier.
	 * @see hardware.h
	 */
	uint16_t hardware;
	
	/** Protocol identifier. */
	uint16_t protocol;
	/** Hardware address length in bytes. */
	uint8_t hardware_length;
	/** Protocol address length in bytes. */
	uint8_t protocol_length;
	
	/**
	 * ARP packet type.
	 * @see arp_oc.h
	 */
	uint16_t operation;
} __attribute__ ((packed));

/** ARP global data. */
arp_globals_t arp_globals;

DEVICE_MAP_IMPLEMENT(arp_cache, arp_device_t);
INT_MAP_IMPLEMENT(arp_protos, arp_proto_t);
GENERIC_CHAR_MAP_IMPLEMENT(arp_addr, arp_trans_t);

static void arp_clear_trans(arp_trans_t *trans)
{
	if (trans->hw_addr) {
		free(trans->hw_addr);
		trans->hw_addr = NULL;
	}
	
	fibril_condvar_broadcast(&trans->cv);
}

static void arp_clear_addr(arp_addr_t *addresses)
{
	int count;
	
	for (count = arp_addr_count(addresses) - 1; count >= 0; count--) {
		arp_trans_t *trans = arp_addr_items_get_index(&addresses->values,
		    count);
		if (trans)
			arp_clear_trans(trans);
	}
}

/** Clear the device specific data.
 *
 * @param[in] device Device specific data.
 */
static void arp_clear_device(arp_device_t *device)
{
	int count;
	
	for (count = arp_protos_count(&device->protos) - 1; count >= 0;
	    count--) {
		arp_proto_t *proto = arp_protos_get_index(&device->protos,
		    count);
		
		if (proto) {
			if (proto->addr)
				free(proto->addr);
			
			if (proto->addr_data)
				free(proto->addr_data);
			
			arp_clear_addr(&proto->addresses);
			arp_addr_destroy(&proto->addresses, free);
		}
	}
	
	arp_protos_clear(&device->protos, free);
}

static int arp_clean_cache_req(void)
{
	int count;
	
	fibril_mutex_lock(&arp_globals.lock);
	for (count = arp_cache_count(&arp_globals.cache) - 1; count >= 0;
	    count--) {
		arp_device_t *device = arp_cache_get_index(&arp_globals.cache,
		    count);
		
		if (device)
			arp_clear_device(device);
	}
	
	arp_cache_clear(&arp_globals.cache, free);
	fibril_mutex_unlock(&arp_globals.lock);
	
	return EOK;
}

static int arp_clear_address_req(nic_device_id_t device_id,
    services_t protocol, measured_string_t *address)
{
	fibril_mutex_lock(&arp_globals.lock);
	
	arp_device_t *device = arp_cache_find(&arp_globals.cache, device_id);
	if (!device) {
		fibril_mutex_unlock(&arp_globals.lock);
		return ENOENT;
	}
	
	arp_proto_t *proto = arp_protos_find(&device->protos, protocol);
	if (!proto) {
		fibril_mutex_unlock(&arp_globals.lock);
		return ENOENT;
	}
	
	arp_trans_t *trans = arp_addr_find(&proto->addresses, address->value,
	    address->length);
	if (trans)
		arp_clear_trans(trans);
	
	arp_addr_exclude(&proto->addresses, address->value, address->length, free);
	
	fibril_mutex_unlock(&arp_globals.lock);
	return EOK;
}

static int arp_clear_device_req(nic_device_id_t device_id)
{
	fibril_mutex_lock(&arp_globals.lock);
	
	arp_device_t *device = arp_cache_find(&arp_globals.cache, device_id);
	if (!device) {
		fibril_mutex_unlock(&arp_globals.lock);
		return ENOENT;
	}
	
	arp_clear_device(device);
	
	fibril_mutex_unlock(&arp_globals.lock);
	return EOK;
}

/** Create new protocol specific data.
 *
 * Allocate and return the needed memory block as the proto parameter.
 *
 * @param[out] proto   Allocated protocol specific data.
 * @param[in]  service Protocol module service.
 * @param[in]  address Actual protocol device address.
 *
 * @return EOK on success.
 * @return ENOMEM if there is not enough memory left.
 *
 */
static int arp_proto_create(arp_proto_t **proto, services_t service,
    measured_string_t *address)
{
	*proto = (arp_proto_t *) malloc(sizeof(arp_proto_t));
	if (!*proto)
		return ENOMEM;
	
	(*proto)->service = service;
	(*proto)->addr = address;
	(*proto)->addr_data = address->value;
	
	int rc = arp_addr_initialize(&(*proto)->addresses);
	if (rc != EOK) {
		free(*proto);
		return rc;
	}
	
	return EOK;
}

/** Process the received ARP packet.
 *
 * Update the source hardware address if the source entry exists or the packet
 * is targeted to my protocol address.
 *
 * Respond to the ARP request if the packet is the ARP request and is
 * targeted to my address.
 *
 * @param[in]     device_id Source device identifier.
 * @param[in,out] packet    Received packet.
 *
 * @return EOK on success and the packet is no longer needed.
 * @return One on success and the packet has been reused.
 * @return EINVAL if the packet is too small to carry an ARP
 *         packet.
 * @return EINVAL if the received address lengths differs from
 *         the registered values.
 * @return ENOENT if the device is not found in the cache.
 * @return ENOENT if the protocol for the device is not found in
 *         the cache.
 * @return ENOMEM if there is not enough memory left.
 *
 */
static int arp_receive_message(nic_device_id_t device_id, packet_t *packet)
{
	int rc;
	
	size_t length = packet_get_data_length(packet);
	if (length <= sizeof(arp_header_t))
		return EINVAL;
	
	arp_device_t *device = arp_cache_find(&arp_globals.cache, device_id);
	if (!device)
		return ENOENT;
	
	arp_header_t *header = (arp_header_t *) packet_get_data(packet);
	if ((ntohs(header->hardware) != device->hardware) ||
	    (length < sizeof(arp_header_t) + header->hardware_length * 2U +
	    header->protocol_length * 2U)) {
		return EINVAL;
	}
	
	arp_proto_t *proto = arp_protos_find(&device->protos,
	    protocol_unmap(device->service, ntohs(header->protocol)));
	if (!proto)
		return ENOENT;
	
	uint8_t *src_hw = ((uint8_t *) header) + sizeof(arp_header_t);
	uint8_t *src_proto = src_hw + header->hardware_length;
	uint8_t *des_hw = src_proto + header->protocol_length;
	uint8_t *des_proto = des_hw + header->hardware_length;
	
	arp_trans_t *trans = arp_addr_find(&proto->addresses, src_proto,
	    header->protocol_length);
	
	if ((trans) && (trans->hw_addr)) {
		/* Translation exists */
		if (trans->hw_addr->length != header->hardware_length)
			return EINVAL;
		
		memcpy(trans->hw_addr->value, src_hw, trans->hw_addr->length);
	}
	
	/* Is my protocol address? */
	if (proto->addr->length != header->protocol_length)
		return EINVAL;
	
	if (!bcmp(proto->addr->value, des_proto, proto->addr->length)) {
		if (!trans) {
			/* Update the translation */
			trans = (arp_trans_t *) malloc(sizeof(arp_trans_t));
			if (!trans)
				return ENOMEM;
			
			trans->hw_addr = NULL;
			fibril_condvar_initialize(&trans->cv);
			rc = arp_addr_add(&proto->addresses, src_proto,
			    header->protocol_length, trans);
			if (rc != EOK) {
				free(trans);
				return rc;
			}
		}
		
		if (!trans->hw_addr) {
			trans->hw_addr = measured_string_create_bulk(src_hw,
			    header->hardware_length);
			if (!trans->hw_addr)
				return ENOMEM;
			
			/* Notify the fibrils that wait for the translation. */
			fibril_condvar_broadcast(&trans->cv);
		}
		
		if (ntohs(header->operation) == ARPOP_REQUEST) {
			header->operation = htons(ARPOP_REPLY);
			memcpy(des_proto, src_proto, header->protocol_length);
			memcpy(src_proto, proto->addr->value,
			    header->protocol_length);
			memcpy(src_hw, device->addr,
			    device->packet_dimension.addr_len);
			memcpy(des_hw, trans->hw_addr->value,
			    header->hardware_length);
			
			rc = packet_set_addr(packet, src_hw, des_hw,
			    header->hardware_length);
			if (rc != EOK)
				return rc;
			
			nil_send_msg(device->sess, device_id, packet,
			    SERVICE_ARP);
			return 1;
		}
	}
	
	return EOK;
}

/** Update the device content length according to the new MTU value.
 *
 * @param[in] device_id Device identifier.
 * @param[in] mtu       New MTU value.
 *
 * @return ENOENT if device is not found.
 * @return EOK on success.
 *
 */
static int arp_mtu_changed_message(nic_device_id_t device_id, size_t mtu)
{
	fibril_mutex_lock(&arp_globals.lock);
	
	arp_device_t *device = arp_cache_find(&arp_globals.cache, device_id);
	if (!device) {
		fibril_mutex_unlock(&arp_globals.lock);
		return ENOENT;
	}
	
	device->packet_dimension.content = mtu;
	
	fibril_mutex_unlock(&arp_globals.lock);
	
	printf("%s: Device %d changed MTU to %zu\n", NAME, device_id, mtu);
	
	return EOK;
}

static int arp_addr_changed_message(nic_device_id_t device_id)
{
	uint8_t addr_buffer[NIC_MAX_ADDRESS_LENGTH];
	size_t length;
	ipc_callid_t data_callid;
	if (!async_data_write_receive(&data_callid, &length)) {
		async_answer_0(data_callid, EINVAL);
		return EINVAL;
	}
	if (length > NIC_MAX_ADDRESS_LENGTH) {
		async_answer_0(data_callid, ELIMIT);
		return ELIMIT;
	}
	if (async_data_write_finalize(data_callid, addr_buffer, length) != EOK) {
		return EINVAL;
	}

	fibril_mutex_lock(&arp_globals.lock);

	arp_device_t *device = arp_cache_find(&arp_globals.cache, device_id);
	if (!device) {
		fibril_mutex_unlock(&arp_globals.lock);
		return ENOENT;
	}

	memcpy(device->addr, addr_buffer, length);
	device->addr_len = length;

	fibril_mutex_unlock(&arp_globals.lock);
	return EOK;
}

/** Process IPC messages from the registered device driver modules
 *
 * @param[in]     iid   Message identifier.
 * @param[in,out] icall Message parameters.
 * @param[in]     arg   Local argument.
 *
 */
static void arp_receiver(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	packet_t *packet;
	int rc;
	
	while (true) {
		switch (IPC_GET_IMETHOD(*icall)) {
		case NET_IL_DEVICE_STATE:
			/* Do nothing - keep the cache */
			async_answer_0(iid, (sysarg_t) EOK);
			break;
		
		case NET_IL_RECEIVED:
			rc = packet_translate_remote(arp_globals.net_sess, &packet,
			    IPC_GET_PACKET(*icall));
			if (rc == EOK) {
				fibril_mutex_lock(&arp_globals.lock);
				do {
					packet_t *next = pq_detach(packet);
					rc = arp_receive_message(IPC_GET_DEVICE(*icall), packet);
					if (rc != 1) {
						pq_release_remote(arp_globals.net_sess,
						    packet_get_id(packet));
					}
					
					packet = next;
				} while (packet);
				fibril_mutex_unlock(&arp_globals.lock);
			}
			async_answer_0(iid, (sysarg_t) rc);
			break;
		
		case NET_IL_MTU_CHANGED:
			rc = arp_mtu_changed_message(IPC_GET_DEVICE(*icall),
			    IPC_GET_MTU(*icall));
			async_answer_0(iid, (sysarg_t) rc);
			break;
		case NET_IL_ADDR_CHANGED:
			rc = arp_addr_changed_message(IPC_GET_DEVICE(*icall));
			async_answer_0(iid, (sysarg_t) rc);
		
		default:
			async_answer_0(iid, (sysarg_t) ENOTSUP);
		}
		
		iid = async_get_call(icall);
	}
}

/** Register the device.
 *
 * Create new device entry in the cache or update the protocol address if the
 * device with the device identifier and the driver service exists.
 *
 * @param[in] device_id Device identifier.
 * @param[in] service   Device driver service.
 * @param[in] protocol  Protocol service.
 * @param[in] address   Actual device protocol address.
 *
 * @return EOK on success.
 * @return EEXIST if another device with the same device identifier
 *         and different driver service exists.
 * @return ENOMEM if there is not enough memory left.
 * @return Other error codes as defined for the
 *         measured_strings_return() function.
 *
 */
static int arp_device_message(nic_device_id_t device_id, services_t service,
    services_t protocol, measured_string_t *address)
{
	int index;
	int rc;
	
	fibril_mutex_lock(&arp_globals.lock);
	
	/* An existing device? */
	arp_device_t *device = arp_cache_find(&arp_globals.cache, device_id);
	if (device) {
		if (device->service != service) {
			printf("%s: Device %d already exists\n", NAME,
			    device->device_id);
			fibril_mutex_unlock(&arp_globals.lock);
			return EEXIST;
		}
		
		arp_proto_t *proto = arp_protos_find(&device->protos, protocol);
		if (proto) {
			free(proto->addr);
			free(proto->addr_data);
			proto->addr = address;
			proto->addr_data = address->value;
		} else {
			rc = arp_proto_create(&proto, protocol, address);
			if (rc != EOK) {
				fibril_mutex_unlock(&arp_globals.lock);
				return rc;
			}
			
			index = arp_protos_add(&device->protos, proto->service,
			    proto);
			if (index < 0) {
				fibril_mutex_unlock(&arp_globals.lock);
				free(proto);
				return index;
			}
			
			printf("%s: New protocol added (id: %d, proto: %d)\n", NAME,
			    device_id, protocol);
		}
	} else {
		hw_type_t hardware = hardware_map(service);
		if (!hardware)
			return ENOENT;
		
		/* Create new device */
		device = (arp_device_t *) malloc(sizeof(arp_device_t));
		if (!device) {
			fibril_mutex_unlock(&arp_globals.lock);
			return ENOMEM;
		}
		
		device->hardware = hardware;
		device->device_id = device_id;
		rc = arp_protos_initialize(&device->protos);
		if (rc != EOK) {
			fibril_mutex_unlock(&arp_globals.lock);
			free(device);
			return rc;
		}
		
		arp_proto_t *proto;
		rc = arp_proto_create(&proto, protocol, address);
		if (rc != EOK) {
			fibril_mutex_unlock(&arp_globals.lock);
			free(device);
			return rc;
		}
		
		index = arp_protos_add(&device->protos, proto->service, proto);
		if (index < 0) {
			fibril_mutex_unlock(&arp_globals.lock);
			arp_protos_destroy(&device->protos, free);
			free(device);
			return index;
		}
		
		device->service = service;
		
		/* Bind */
		device->sess = nil_bind_service(device->service,
		    (sysarg_t) device->device_id, SERVICE_ARP,
		    arp_receiver);
		if (device->sess == NULL) {
			fibril_mutex_unlock(&arp_globals.lock);
			arp_protos_destroy(&device->protos, free);
			free(device);
			return EREFUSED;
		}
		
		/* Get packet dimensions */
		rc = nil_packet_size_req(device->sess, device_id,
		    &device->packet_dimension);
		if (rc != EOK) {
			fibril_mutex_unlock(&arp_globals.lock);
			arp_protos_destroy(&device->protos, free);
			free(device);
			return rc;
		}
		
		/* Get hardware address */
		int len = nil_get_addr_req(device->sess, device_id, device->addr,
		    NIC_MAX_ADDRESS_LENGTH);
		if (len < 0) {
			fibril_mutex_unlock(&arp_globals.lock);
			arp_protos_destroy(&device->protos, free);
			free(device);
			return len;
		}
		
		device->addr_len = len;
		
		/* Get broadcast address */
		len = nil_get_broadcast_addr_req(device->sess, device_id,
		    device->broadcast_addr, NIC_MAX_ADDRESS_LENGTH);
		if (len < 0) {
			fibril_mutex_unlock(&arp_globals.lock);
			arp_protos_destroy(&device->protos, free);
			free(device);
			return len;
		}
		
		device->broadcast_addr_len = len;
		
		rc = arp_cache_add(&arp_globals.cache, device->device_id,
		    device);
		if (rc != EOK) {
			fibril_mutex_unlock(&arp_globals.lock);
			arp_protos_destroy(&device->protos, free);
			free(device);
			return rc;
		}
		printf("%s: Device registered (id: %d, type: 0x%x, service: %d,"
		    " proto: %d)\n", NAME, device->device_id, device->hardware,
		    device->service, protocol);
	}
	
	fibril_mutex_unlock(&arp_globals.lock);
	return EOK;
}

int il_initialize(async_sess_t *net_sess)
{
	fibril_mutex_initialize(&arp_globals.lock);
	
	fibril_mutex_lock(&arp_globals.lock);
	arp_globals.net_sess = net_sess;
	int rc = arp_cache_initialize(&arp_globals.cache);
	fibril_mutex_unlock(&arp_globals.lock);
	
	return rc;
}

static int arp_send_request(nic_device_id_t device_id, services_t protocol,
    measured_string_t *target, arp_device_t *device, arp_proto_t *proto)
{
	/* ARP packet content size = header + (address + translation) * 2 */
	size_t length = 8 + 2 * (proto->addr->length + device->addr_len);
	if (length > device->packet_dimension.content)
		return ELIMIT;
	
	packet_t *packet = packet_get_4_remote(arp_globals.net_sess,
	    device->packet_dimension.addr_len, device->packet_dimension.prefix,
	    length, device->packet_dimension.suffix);
	if (!packet)
		return ENOMEM;
	
	arp_header_t *header = (arp_header_t *) packet_suffix(packet, length);
	if (!header) {
		pq_release_remote(arp_globals.net_sess, packet_get_id(packet));
		return ENOMEM;
	}
	
	header->hardware = htons(device->hardware);
	header->hardware_length = (uint8_t) device->addr_len;
	header->protocol = htons(protocol_map(device->service, protocol));
	header->protocol_length = (uint8_t) proto->addr->length;
	header->operation = htons(ARPOP_REQUEST);
	
	length = sizeof(arp_header_t);
	memcpy(((uint8_t *) header) + length, device->addr,
	    device->addr_len);
	length += device->addr_len;
	memcpy(((uint8_t *) header) + length, proto->addr->value,
	    proto->addr->length);
	length += proto->addr->length;
	bzero(((uint8_t *) header) + length, device->addr_len);
	length += device->addr_len;
	memcpy(((uint8_t *) header) + length, target->value, target->length);

	int rc = packet_set_addr(packet, device->addr, device->broadcast_addr,
	    device->addr_len);
	if (rc != EOK) {
		pq_release_remote(arp_globals.net_sess, packet_get_id(packet));
		return rc;
	}
	
	nil_send_msg(device->sess, device_id, packet, SERVICE_ARP);
	return EOK;
}

/** Return the hardware address for the given protocol address.
 *
 * Send the ARP request packet if the hardware address is not found in the
 * cache.
 *
 * @param[in]  device_id   Device identifier.
 * @param[in]  protocol    Protocol service.
 * @param[in]  target      Target protocol address.
 * @param[out] translation Where the hardware address of the target is stored.
 *
 * @return EOK on success.
 * @return EAGAIN if the caller should try again.
 * @return Other error codes in case of error.
 *
 */
static int arp_translate_message(nic_device_id_t device_id, services_t protocol,
    measured_string_t *target, measured_string_t **translation)
{
	bool retry = false;
	int rc;

	assert(fibril_mutex_is_locked(&arp_globals.lock));
	
restart:
	if ((!target) || (!translation))
		return EBADMEM;
	
	arp_device_t *device = arp_cache_find(&arp_globals.cache, device_id);
	if (!device)
		return ENOENT;
	
	arp_proto_t *proto = arp_protos_find(&device->protos, protocol);
	if ((!proto) || (proto->addr->length != target->length))
		return ENOENT;
	
	arp_trans_t *trans = arp_addr_find(&proto->addresses, target->value,
	    target->length);
	if (trans) {
		if (trans->hw_addr) {
			/* The translation is in place. */
			*translation = trans->hw_addr;
			return EOK;
		}
		
		if (retry) {
			/*
			 * We may get here as a result of being signalled for
			 * some reason while waiting for the translation (e.g.
			 * translation becoming available, record being removed
			 * from the table) and then losing the race for
			 * the arp_globals.lock with someone else who modified
			 * the table.
			 *
			 * Remove the incomplete record so that it is possible
			 * to make new ARP requests.
			 */
			arp_clear_trans(trans);
			arp_addr_exclude(&proto->addresses, target->value,
			    target->length, free);
			return EAGAIN;
		}
		
		/*
		 * We are a random passer-by who merely joins an already waiting
		 * fibril in waiting for the translation.
		 */
		rc = fibril_condvar_wait_timeout(&trans->cv, &arp_globals.lock,
		    ARP_TRANS_WAIT);
		if (rc == ETIMEOUT)
			return ENOENT;
		
		/*
		 * Need to recheck because we did not hold the lock while
		 * sleeping on the condition variable.
		 */
		retry = true;
		goto restart;
	}
	
	if (retry)
		return EAGAIN;

	/*
	 * We are under the protection of arp_globals.lock, so we can afford to
	 * first send the ARP request and then insert an incomplete ARP record.
	 * The incomplete record is used to tell any other potential waiter
	 * that this fibril has already sent the request and that it is waiting
	 * for the answer. Lastly, any fibril which sees the incomplete request
	 * can perform a timed wait on its condition variable to wait for the
	 * ARP reply to arrive.
	 */

	rc = arp_send_request(device_id, protocol, target, device, proto);
	if (rc != EOK)
		return rc;
	
	trans = (arp_trans_t *) malloc(sizeof(arp_trans_t));
	if (!trans)
		return ENOMEM;
	
	trans->hw_addr = NULL;
	fibril_condvar_initialize(&trans->cv);
	
	rc = arp_addr_add(&proto->addresses, target->value, target->length,
	    trans);
	if (rc != EOK) {
		free(trans);
		return rc;
	}
	
	rc = fibril_condvar_wait_timeout(&trans->cv, &arp_globals.lock,
	    ARP_TRANS_WAIT);
	if (rc == ETIMEOUT) {
		/*
		 * Remove the incomplete record so that it is possible to make 
		 * new ARP requests.
		 */
		arp_clear_trans(trans);
		arp_addr_exclude(&proto->addresses, target->value,
		    target->length, free);
		return ENOENT;
	}
	
	/*
	 * We need to recheck that the translation has indeed become available,
	 * because we dropped the arp_globals.lock while sleeping on the
	 * condition variable and someone else might have e.g. removed the
	 * translation before we managed to lock arp_globals.lock again.
	 */

	retry = true;
	goto restart;
}

/** Process the ARP message.
 *
 * @param[in]  callid Message identifier.
 * @param[in]  call   Message parameters.
 * @param[out] answer Answer.
 * @param[out] count  Number of arguments of the answer.
 *
 * @return EOK on success.
 * @return ENOTSUP if the message is not known.
 *
 * @see arp_interface.h
 * @see IS_NET_ARP_MESSAGE()
 *
 */
int il_module_message(ipc_callid_t callid, ipc_call_t *call, ipc_call_t *answer,
    size_t *count)
{
	measured_string_t *address;
	measured_string_t *translation;
	uint8_t *data;
	int rc;
	
	*count = 0;
	
	if (!IPC_GET_IMETHOD(*call))
		return EOK;
	
	switch (IPC_GET_IMETHOD(*call)) {
	case NET_ARP_DEVICE:
		rc = measured_strings_receive(&address, &data, 1);
		if (rc != EOK)
			return rc;
		
		rc = arp_device_message(IPC_GET_DEVICE(*call),
		    IPC_GET_SERVICE(*call), ARP_GET_NETIF(*call), address);
		if (rc != EOK) {
			free(address);
			free(data);
		}
		
		return rc;
	
	case NET_ARP_TRANSLATE:
		rc = measured_strings_receive(&address, &data, 1);
		if (rc != EOK)
			return rc;
		
		fibril_mutex_lock(&arp_globals.lock);
		rc = arp_translate_message(IPC_GET_DEVICE(*call),
		    IPC_GET_SERVICE(*call), address, &translation);
		free(address);
		free(data);
		
		if (rc != EOK) {
			fibril_mutex_unlock(&arp_globals.lock);
			return rc;
		}
		
		if (!translation) {
			fibril_mutex_unlock(&arp_globals.lock);
			return ENOENT;
		}
		
		rc = measured_strings_reply(translation, 1);
		fibril_mutex_unlock(&arp_globals.lock);
		return rc;
	
	case NET_ARP_CLEAR_DEVICE:
		return arp_clear_device_req(IPC_GET_DEVICE(*call));
	
	case NET_ARP_CLEAR_ADDRESS:
		rc = measured_strings_receive(&address, &data, 1);
		if (rc != EOK)
			return rc;
		
		arp_clear_address_req(IPC_GET_DEVICE(*call),
		    IPC_GET_SERVICE(*call), address);
		free(address);
		free(data);
		return EOK;
	
	case NET_ARP_CLEAN_CACHE:
		return arp_clean_cache_req();
	}
	
	return ENOTSUP;
}

int main(int argc, char *argv[])
{
	/* Start the module */
	return il_module_start(SERVICE_ARP);
}

/** @}
 */
