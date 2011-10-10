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
 * ARP module.
 */

#ifndef NET_ARP_H_
#define NET_ARP_H_

#include <async.h>
#include <fibril_synch.h>
#include <ipc/services.h>
#include <net/device.h>
#include <net/packet.h>
#include <net_hardware.h>
#include <adt/generic_char_map.h>
#include <adt/int_map.h>
#include <adt/measured_strings.h>

/** Type definition of the ARP device specific data.
 * @see arp_device
 */
typedef struct arp_device arp_device_t;

/** Type definition of the ARP global data.
 * @see arp_globals
 */
typedef struct arp_globals arp_globals_t;

/** Type definition of the ARP protocol specific data.
 * @see arp_proto
 */
typedef struct arp_proto arp_proto_t;

/** Type definition of the ARP address translation record.
 * @see arp_trans
 */
typedef struct arp_trans arp_trans_t;

/** ARP address map.
 *
 * Translates addresses.
 * @see generic_char_map.h
 */
GENERIC_CHAR_MAP_DECLARE(arp_addr, arp_trans_t);

/** ARP address cache.
 *
 * Maps devices to the ARP device specific data.
 * @see device.h
 */
DEVICE_MAP_DECLARE(arp_cache, arp_device_t);

/** ARP protocol map.
 *
 * Maps protocol identifiers to the ARP protocol specific data.
 * @see int_map.h
 */
INT_MAP_DECLARE(arp_protos, arp_proto_t);

/** ARP device specific data. */
struct arp_device {
	/** Actual device hardware address. */
	uint8_t addr[NIC_MAX_ADDRESS_LENGTH];
	/** Actual device hardware address length. */
	size_t addr_len;
	/** Broadcast device hardware address. */
	uint8_t broadcast_addr[NIC_MAX_ADDRESS_LENGTH];
	/** Broadcast device hardware address length. */
	size_t broadcast_addr_len;
	/** Device identifier. */
	nic_device_id_t device_id;
	/** Hardware type. */
	hw_type_t hardware;
	/** Packet dimension. */
	packet_dimension_t packet_dimension;
	/** Device module session. */
	async_sess_t *sess;
	
	/**
	 * Protocol map.
	 * Address map for each protocol.
	 */
	arp_protos_t protos;
	
	/** Device module service. */
	services_t service;
};

/** ARP global data. */
struct arp_globals {
	/** ARP address cache. */
	arp_cache_t cache;
	
	/** Networking module session. */
	async_sess_t *net_sess;
	
	/** Safety lock. */
	fibril_mutex_t lock;
};

/** ARP protocol specific data. */
struct arp_proto {
	/** Actual device protocol address. */
	measured_string_t *addr;
	/** Actual device protocol address data. */
	uint8_t *addr_data;
	/** Address map. */
	arp_addr_t addresses;
	/** Protocol service. */
	services_t service;
};

/** ARP address translation record. */
struct arp_trans {
	/**
	 * Hardware address for the translation. NULL denotes an incomplete
	 * record with possible waiters.
	 */
	measured_string_t *hw_addr;
	/** Condition variable used for waiting for completion of the record. */
	fibril_condvar_t cv;
};

#endif

/** @}
 */

