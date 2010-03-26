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

/** @addtogroup eth
 *  @{
 */

/** @file
 *  Ethernet module implementation.
 *  @see eth.h
 */

#include <async.h>
#include <malloc.h>
#include <mem.h>
#include <stdio.h>
#include <str.h>

#include <ipc/ipc.h>
#include <ipc/services.h>

#include "../../err.h"
#include "../../messages.h"
#include "../../modules.h"

#include "../../include/byteorder.h"
#include "../../include/checksum.h"
#include "../../include/ethernet_lsap.h"
#include "../../include/ethernet_protocols.h"
#include "../../include/protocol_map.h"
#include "../../include/device.h"
#include "../../include/netif_interface.h"
#include "../../include/net_interface.h"
#include "../../include/nil_interface.h"
#include "../../include/il_interface.h"

#include "../../structures/measured_strings.h"
#include "../../structures/packet/packet_client.h"

#include "../nil_module.h"

#include "eth.h"
#include "eth_header.h"

/** Reserved packet prefix length.
 */
#define ETH_PREFIX		(sizeof(eth_header_t) + sizeof(eth_header_lsap_t) + sizeof(eth_header_snap_t))

/** Reserved packet suffix length.
 */
#define ETH_SUFFIX		sizeof(eth_fcs_t)

/** Maximum packet content length.
 */
#define ETH_MAX_CONTENT 1500u

/** Minimum packet content length.
 */
#define ETH_MIN_CONTENT 46u

/** Maximum tagged packet content length.
 */
#define ETH_MAX_TAGGED_CONTENT(flags)	(ETH_MAX_CONTENT - ((IS_8023_2_LSAP(flags) || IS_8023_2_SNAP(flags)) ? sizeof(eth_header_lsap_t) : 0) - (IS_8023_2_SNAP(flags) ? sizeof(eth_header_snap_t) : 0))

/** Minimum tagged packet content length.
 */
#define ETH_MIN_TAGGED_CONTENT(flags)	(ETH_MIN_CONTENT - ((IS_8023_2_LSAP(flags) || IS_8023_2_SNAP(flags)) ? sizeof(eth_header_lsap_t) : 0) - (IS_8023_2_SNAP(flags) ? sizeof(eth_header_snap_t) : 0))

/** Dummy flag shift value.
 */
#define ETH_DUMMY_SHIFT	0

/** Mode flag shift value.
 */
#define ETH_MODE_SHIFT	1

/** Dummy device flag.
 *  Preamble and FCS are mandatory part of the packets.
 */
#define ETH_DUMMY				(1 << ETH_DUMMY_SHIFT)

/** Returns the dummy flag.
 *  @see ETH_DUMMY
 */
#define IS_DUMMY(flags)		((flags) &ETH_DUMMY)

/** Device mode flags.
 *  @see ETH_DIX
 *  @see ETH_8023_2_LSAP
 *  @see ETH_8023_2_SNAP
 */
#define ETH_MODE_MASK			(3 << ETH_MODE_SHIFT)

/** DIX Ethernet mode flag.
 */
#define ETH_DIX					(1 << ETH_MODE_SHIFT)

/** Returns whether the DIX Ethernet mode flag is set.
 *  @param[in] flags The ethernet flags.
 *  @see ETH_DIX
 */
#define IS_DIX(flags)			(((flags) &ETH_MODE_MASK) == ETH_DIX)

/** 802.3 + 802.2 + LSAP mode flag.
 */
#define ETH_8023_2_LSAP			(2 << ETH_MODE_SHIFT)

/** Returns whether the 802.3 + 802.2 + LSAP mode flag is set.
 *  @param[in] flags The ethernet flags.
 *  @see ETH_8023_2_LSAP
 */
#define IS_8023_2_LSAP(flags)	(((flags) &ETH_MODE_MASK) == ETH_8023_2_LSAP)

/** 802.3 + 802.2 + LSAP + SNAP mode flag.
 */
#define ETH_8023_2_SNAP			(3 << ETH_MODE_SHIFT)

/** Returns whether the 802.3 + 802.2 + LSAP + SNAP mode flag is set.
 *  @param[in] flags The ethernet flags.
 *  @see ETH_8023_2_SNAP
 */
#define IS_8023_2_SNAP(flags)	(((flags) &ETH_MODE_MASK) == ETH_8023_2_SNAP)

/** Type definition of the ethernet address type.
 *  @see eth_addr_type
 */
typedef enum eth_addr_type	eth_addr_type_t;

/** Type definition of the ethernet address type pointer.
 *  @see eth_addr_type
 */
typedef eth_addr_type_t *	eth_addr_type_ref;

/** Ethernet address type.
 */
enum eth_addr_type{
	/** Local address.
	 */
	ETH_LOCAL_ADDR,
	/** Broadcast address.
	 */
	ETH_BROADCAST_ADDR
};

/** Ethernet module global data.
 */
eth_globals_t	eth_globals;

/** @name Message processing functions
 */
/*@{*/

/** Processes IPC messages from the registered device driver modules in an infinite loop.
 *  @param[in] iid The message identifier.
 *  @param[in,out] icall The message parameters.
 */
void eth_receiver(ipc_callid_t iid, ipc_call_t * icall);

/** Registers new device or updates the MTU of an existing one.
 *  Determines the device local hardware address.
 *  @param[in] device_id The new device identifier.
 *  @param[in] service The device driver service.
 *  @param[in] mtu The device maximum transmission unit.
 *  @returns EOK on success.
 *  @returns EEXIST if the device with the different service exists.
 *  @returns ENOMEM if there is not enough memory left.
 *  @returns Other error codes as defined for the net_get_device_conf_req() function.
 *  @returns Other error codes as defined for the netif_bind_service() function.
 *  @returns Other error codes as defined for the netif_get_addr_req() function.
 */
int eth_device_message(device_id_t device_id, services_t service, size_t mtu);

/** Registers receiving module service.
 *  Passes received packets for this service.
 *  @param[in] service The module service.
 *  @param[in] phone The service phone.
 *  @returns EOK on success.
 *  @returns ENOENT if the service is not known.
 *  @returns ENOMEM if there is not enough memory left.
 */
int eth_register_message(services_t service, int phone);

/** Returns the device packet dimensions for sending.
 *  @param[in] device_id The device identifier.
 *  @param[out] addr_len The minimum reserved address length.
 *  @param[out] prefix The minimum reserved prefix size.
 *  @param[out] content The maximum content size.
 *  @param[out] suffix The minimum reserved suffix size.
 *  @returns EOK on success.
 *  @returns EBADMEM if either one of the parameters is NULL.
 *  @returns ENOENT if there is no such device.
 */
int eth_packet_space_message(device_id_t device_id, size_t * addr_len, size_t * prefix, size_t * content, size_t * suffix);

/** Returns the device hardware address.
 *  @param[in] device_id The device identifier.
 *  @param[in] type Type of the desired address.
 *  @param[out] address The device hardware address.
 *  @returns EOK on success.
 *  @returns EBADMEM if the address parameter is NULL.
 *  @returns ENOENT if there no such device.
 */
int eth_addr_message(device_id_t device_id, eth_addr_type_t type, measured_string_ref * address);

/** Sends the packet queue.
 *  Sends only packet successfully processed by the eth_prepare_packet() function.
 *  @param[in] device_id The device identifier.
 *  @param[in] packet The packet queue.
 *  @param[in] sender The sending module service.
 *  @returns EOK on success.
 *  @returns ENOENT if there no such device.
 *  @returns EINVAL if the service parameter is not known.
 */
int eth_send_message(device_id_t device_id, packet_t packet, services_t sender);

/*@}*/

/** Processes the received packet and chooses the target registered module.
 *  @param[in] flags The device flags.
 *  @param[in] packet The packet.
 *  @returns The target registered module.
 *  @returns NULL if the packet is not long enough.
 *  @returns NULL if the packet is too long.
 *  @returns NULL if the raw ethernet protocol is used.
 *  @returns NULL if the dummy device FCS checksum is invalid.
 *  @returns NULL if the packet address length is not big enough.
 */
eth_proto_ref eth_process_packet(int flags, packet_t packet);

/** Prepares the packet for sending.
 *  @param[in] flags The device flags.
 *  @param[in] packet The packet.
 *  @param[in] src_addr The source hardware address.
 *  @param[in] ethertype The ethernet protocol type.
 *  @param[in] mtu The device maximum transmission unit.
 *  @returns EOK on success.
 *  @returns EINVAL if the packet addresses length is not long enough.
 *  @returns EINVAL if the packet is bigger than the device MTU.
 *  @returns ENOMEM if there is not enough memory in the packet.
 */
int eth_prepare_packet(int flags, packet_t packet, uint8_t * src_addr, int ethertype, size_t mtu);

DEVICE_MAP_IMPLEMENT(eth_devices, eth_device_t)

INT_MAP_IMPLEMENT(eth_protos, eth_proto_t)

int nil_device_state_msg(int nil_phone, device_id_t device_id, int state){
	int index;
	eth_proto_ref proto;

	fibril_rwlock_read_lock(&eth_globals.protos_lock);
	for(index = eth_protos_count(&eth_globals.protos) - 1; index >= 0; -- index){
		proto = eth_protos_get_index(&eth_globals.protos, index);
		if(proto && proto->phone){
			il_device_state_msg(proto->phone, device_id, state, proto->service);
		}
	}
	fibril_rwlock_read_unlock(&eth_globals.protos_lock);
	return EOK;
}

int nil_initialize(int net_phone){
	ERROR_DECLARE;

	fibril_rwlock_initialize(&eth_globals.devices_lock);
	fibril_rwlock_initialize(&eth_globals.protos_lock);
	fibril_rwlock_write_lock(&eth_globals.devices_lock);
	fibril_rwlock_write_lock(&eth_globals.protos_lock);
	eth_globals.net_phone = net_phone;
	eth_globals.broadcast_addr = measured_string_create_bulk("\xFF\xFF\xFF\xFF\xFF\xFF", CONVERT_SIZE(uint8_t, char, ETH_ADDR));
	if(! eth_globals.broadcast_addr){
		return ENOMEM;
	}
	ERROR_PROPAGATE(eth_devices_initialize(&eth_globals.devices));
	if(ERROR_OCCURRED(eth_protos_initialize(&eth_globals.protos))){
		eth_devices_destroy(&eth_globals.devices);
		return ERROR_CODE;
	}
	fibril_rwlock_write_unlock(&eth_globals.protos_lock);
	fibril_rwlock_write_unlock(&eth_globals.devices_lock);
	return EOK;
}

int eth_device_message(device_id_t device_id, services_t service, size_t mtu){
	ERROR_DECLARE;

	eth_device_ref device;
	int index;
	measured_string_t names[2] = {{str_dup("ETH_MODE"), 8}, {str_dup("ETH_DUMMY"), 9}};
	measured_string_ref configuration;
	size_t count = sizeof(names) / sizeof(measured_string_t);
	char * data;
	eth_proto_ref proto;

	fibril_rwlock_write_lock(&eth_globals.devices_lock);
	// an existing device?
	device = eth_devices_find(&eth_globals.devices, device_id);
	if(device){
		if(device->service != service){
			printf("Device %d already exists\n", device->device_id);
			fibril_rwlock_write_unlock(&eth_globals.devices_lock);
			return EEXIST;
		}else{
			// update mtu
			if((mtu > 0) && (mtu <= ETH_MAX_TAGGED_CONTENT(device->flags))){
				device->mtu = mtu;
			}else{
				 device->mtu = ETH_MAX_TAGGED_CONTENT(device->flags);
			}
			printf("Device %d already exists:\tMTU\t= %d\n", device->device_id, device->mtu);
			fibril_rwlock_write_unlock(&eth_globals.devices_lock);
			// notify all upper layer modules
			fibril_rwlock_read_lock(&eth_globals.protos_lock);
			for(index = 0; index < eth_protos_count(&eth_globals.protos); ++ index){
				proto = eth_protos_get_index(&eth_globals.protos, index);
				if (proto->phone){
					il_mtu_changed_msg(proto->phone, device->device_id, device->mtu, proto->service);
				}
			}
			fibril_rwlock_read_unlock(&eth_globals.protos_lock);
			return EOK;
		}
	}else{
		// create a new device
		device = (eth_device_ref) malloc(sizeof(eth_device_t));
		if(! device){
			return ENOMEM;
		}
		device->device_id = device_id;
		device->service = service;
		device->flags = 0;
		if((mtu > 0) && (mtu <= ETH_MAX_TAGGED_CONTENT(device->flags))){
			device->mtu = mtu;
		}else{
			 device->mtu = ETH_MAX_TAGGED_CONTENT(device->flags);
		}
		configuration = &names[0];
		if(ERROR_OCCURRED(net_get_device_conf_req(eth_globals.net_phone, device->device_id, &configuration, count, &data))){
			fibril_rwlock_write_unlock(&eth_globals.devices_lock);
			free(device);
			return ERROR_CODE;
		}
		if(configuration){
			if(! str_lcmp(configuration[0].value, "DIX", configuration[0].length)){
				device->flags |= ETH_DIX;
			}else if(! str_lcmp(configuration[0].value, "8023_2_LSAP", configuration[0].length)){
				device->flags |= ETH_8023_2_LSAP;
			}else device->flags |= ETH_8023_2_SNAP;
			if((configuration[1].value) && (configuration[1].value[0] == 'y')){
				device->flags |= ETH_DUMMY;
			}
			net_free_settings(configuration, data);
		}else{
			device->flags |= ETH_8023_2_SNAP;
		}
		// bind the device driver
		device->phone = netif_bind_service(device->service, device->device_id, SERVICE_ETHERNET, eth_receiver);
		if(device->phone < 0){
			fibril_rwlock_write_unlock(&eth_globals.devices_lock);
			free(device);
			return device->phone;
		}
		// get hardware address
		if(ERROR_OCCURRED(netif_get_addr_req(device->phone, device->device_id, &device->addr, &device->addr_data))){
			fibril_rwlock_write_unlock(&eth_globals.devices_lock);
			free(device);
			return ERROR_CODE;
		}
		// add to the cache
		index = eth_devices_add(&eth_globals.devices, device->device_id, device);
		if(index < 0){
			fibril_rwlock_write_unlock(&eth_globals.devices_lock);
			free(device->addr);
			free(device->addr_data);
			free(device);
			return index;
		}
		printf("New device registered:\n\tid\t= %d\n\tservice\t= %d\n\tMTU\t= %d\n\taddress\t= %X:%X:%X:%X:%X:%X\n\tflags\t= 0x%x\n", device->device_id, device->service, device->mtu, device->addr_data[0], device->addr_data[1], device->addr_data[2], device->addr_data[3], device->addr_data[4], device->addr_data[5], device->flags);
	}
	fibril_rwlock_write_unlock(&eth_globals.devices_lock);
	return EOK;
}

eth_proto_ref eth_process_packet(int flags, packet_t packet){
	ERROR_DECLARE;

	eth_header_snap_ref header;
	size_t length;
	eth_type_t type;
	size_t prefix;
	size_t suffix;
	eth_fcs_ref fcs;
	uint8_t * data;

	length = packet_get_data_length(packet);
	if(IS_DUMMY(flags)){
		packet_trim(packet, sizeof(eth_preamble_t), 0);
	}
	if(length < sizeof(eth_header_t) + ETH_MIN_CONTENT + (IS_DUMMY(flags) ? ETH_SUFFIX : 0)) return NULL;
	data = packet_get_data(packet);
	header = (eth_header_snap_ref) data;
	type = ntohs(header->header.ethertype);
	if(type >= ETH_MIN_PROTO){
		// DIX Ethernet
		prefix = sizeof(eth_header_t);
		suffix = 0;
		fcs = (eth_fcs_ref) data + length - sizeof(eth_fcs_t);
		length -= sizeof(eth_fcs_t);
	}else if(type <= ETH_MAX_CONTENT){
		// translate "LSAP" values
		if((header->lsap.dsap == ETH_LSAP_GLSAP) && (header->lsap.ssap == ETH_LSAP_GLSAP)){
			// raw packet
			// discard
			return NULL;
		}else if((header->lsap.dsap == ETH_LSAP_SNAP) && (header->lsap.ssap == ETH_LSAP_SNAP)){
			// IEEE 802.3 + 802.2 + LSAP + SNAP
			// organization code not supported
			type = ntohs(header->snap.ethertype);
			prefix = sizeof(eth_header_t) + sizeof(eth_header_lsap_t) + sizeof(eth_header_snap_t);
		}else{
			// IEEE 802.3 + 802.2 LSAP
			type = lsap_map(header->lsap.dsap);
			prefix = sizeof(eth_header_t) + sizeof(eth_header_lsap_t);
		}
		suffix = (type < ETH_MIN_CONTENT) ? ETH_MIN_CONTENT - type : 0u;
		fcs = (eth_fcs_ref) data + prefix + type + suffix;
		suffix += length - prefix - type;
		length = prefix + type + suffix;
	}else{
		// invalid length/type, should not occurr
		return NULL;
	}
	if(IS_DUMMY(flags)){
		if((~ compute_crc32(~ 0u, data, length * 8)) != ntohl(*fcs)){
			return NULL;
		}
		suffix += sizeof(eth_fcs_t);
	}
	if(ERROR_OCCURRED(packet_set_addr(packet, header->header.source_address, header->header.destination_address, ETH_ADDR))
		|| ERROR_OCCURRED(packet_trim(packet, prefix, suffix))){
		return NULL;
	}
	return eth_protos_find(&eth_globals.protos, type);
}

int nil_received_msg(int nil_phone, device_id_t device_id, packet_t packet, services_t target){
	eth_proto_ref proto;
	packet_t next;
	eth_device_ref device;
	int flags;

	fibril_rwlock_read_lock(&eth_globals.devices_lock);
	device = eth_devices_find(&eth_globals.devices, device_id);
	if(! device){
		fibril_rwlock_read_unlock(&eth_globals.devices_lock);
		return ENOENT;
	}
	flags = device->flags;
	fibril_rwlock_read_unlock(&eth_globals.devices_lock);
	fibril_rwlock_read_lock(&eth_globals.protos_lock);
	do{
		next = pq_detach(packet);
		proto = eth_process_packet(flags, packet);
		if(proto){
			il_received_msg(proto->phone, device_id, packet, proto->service);
		}else{
			// drop invalid/unknown
			pq_release(eth_globals.net_phone, packet_get_id(packet));
		}
		packet = next;
	}while(packet);
	fibril_rwlock_read_unlock(&eth_globals.protos_lock);
	return EOK;
}

int eth_packet_space_message(device_id_t device_id, size_t * addr_len, size_t * prefix, size_t * content, size_t * suffix){
	eth_device_ref device;

	if(!(addr_len && prefix && content && suffix)){
		return EBADMEM;
	}
	fibril_rwlock_read_lock(&eth_globals.devices_lock);
	device = eth_devices_find(&eth_globals.devices, device_id);
	if(! device){
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

int eth_addr_message(device_id_t device_id, eth_addr_type_t type, measured_string_ref * address){
	eth_device_ref device;

	if(! address){
		return EBADMEM;
	}
	if(type == ETH_BROADCAST_ADDR){
		*address = eth_globals.broadcast_addr;
	}else{
		fibril_rwlock_read_lock(&eth_globals.devices_lock);
		device = eth_devices_find(&eth_globals.devices, device_id);
		if(! device){
			fibril_rwlock_read_unlock(&eth_globals.devices_lock);
			return ENOENT;
		}
		*address = device->addr;
		fibril_rwlock_read_unlock(&eth_globals.devices_lock);
	}
	return (*address) ? EOK : ENOENT;
}

int eth_register_message(services_t service, int phone){
	eth_proto_ref proto;
	int protocol;
	int index;

	protocol = protocol_map(SERVICE_ETHERNET, service);
	if(! protocol){
		return ENOENT;
	}
	fibril_rwlock_write_lock(&eth_globals.protos_lock);
	proto = eth_protos_find(&eth_globals.protos, protocol);
	if(proto){
		proto->phone = phone;
		fibril_rwlock_write_unlock(&eth_globals.protos_lock);
		return EOK;
	}else{
		proto = (eth_proto_ref) malloc(sizeof(eth_proto_t));
		if(! proto){
			fibril_rwlock_write_unlock(&eth_globals.protos_lock);
			return ENOMEM;
		}
		proto->service = service;
		proto->protocol = protocol;
		proto->phone = phone;
		index = eth_protos_add(&eth_globals.protos, protocol, proto);
		if(index < 0){
			fibril_rwlock_write_unlock(&eth_globals.protos_lock);
			free(proto);
			return index;
		}
	}
	printf("New protocol registered:\n\tprotocol\t= 0x%x\n\tservice\t= %d\n\tphone\t= %d\n", proto->protocol, proto->service, proto->phone);
	fibril_rwlock_write_unlock(&eth_globals.protos_lock);
	return EOK;
}

int eth_prepare_packet(int flags, packet_t packet, uint8_t * src_addr, int ethertype, size_t mtu){
	eth_header_snap_ref header;
	eth_header_lsap_ref header_lsap;
	eth_header_ref header_dix;
	eth_fcs_ref fcs;
	uint8_t * src;
	uint8_t * dest;
	size_t length;
	int i;
	void * padding;
	eth_preamble_ref preamble;

	i = packet_get_addr(packet, &src, &dest);
	if(i < 0){
		return i;
	}
	if(i != ETH_ADDR){
		return EINVAL;
	}
	length = packet_get_data_length(packet);
	if(length > mtu){
		return EINVAL;
	}
	if(length < ETH_MIN_TAGGED_CONTENT(flags)){
		padding = packet_suffix(packet, ETH_MIN_TAGGED_CONTENT(flags) - length);
		if(! padding){
			return ENOMEM;
		}
		bzero(padding, ETH_MIN_TAGGED_CONTENT(flags) - length);
	}
	if(IS_DIX(flags)){
		header_dix = PACKET_PREFIX(packet, eth_header_t);
		if(! header_dix){
			return ENOMEM;
		}
		header_dix->ethertype = (uint16_t) ethertype;
		memcpy(header_dix->source_address, src_addr, ETH_ADDR);
		memcpy(header_dix->destination_address, dest, ETH_ADDR);
		src = &header_dix->destination_address[0];
	}else if(IS_8023_2_LSAP(flags)){
		header_lsap = PACKET_PREFIX(packet, eth_header_lsap_t);
		if(! header_lsap){
			return ENOMEM;
		}
		header_lsap->header.ethertype = htons(length + sizeof(eth_header_lsap_t));
		header_lsap->lsap.dsap = lsap_unmap(ntohs(ethertype));
		header_lsap->lsap.ssap = header_lsap->lsap.dsap;
		header_lsap->lsap.ctrl = IEEE_8023_2_UI;
		memcpy(header_lsap->header.source_address, src_addr, ETH_ADDR);
		memcpy(header_lsap->header.destination_address, dest, ETH_ADDR);
		src = &header_lsap->header.destination_address[0];
	}else if(IS_8023_2_SNAP(flags)){
		header = PACKET_PREFIX(packet, eth_header_snap_t);
		if(! header){
			return ENOMEM;
		}
		header->header.ethertype = htons(length + sizeof(eth_header_lsap_t) + sizeof(eth_header_snap_t));
		header->lsap.dsap = (uint16_t) ETH_LSAP_SNAP;
		header->lsap.ssap = header->lsap.dsap;
		header->lsap.ctrl = IEEE_8023_2_UI;
		for(i = 0; i < 3; ++ i){
			header->snap.protocol[i] = 0;
		}
		header->snap.ethertype = (uint16_t) ethertype;
		memcpy(header->header.source_address, src_addr, ETH_ADDR);
		memcpy(header->header.destination_address, dest, ETH_ADDR);
		src = &header->header.destination_address[0];
	}
	if(IS_DUMMY(flags)){
		preamble = PACKET_PREFIX(packet, eth_preamble_t);
		if(! preamble){
			return ENOMEM;
		}
		for(i = 0; i < 7; ++ i){
			preamble->preamble[i] = ETH_PREAMBLE;
		}
		preamble->sfd = ETH_SFD;
		fcs = PACKET_SUFFIX(packet, eth_fcs_t);
		if(! fcs){
			return ENOMEM;
		}
		*fcs = htonl(~ compute_crc32(~ 0u, src, length * 8));
	}
	return EOK;
}

int eth_send_message(device_id_t device_id, packet_t packet, services_t sender){
	ERROR_DECLARE;

	eth_device_ref device;
	packet_t next;
	packet_t tmp;
	int ethertype;

	ethertype = htons(protocol_map(SERVICE_ETHERNET, sender));
	if(! ethertype){
		pq_release(eth_globals.net_phone, packet_get_id(packet));
		return EINVAL;
	}
	fibril_rwlock_read_lock(&eth_globals.devices_lock);
	device = eth_devices_find(&eth_globals.devices, device_id);
	if(! device){
		fibril_rwlock_read_unlock(&eth_globals.devices_lock);
		return ENOENT;
	}
	// process packet queue
	next = packet;
	do{
		if(ERROR_OCCURRED(eth_prepare_packet(device->flags, next, (uint8_t *) device->addr->value, ethertype, device->mtu))){
			// release invalid packet
			tmp = pq_detach(next);
			if(next == packet){
				packet = tmp;
			}
			pq_release(eth_globals.net_phone, packet_get_id(next));
			next = tmp;
		}else{
			next = pq_next(next);
		}
	}while(next);
	// send packet queue
	if(packet){
		netif_send_msg(device->phone, device_id, packet, SERVICE_ETHERNET);
	}
	fibril_rwlock_read_unlock(&eth_globals.devices_lock);
	return EOK;
}

int nil_message(ipc_callid_t callid, ipc_call_t * call, ipc_call_t * answer, int * answer_count){
	ERROR_DECLARE;

	measured_string_ref address;
	packet_t packet;
	size_t addrlen;
	size_t prefix;
	size_t suffix;
	size_t content;

//	printf("message %d - %d\n", IPC_GET_METHOD(*call), NET_NIL_FIRST);
	*answer_count = 0;
	switch(IPC_GET_METHOD(*call)){
		case IPC_M_PHONE_HUNGUP:
			return EOK;
		case NET_NIL_DEVICE:
			return eth_device_message(IPC_GET_DEVICE(call), IPC_GET_SERVICE(call), IPC_GET_MTU(call));
		case NET_NIL_SEND:
			ERROR_PROPAGATE(packet_translate(eth_globals.net_phone, &packet, IPC_GET_PACKET(call)));
			return eth_send_message(IPC_GET_DEVICE(call), packet, IPC_GET_SERVICE(call));
		case NET_NIL_PACKET_SPACE:
			ERROR_PROPAGATE(eth_packet_space_message(IPC_GET_DEVICE(call), &addrlen, &prefix, &content, &suffix));
			IPC_SET_ADDR(answer, addrlen);
			IPC_SET_PREFIX(answer, prefix);
			IPC_SET_CONTENT(answer, content);
			IPC_SET_SUFFIX(answer, suffix);
			*answer_count = 4;
			return EOK;
		case NET_NIL_ADDR:
			ERROR_PROPAGATE(eth_addr_message(IPC_GET_DEVICE(call), ETH_LOCAL_ADDR, &address));
			return measured_strings_reply(address, 1);
		case NET_NIL_BROADCAST_ADDR:
			ERROR_PROPAGATE(eth_addr_message(IPC_GET_DEVICE(call), ETH_BROADCAST_ADDR, &address));
			return measured_strings_reply(address, 1);
		case IPC_M_CONNECT_TO_ME:
			return eth_register_message(NIL_GET_PROTO(call), IPC_GET_PHONE(call));
	}
	return ENOTSUP;
}

void eth_receiver(ipc_callid_t iid, ipc_call_t * icall){
	ERROR_DECLARE;

	packet_t packet;

	while(true){
//		printf("message %d - %d\n", IPC_GET_METHOD(*icall), NET_NIL_FIRST);
		switch(IPC_GET_METHOD(*icall)){
			case NET_NIL_DEVICE_STATE:
				nil_device_state_msg(0, IPC_GET_DEVICE(icall), IPC_GET_STATE(icall));
				ipc_answer_0(iid, EOK);
				break;
			case NET_NIL_RECEIVED:
				if(! ERROR_OCCURRED(packet_translate(eth_globals.net_phone, &packet, IPC_GET_PACKET(icall)))){
					ERROR_CODE = nil_received_msg(0, IPC_GET_DEVICE(icall), packet, 0);
				}
				ipc_answer_0(iid, (ipcarg_t) ERROR_CODE);
				break;
			default:
				ipc_answer_0(iid, (ipcarg_t) ENOTSUP);
		}
		iid = async_get_call(icall);
	}
}

/** @}
 */
