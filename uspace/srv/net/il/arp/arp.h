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
 *  @{
 */

/** @file
 *  ARP module.
 */

#ifndef __NET_ARP_H__
#define __NET_ARP_H__

#include <fibril_synch.h>

#include <ipc/ipc.h>
#include <ipc/services.h>

#include "../../include/device.h"
#include "../../include/hardware.h"

#include "../../structures/generic_char_map.h"
#include "../../structures/int_map.h"
#include "../../structures/measured_strings.h"


/** Type definition of the ARP global data.
 *  @see arp_globals
 */
typedef struct arp_globals	arp_globals_t;

/** Type definition of the ARP device specific data.
 *  @see arp_device
 */
typedef struct arp_device	arp_device_t;

/** Type definition of the ARP device specific data pointer.
 *  @see arp_device
 */
typedef arp_device_t *		arp_device_ref;

/** Type definition of the ARP protocol specific data.
 *  @see arp_proto
 */
typedef struct arp_proto	arp_proto_t;

/** Type definition of the ARP protocol specific data pointer.
 *  @see arp_proto
 */
typedef arp_proto_t *		arp_proto_ref;

/** ARP address cache.
 *  Maps devices to the ARP device specific data.
 *  @see device.h
 */
DEVICE_MAP_DECLARE( arp_cache, arp_device_t )

/** ARP protocol map.
 *  Maps protocol identifiers to the ARP protocol specific data.
 *  @see int_map.h
 */
INT_MAP_DECLARE( arp_protos, arp_proto_t )

/** ARP address map.
 *  Translates addresses.
 *  @see generic_char_map.h
 */
GENERIC_CHAR_MAP_DECLARE( arp_addr, measured_string_t )

/** ARP device specific data.
 */
struct arp_device{
	/** Device identifier.
	 */
	device_id_t			device_id;
	/** Hardware type.
	 */
	hw_type_t			hardware;
	/** Packet dimension.
	 */
	packet_dimension_t	packet_dimension;
	/** Actual device hardware address.
	 */
	measured_string_ref	addr;
	/** Actual device hardware address data.
	 */
	char *				addr_data;
	/** Broadcast device hardware address.
	 */
	measured_string_ref	broadcast_addr;
	/** Broadcast device hardware address data.
	 */
	char *				broadcast_data;
	/** Device module service.
	 */
	services_t			service;
	/** Device module phone.
	 */
	int					phone;
	/** Protocol map.
	 *  Address map for each protocol.
	 */
	arp_protos_t		protos;
};

/** ARP protocol specific data.
 */
struct arp_proto{
	/** Protocol service.
	 */
	services_t			service;
	/** Actual device protocol address.
	 */
	measured_string_ref	addr;
	/** Actual device protocol address data.
	 */
	char *				addr_data;
	/** Address map.
	 */
	arp_addr_t			addresses;
};

/** ARP global data.
 */
struct	arp_globals{
	/** Networking module phone.
	 */
	int			net_phone;
	/** Safety lock.
	 */
	fibril_rwlock_t		lock;
	/** ARP address cache.
	 */
	arp_cache_t	cache;
	/** The client connection processing function.
	 *  The module skeleton propagates its own one.
	 */
	async_client_conn_t client_connection;
};

#endif

/** @}
 */
