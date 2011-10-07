/*
 * Copyright (c) 2009 Lukas Mejdrech
 * Copyright (c) 2011 Radim Vansa
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

/** @addtogroup eth
 *  @{
 */

/** @file
 *  Ethernet module implementation.
 *  @see eth.h
 */

#include <assert.h>
#include <async.h>
#include <malloc.h>
#include <mem.h>
#include <stdio.h>
#include <byteorder.h>
#include <str.h>
#include <errno.h>
#include <ipc/nil.h>
#include <ipc/net.h>
#include <ipc/services.h>
#include <net/modules.h>
#include <net_checksum.h>
#include <ethernet_lsap.h>
#include <ethernet_protocols.h>
#include <protocol_map.h>
#include <net/device.h>
#include <net_interface.h>
#include <il_remote.h>
#include <adt/measured_strings.h>
#include <packet_client.h>
#include <packet_remote.h>
#include <device/nic.h>
#include <nil_skel.h>
#include "eth.h"

/** The module name. */
#define NAME  "eth"

/** Reserved packet prefix length. */
#define ETH_PREFIX \
	(sizeof(eth_header_t) + sizeof(eth_header_lsap_t) + \
	    sizeof(eth_header_snap_t))

/** Reserved packet suffix length. */
#define ETH_SUFFIX  (sizeof(eth_fcs_t))

/** Maximum packet content length. */
#define ETH_MAX_CONTENT  1500u

/** Minimum packet content length. */
#define ETH_MIN_CONTENT  46u

/** Maximum tagged packet content length. */
#define ETH_MAX_TAGGED_CONTENT(flags) \
	(ETH_MAX_CONTENT - \
	    ((IS_8023_2_LSAP(flags) || IS_8023_2_SNAP(flags)) ? \
	    sizeof(eth_header_lsap_t) : 0) - \
	    (IS_8023_2_SNAP(flags) ? sizeof(eth_header_snap_t) : 0))

/** Minimum tagged packet content length. */
#define ETH_MIN_TAGGED_CONTENT(flags) \
	(ETH_MIN_CONTENT - \
	    ((IS_8023_2_LSAP(flags) || IS_8023_2_SNAP(flags)) ? \
	    sizeof(eth_header_lsap_t) : 0) - \
	    (IS_8023_2_SNAP(flags) ? sizeof(eth_header_snap_t) : 0))

/** Dummy flag shift value. */
#define ETH_DUMMY_SHIFT  0

/** Mode flag shift value. */
#define ETH_MODE_SHIFT  1

/** Dummy device flag.
 * Preamble and FCS are mandatory part of the packets.
 */
#define ETH_DUMMY  (1 << ETH_DUMMY_SHIFT)

/** Returns the dummy flag.
 * @see ETH_DUMMY
 */
#define IS_DUMMY(flags)  ((flags) & ETH_DUMMY)

/** Device mode flags.
 * @see ETH_DIX
 * @see ETH_8023_2_LSAP
 * @see ETH_8023_2_SNAP
 */
#define ETH_MODE_MASK  (3 << ETH_MODE_SHIFT)

/** DIX Ethernet mode flag. */
#define ETH_DIX  (1 << ETH_MODE_SHIFT)

/** Return whether the DIX Ethernet mode flag is set.
 *
 * @param[in] flags Ethernet flags.
 * @see ETH_DIX
 *
 */
#define IS_DIX(flags)  (((flags) & ETH_MODE_MASK) == ETH_DIX)

/** 802.3 + 802.2 + LSAP mode flag. */
#define ETH_8023_2_LSAP  (2 << ETH_MODE_SHIFT)

/** Return whether the 802.3 + 802.2 + LSAP mode flag is set.
 *
 * @param[in] flags Ethernet flags.
 * @see ETH_8023_2_LSAP
 *
 */
#define IS_8023_2_LSAP(flags)  (((flags) & ETH_MODE_MASK) == ETH_8023_2_LSAP)

/** 802.3 + 802.2 + LSAP + SNAP mode flag. */
#define ETH_8023_2_SNAP  (3 << ETH_MODE_SHIFT)

/** Return whether the 802.3 + 802.2 + LSAP + SNAP mode flag is set.
 *
 * @param[in] flags Ethernet flags.
 * @see ETH_8023_2_SNAP
 *
 */
#define IS_8023_2_SNAP(flags)  (((flags) & ETH_MODE_MASK) == ETH_8023_2_SNAP)

/** Type definition of the ethernet address type.
 * @see eth_addr_type
 */
typedef enum eth_addr_type eth_addr_type_t;

/** Ethernet address type. */
enum eth_addr_type {
	/** Local address. */
	ETH_LOCAL_ADDR,
	/** Broadcast address. */
	ETH_BROADCAST_ADDR
};

/** Ethernet module global data. */
eth_globals_t eth_globals;

DEVICE_MAP_IMPLEMENT(eth_devices, eth_device_t);
INT_MAP_IMPLEMENT(eth_protos, eth_proto_t);

int nil_device_state_msg_local(nic_device_id_t device_id, sysarg_t state)
{
	int index;
	eth_proto_t *proto;

	fibril_rwlock_read_lock(&eth_globals.protos_lock);
	for (index = eth_protos_count(&eth_globals.protos) - 1; index >= 0;
	    index--) {
		proto = eth_protos_get_index(&eth_globals.protos, index);
		if ((proto) && (proto->sess)) {
			il_device_state_msg(proto->sess, device_id, state,
			    proto->service);
		}
	}
	fibril_rwlock_read_unlock(&eth_globals.protos_lock);
	
	return EOK;
}

int nil_initialize(async_sess_t *sess)
{
	int rc;

	fibril_rwlock_initialize(&eth_globals.devices_lock);
	fibril_rwlock_initialize(&eth_globals.protos_lock);
	
	fibril_rwlock_write_lock(&eth_globals.devices_lock);
	fibril_rwlock_write_lock(&eth_globals.protos_lock);
	eth_globals.net_sess = sess;
	memcpy(eth_globals.broadcast_addr, "\xFF\xFF\xFF\xFF\xFF\xFF",
			ETH_ADDR);

	rc = eth_devices_initialize(&eth_globals.devices);
	if (rc != EOK) {
		free(eth_globals.broadcast_addr);
		goto out;
	}

	rc = eth_protos_initialize(&eth_globals.protos);
	if (rc != EOK) {
		free(eth_globals.broadcast_addr);
		eth_devices_destroy(&eth_globals.devices, free);
	}
	
out:
	fibril_rwlock_write_unlock(&eth_globals.protos_lock);
	fibril_rwlock_write_unlock(&eth_globals.devices_lock);
	
	return rc;
}

/** Register new device or updates the MTU of an existing one.
 *
 * Determine the device local hardware address.
 *
 * @param[in] device_id New device identifier.
 * @param[in] handle    Device driver handle.
 * @param[in] mtu       Device maximum transmission unit.
 *
 * @return EOK on success.
 * @return EEXIST if the device with the different service exists.
 * @return ENOMEM if there is not enough memory left.
 *
 */
static int eth_device_message(nic_device_id_t device_id, devman_handle_t handle,
    size_t mtu)
{
	eth_device_t *device;
	int index;
	measured_string_t names[2] = {
		{
			(uint8_t *) "ETH_MODE",
			8
		},
		{
			(uint8_t *) "ETH_DUMMY",
			9
		}
	};
	measured_string_t *configuration;
	size_t count = sizeof(names) / sizeof(measured_string_t);
	uint8_t *data;
	eth_proto_t *proto;
	int rc;

	fibril_rwlock_write_lock(&eth_globals.devices_lock);
	/* An existing device? */
	device = eth_devices_find(&eth_globals.devices, device_id);
	if (device) {
		if (device->handle != handle) {
			printf("Device %d already exists\n", device->device_id);
			fibril_rwlock_write_unlock(&eth_globals.devices_lock);
			return EEXIST;
		}
		
		/* Update mtu */
		if ((mtu > 0) && (mtu <= ETH_MAX_TAGGED_CONTENT(device->flags)))
			device->mtu = mtu;
		else
			device->mtu = ETH_MAX_TAGGED_CONTENT(device->flags);
		
		printf("Device %d already exists:\tMTU\t= %zu\n",
		    device->device_id, device->mtu);
		fibril_rwlock_write_unlock(&eth_globals.devices_lock);
		
		/* Notify all upper layer modules */
		fibril_rwlock_read_lock(&eth_globals.protos_lock);
		for (index = 0; index < eth_protos_count(&eth_globals.protos);
		    index++) {
			proto = eth_protos_get_index(&eth_globals.protos,
			    index);
			if (proto->sess) {
				il_mtu_changed_msg(proto->sess,
				    device->device_id, device->mtu,
				    proto->service);
			}
		}

		fibril_rwlock_read_unlock(&eth_globals.protos_lock);
		return EOK;
	}
	
	/* Create a new device */
	device = (eth_device_t *) malloc(sizeof(eth_device_t));
	if (!device)
		return ENOMEM;

	device->device_id = device_id;
	device->handle = handle;
	device->flags = 0;
	if ((mtu > 0) && (mtu <= ETH_MAX_TAGGED_CONTENT(device->flags)))
		device->mtu = mtu;
	else
		device->mtu = ETH_MAX_TAGGED_CONTENT(device->flags);

	configuration = &names[0];
	rc = net_get_device_conf_req(eth_globals.net_sess, device->device_id,
	    &configuration, count, &data);
	if (rc != EOK) {
		fibril_rwlock_write_unlock(&eth_globals.devices_lock);
		free(device);
		return rc;
	}

	if (configuration) {
		if (!str_lcmp((char *) configuration[0].value, "DIX",
		    configuration[0].length)) {
			device->flags |= ETH_DIX;
		} else if(!str_lcmp((char *) configuration[0].value, "8023_2_LSAP",
		    configuration[0].length)) {
			device->flags |= ETH_8023_2_LSAP;
		} else {
			device->flags |= ETH_8023_2_SNAP;
		}
		
		if (configuration[1].value &&
		    (configuration[1].value[0] == 'y')) {
			device->flags |= ETH_DUMMY;
		}
		net_free_settings(configuration, data);
	} else {
		device->flags |= ETH_8023_2_SNAP;
	}
	
	/* Bind the device driver */
	device->sess = devman_device_connect(EXCHANGE_SERIALIZE, handle,
	    IPC_FLAG_BLOCKING);
	if (device->sess == NULL) {
		fibril_rwlock_write_unlock(&eth_globals.devices_lock);
		free(device);
		return ENOENT;
	}
	
	nic_connect_to_nil(device->sess, SERVICE_ETHERNET, device_id);
	
	/* Get hardware address */
	rc = nic_get_address(device->sess, &device->addr);
	if (rc != EOK) {
		fibril_rwlock_write_unlock(&eth_globals.devices_lock);
		free(device);
		return rc;
	}
	
	/* Add to the cache */
	index = eth_devices_add(&eth_globals.devices, device->device_id,
	    device);
	if (index < 0) {
		fibril_rwlock_write_unlock(&eth_globals.devices_lock);
		free(device);
		return index;
	}
	
	printf("%s: Device registered (id: %d, handle: %zu: mtu: %zu, "
	    "mac: " PRIMAC ", flags: 0x%x)\n", NAME,
	    device->device_id, device->handle, device->mtu,
	    ARGSMAC(device->addr.address), device->flags);

	fibril_rwlock_write_unlock(&eth_globals.devices_lock);
	return EOK;
}

/** Processes the received packet and chooses the target registered module.
 *
 * @param[in] flags	The device flags.
 * @param[in] packet	The packet.
 * @return		The target registered module.
 * @return		NULL if the packet is not long enough.
 * @return		NULL if the packet is too long.
 * @return		NULL if the raw ethernet protocol is used.
 * @return		NULL if the dummy device FCS checksum is invalid.
 * @return		NULL if the packet address length is not big enough.
 */
static eth_proto_t *eth_process_packet(int flags, packet_t *packet)
{
	eth_header_snap_t *header;
	size_t length;
	eth_type_t type;
	size_t prefix;
	size_t suffix;
	eth_fcs_t *fcs;
	uint8_t *data;
	int rc;

	length = packet_get_data_length(packet);
	
	if (IS_DUMMY(flags))
		packet_trim(packet, sizeof(eth_preamble_t), 0);
	if (length < sizeof(eth_header_t) + ETH_MIN_CONTENT +
	    (IS_DUMMY(flags) ? ETH_SUFFIX : 0))
		return NULL;
	
	data = packet_get_data(packet);
	header = (eth_header_snap_t *) data;
	type = ntohs(header->header.ethertype);
	
	if (type >= ETH_MIN_PROTO) {
		/* DIX Ethernet */
		prefix = sizeof(eth_header_t);
		suffix = 0;
		fcs = (eth_fcs_t *) data + length - sizeof(eth_fcs_t);
		length -= sizeof(eth_fcs_t);
	} else if (type <= ETH_MAX_CONTENT) {
		/* Translate "LSAP" values */
		if ((header->lsap.dsap == ETH_LSAP_GLSAP) &&
		    (header->lsap.ssap == ETH_LSAP_GLSAP)) {
			/* Raw packet -- discard */
			return NULL;
		} else if ((header->lsap.dsap == ETH_LSAP_SNAP) &&
		    (header->lsap.ssap == ETH_LSAP_SNAP)) {
			/*
			 * IEEE 802.3 + 802.2 + LSAP + SNAP
			 * organization code not supported
			 */
			type = ntohs(header->snap.ethertype);
			prefix = sizeof(eth_header_t) + sizeof(eth_header_lsap_t) +
			    sizeof(eth_header_snap_t);
		} else {
			/* IEEE 802.3 + 802.2 LSAP */
			type = lsap_map(header->lsap.dsap);
			prefix = sizeof(eth_header_t) + sizeof(eth_header_lsap_t);
		}

		suffix = (type < ETH_MIN_CONTENT) ? ETH_MIN_CONTENT - type : 0U;
		fcs = (eth_fcs_t *) data + prefix + type + suffix;
		suffix += length - prefix - type;
		length = prefix + type + suffix;
	} else {
		/* Invalid length/type, should not occur */
		return NULL;
	}
	
	if (IS_DUMMY(flags)) {
		if (~compute_crc32(~0U, data, length * 8) != ntohl(*fcs))
			return NULL;
		suffix += sizeof(eth_fcs_t);
	}
	
	rc = packet_set_addr(packet, header->header.source_address,
	    header->header.destination_address, ETH_ADDR);
	if (rc != EOK)
		return NULL;

	rc = packet_trim(packet, prefix, suffix);
	if (rc != EOK)
		return NULL;
	
	return eth_protos_find(&eth_globals.protos, type);
}

int nil_received_msg_local(nic_device_id_t device_id, packet_t *packet)
{
	eth_proto_t *proto;
	packet_t *next;
	eth_device_t *device;
	int flags;

	fibril_rwlock_read_lock(&eth_globals.devices_lock);
	device = eth_devices_find(&eth_globals.devices, device_id);
	if (!device) {
		fibril_rwlock_read_unlock(&eth_globals.devices_lock);
		return ENOENT;
	}

	flags = device->flags;
	fibril_rwlock_read_unlock(&eth_globals.devices_lock);
	fibril_rwlock_read_lock(&eth_globals.protos_lock);
	
	do {
		next = pq_detach(packet);
		proto = eth_process_packet(flags, packet);
		if (proto) {
			il_received_msg(proto->sess, device_id, packet,
			    proto->service);
		} else {
			/* Drop invalid/unknown */
			pq_release_remote(eth_globals.net_sess,
			    packet_get_id(packet));
		}
		packet = next;
	} while (packet);

	fibril_rwlock_read_unlock(&eth_globals.protos_lock);
	return EOK;
}

/** Returns the device packet dimensions for sending.
 *
 * @param[in] device_id	The device identifier.
 * @param[out] addr_len	The minimum reserved address length.
 * @param[out] prefix	The minimum reserved prefix size.
 * @param[out] content	The maximum content size.
 * @param[out] suffix	The minimum reserved suffix size.
 * @return		EOK on success.
 * @return		EBADMEM if either one of the parameters is NULL.
 * @return		ENOENT if there is no such device.
 */
static int eth_packet_space_message(nic_device_id_t device_id, size_t *addr_len,
    size_t *prefix, size_t *content, size_t *suffix)
{
	eth_device_t *device;

	if (!addr_len || !prefix || !content || !suffix)
		return EBADMEM;
	
	fibril_rwlock_read_lock(&eth_globals.devices_lock);
	device = eth_devices_find(&eth_globals.devices, device_id);
	if (!device) {
		fibril_rwlock_read_unlock(&eth_globals.devices_lock);
		return ENOENT;
	}

	*content = device->mtu;
	fibril_rwlock_read_unlock(&eth_globals.devices_lock);
	
	*addr_len = ETH_ADDR;
	*prefix = ETH_PREFIX;
	*suffix = ETH_MIN_CONTENT + ETH_SUFFIX;

	return EOK;
}

/** Send the device hardware address.
 *
 * @param[in] device_id	The device identifier.
 * @param[in] type	Type of the desired address.
 * @return		EOK on success.
 * @return		EBADMEM if the address parameter is NULL.
 * @return		ENOENT if there no such device.
 */
static int eth_addr_message(nic_device_id_t device_id, eth_addr_type_t type)
{
	eth_device_t *device = NULL;
	uint8_t *address;
	size_t max_len;
	ipc_callid_t callid;
	
	if (type == ETH_BROADCAST_ADDR)
		address = eth_globals.broadcast_addr;
	else {
		fibril_rwlock_read_lock(&eth_globals.devices_lock);
		device = eth_devices_find(&eth_globals.devices, device_id);
		if (!device) {
			fibril_rwlock_read_unlock(&eth_globals.devices_lock);
			return ENOENT;
		}
		
		address = (uint8_t *) &device->addr.address;
	}
	
	int rc = EOK;
	if (!async_data_read_receive(&callid, &max_len)) {
		rc = EREFUSED;
		goto end;
	}
	
	if (max_len < ETH_ADDR) {
		async_data_read_finalize(callid, NULL, 0);
		rc = ELIMIT;
		goto end;
	}
	
	rc = async_data_read_finalize(callid, address, ETH_ADDR);
	if (rc != EOK)
		goto end;
	
end:
	
	if (type == ETH_LOCAL_ADDR)
		fibril_rwlock_read_unlock(&eth_globals.devices_lock);
	
	return rc;
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
static int eth_register_message(services_t service, async_sess_t *sess)
{
	eth_proto_t *proto;
	int protocol;
	int index;

	protocol = protocol_map(SERVICE_ETHERNET, service);
	if (!protocol)
		return ENOENT;

	fibril_rwlock_write_lock(&eth_globals.protos_lock);
	proto = eth_protos_find(&eth_globals.protos, protocol);
	if (proto) {
		proto->sess = sess;
		fibril_rwlock_write_unlock(&eth_globals.protos_lock);
		return EOK;
	} else {
		proto = (eth_proto_t *) malloc(sizeof(eth_proto_t));
		if (!proto) {
			fibril_rwlock_write_unlock(&eth_globals.protos_lock);
			return ENOMEM;
		}

		proto->service = service;
		proto->protocol = protocol;
		proto->sess = sess;

		index = eth_protos_add(&eth_globals.protos, protocol, proto);
		if (index < 0) {
			fibril_rwlock_write_unlock(&eth_globals.protos_lock);
			free(proto);
			return index;
		}
	}
	
	printf("%s: Protocol registered (protocol: %d, service: %#x)\n",
	    NAME, proto->protocol, proto->service);
	
	fibril_rwlock_write_unlock(&eth_globals.protos_lock);
	return EOK;
}

/** Prepares the packet for sending.
 *
 * @param[in] flags	The device flags.
 * @param[in] packet	The packet.
 * @param[in] src_addr	The source hardware address.
 * @param[in] ethertype	The ethernet protocol type.
 * @param[in] mtu	The device maximum transmission unit.
 * @return		EOK on success.
 * @return		EINVAL if the packet addresses length is not long
 *			enough.
 * @return		EINVAL if the packet is bigger than the device MTU.
 * @return		ENOMEM if there is not enough memory in the packet.
 */
static int
eth_prepare_packet(int flags, packet_t *packet, uint8_t *src_addr, int ethertype,
    size_t mtu)
{
	eth_header_snap_t *header;
	eth_header_lsap_t *header_lsap;
	eth_header_t *header_dix;
	eth_fcs_t *fcs;
	uint8_t *src;
	uint8_t *dest;
	size_t length;
	int i;
	void *padding;
	eth_preamble_t *preamble;

	i = packet_get_addr(packet, &src, &dest);
	if (i < 0)
		return i;
	
	if (i != ETH_ADDR)
		return EINVAL;
	
	for (i = 0; i < ETH_ADDR; i++) {
		if (src[i]) {
			src_addr = src;
			break;
		}
	}

	length = packet_get_data_length(packet);
	if (length > mtu)
		return EINVAL;
	
	if (length < ETH_MIN_TAGGED_CONTENT(flags)) {
		padding = packet_suffix(packet,
		    ETH_MIN_TAGGED_CONTENT(flags) - length);
		if (!padding)
			return ENOMEM;

		bzero(padding, ETH_MIN_TAGGED_CONTENT(flags) - length);
	}
	
	if (IS_DIX(flags)) {
		header_dix = PACKET_PREFIX(packet, eth_header_t);
		if (!header_dix)
			return ENOMEM;
		
		header_dix->ethertype = (uint16_t) ethertype;
		memcpy(header_dix->source_address, src_addr, ETH_ADDR);
		memcpy(header_dix->destination_address, dest, ETH_ADDR);
		src = &header_dix->destination_address[0];
	} else if (IS_8023_2_LSAP(flags)) {
		header_lsap = PACKET_PREFIX(packet, eth_header_lsap_t);
		if (!header_lsap)
			return ENOMEM;
		
		header_lsap->header.ethertype = htons(length +
		    sizeof(eth_header_lsap_t));
		header_lsap->lsap.dsap = lsap_unmap(ntohs(ethertype));
		header_lsap->lsap.ssap = header_lsap->lsap.dsap;
		header_lsap->lsap.ctrl = IEEE_8023_2_UI;
		memcpy(header_lsap->header.source_address, src_addr, ETH_ADDR);
		memcpy(header_lsap->header.destination_address, dest, ETH_ADDR);
		src = &header_lsap->header.destination_address[0];
	} else if (IS_8023_2_SNAP(flags)) {
		header = PACKET_PREFIX(packet, eth_header_snap_t);
		if (!header)
			return ENOMEM;
		
		header->header.ethertype = htons(length +
		    sizeof(eth_header_lsap_t) + sizeof(eth_header_snap_t));
		header->lsap.dsap = (uint16_t) ETH_LSAP_SNAP;
		header->lsap.ssap = header->lsap.dsap;
		header->lsap.ctrl = IEEE_8023_2_UI;
		
		for (i = 0; i < 3; i++)
			header->snap.protocol[i] = 0;
		
		header->snap.ethertype = (uint16_t) ethertype;
		memcpy(header->header.source_address, src_addr, ETH_ADDR);
		memcpy(header->header.destination_address, dest, ETH_ADDR);
		src = &header->header.destination_address[0];
	}
	
	if (IS_DUMMY(flags)) {
		preamble = PACKET_PREFIX(packet, eth_preamble_t);
		if (!preamble)
			return ENOMEM;
		
		for (i = 0; i < 7; i++)
			preamble->preamble[i] = ETH_PREAMBLE;
		
		preamble->sfd = ETH_SFD;
		
		fcs = PACKET_SUFFIX(packet, eth_fcs_t);
		if (!fcs)
			return ENOMEM;

		*fcs = htonl(~compute_crc32(~0U, src, length * 8));
	}
	
	return EOK;
}

/** Sends the packet queue.
 *
 * Sends only packet successfully processed by the eth_prepare_packet()
 * function.
 *
 * @param[in] device_id	The device identifier.
 * @param[in] packet	The packet queue.
 * @param[in] sender	The sending module service.
 * @return		EOK on success.
 * @return		ENOENT if there no such device.
 * @return		EINVAL if the service parameter is not known.
 */
static int eth_send_message(nic_device_id_t device_id, packet_t *packet,
    services_t sender)
{
	eth_device_t *device;
	packet_t *next;
	packet_t *tmp;
	int ethertype;
	int rc;

	ethertype = htons(protocol_map(SERVICE_ETHERNET, sender));
	if (!ethertype) {
		pq_release_remote(eth_globals.net_sess, packet_get_id(packet));
		return EINVAL;
	}
	
	fibril_rwlock_read_lock(&eth_globals.devices_lock);
	device = eth_devices_find(&eth_globals.devices, device_id);
	if (!device) {
		fibril_rwlock_read_unlock(&eth_globals.devices_lock);
		return ENOENT;
	}
	
	/* Process packet queue */
	next = packet;
	do {
		rc = eth_prepare_packet(device->flags, next,
		    (uint8_t *) &device->addr.address, ethertype, device->mtu);
		if (rc != EOK) {
			/* Release invalid packet */
			tmp = pq_detach(next);
			if (next == packet)
				packet = tmp;
			pq_release_remote(eth_globals.net_sess,
			    packet_get_id(next));
			next = tmp;
		} else {
			next = pq_next(next);
		}
	} while (next);
	
	/* Send packet queue */
	if (packet)
		nic_send_message(device->sess, packet_get_id(packet));
	
	fibril_rwlock_read_unlock(&eth_globals.devices_lock);
	return EOK;
}

static int eth_addr_changed(nic_device_id_t device_id)
{
	nic_address_t address;
	size_t length;
	ipc_callid_t data_callid;
	if (!async_data_write_receive(&data_callid, &length)) {
		async_answer_0(data_callid, EINVAL);
		return EINVAL;
	}
	if (length > sizeof (nic_address_t)) {
		async_answer_0(data_callid, ELIMIT);
		return ELIMIT;
	}
	if (async_data_write_finalize(data_callid, &address, length) != EOK) {
		return EINVAL;
	}

	fibril_rwlock_write_lock(&eth_globals.devices_lock);
	/* An existing device? */
	eth_device_t *device = eth_devices_find(&eth_globals.devices, device_id);
	if (device) {
		printf("Device %d changing address from " PRIMAC " to " PRIMAC "\n",
			device_id, ARGSMAC(device->addr.address), ARGSMAC(address.address));
		memcpy(&device->addr, &address, sizeof (nic_address_t));
		fibril_rwlock_write_unlock(&eth_globals.devices_lock);

		/* Notify all upper layer modules */
		fibril_rwlock_read_lock(&eth_globals.protos_lock);
		int index;
		for (index = 0; index < eth_protos_count(&eth_globals.protos); index++) {
			eth_proto_t *proto = eth_protos_get_index(&eth_globals.protos, index);
			if (proto->sess != NULL) {
				il_addr_changed_msg(proto->sess, device->device_id,
						ETH_ADDR, address.address);
			}
		}

		fibril_rwlock_read_unlock(&eth_globals.protos_lock);
		return EOK;
	} else {
		return ENOENT;
	}
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
		return eth_register_message(NIL_GET_PROTO(*call), callback);
	
	switch (IPC_GET_IMETHOD(*call)) {
	case NET_NIL_DEVICE:
		return eth_device_message(IPC_GET_DEVICE(*call),
		    IPC_GET_DEVICE_HANDLE(*call), IPC_GET_MTU(*call));
	case NET_NIL_SEND:
		rc = packet_translate_remote(eth_globals.net_sess, &packet,
		    IPC_GET_PACKET(*call));
		if (rc != EOK)
			return rc;
		
		return eth_send_message(IPC_GET_DEVICE(*call), packet,
		    IPC_GET_SERVICE(*call));
	case NET_NIL_PACKET_SPACE:
		rc = eth_packet_space_message(IPC_GET_DEVICE(*call), &addrlen,
		    &prefix, &content, &suffix);
		if (rc != EOK)
			return rc;
		
		IPC_SET_ADDR(*answer, addrlen);
		IPC_SET_PREFIX(*answer, prefix);
		IPC_SET_CONTENT(*answer, content);
		IPC_SET_SUFFIX(*answer, suffix);
		*answer_count = 4;
		
		return EOK;
	case NET_NIL_ADDR:
		rc = eth_addr_message(IPC_GET_DEVICE(*call), ETH_LOCAL_ADDR);
		if (rc != EOK)
			return rc;
		
		IPC_SET_ADDR(*answer, ETH_ADDR);
		*answer_count = 1;
		
		return EOK;
	case NET_NIL_BROADCAST_ADDR:
		rc = eth_addr_message(IPC_GET_DEVICE(*call), ETH_BROADCAST_ADDR);
		if (rc != EOK)
			return rc;
		
		IPC_SET_ADDR(*answer, ETH_ADDR);
		*answer_count = 1;
		
		return EOK;
	case NET_NIL_DEVICE_STATE:
		nil_device_state_msg_local(IPC_GET_DEVICE(*call), IPC_GET_STATE(*call));
		async_answer_0(callid, EOK);
		return EOK;
	case NET_NIL_RECEIVED:
		rc = packet_translate_remote(eth_globals.net_sess, &packet,
		    IPC_GET_ARG2(*call));
		if (rc == EOK)
			rc = nil_received_msg_local(IPC_GET_ARG1(*call), packet);
		
		async_answer_0(callid, (sysarg_t) rc);
		return rc;
	case NET_NIL_ADDR_CHANGED:
		rc = eth_addr_changed(IPC_GET_DEVICE(*call));
		async_answer_0(callid, (sysarg_t) rc);
		return rc;
	}
	
	return ENOTSUP;
}

int main(int argc, char *argv[])
{
	/* Start the module */
	return nil_module_start(SERVICE_ETHERNET);
}

/** @}
 */
