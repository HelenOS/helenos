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
 *  Ethernet module.
 */

#ifndef __NET_ETH_H__
#define __NET_ETH_H__

#include <fibril_synch.h>
#include <ipc/services.h>

#include <net_device.h>
#include <adt/measured_strings.h>

/** Type definition of the Ethernet global data.
 *  @see eth_globals
 */
typedef struct eth_globals	eth_globals_t;

/** Type definition of the Ethernet device specific data.
 *  @see eth_device
 */
typedef struct eth_device	eth_device_t;

/** Type definition of the Ethernet device specific data pointer.
 *  @see eth_device
 */
typedef eth_device_t *		eth_device_ref;

/** Type definition of the Ethernet protocol specific data.
 *  @see eth_proto
 */
typedef struct eth_proto	eth_proto_t;

/** Type definition of the Ethernet protocol specific data pointer.
 *  @see eth_proto
 */
typedef eth_proto_t *		eth_proto_ref;

/** Ethernet device map.
 *  Maps devices to the Ethernet device specific data.
 *  @see device.h
 */
DEVICE_MAP_DECLARE(eth_devices, eth_device_t)

/** Ethernet protocol map.
 *  Maps protocol identifiers to the Ethernet protocol specific data.
 *  @see int_map.h
 */
INT_MAP_DECLARE(eth_protos, eth_proto_t)

/** Ethernet device specific data.
 */
struct	eth_device{
	/** Device identifier.
	 */
	device_id_t device_id;
	/** Device driver service.
	 */
	services_t service;
	/** Driver phone.
	 */
	int phone;
	/** Maximal transmission unit.
	 */
	size_t mtu;
	/** Various device flags.
	 *  @see ETH_DUMMY
	 *  @see ETH_MODE_MASK
	 */
	int flags;
	/** Actual device hardware address.
	 */
	measured_string_ref addr;
	/** Actual device hardware address data.
	 */
	char * addr_data;
};

/** Ethernet protocol specific data.
 */
struct eth_proto{
	/** Protocol service.
	 */
	services_t service;
	/** Protocol identifier.
	 */
	int protocol;
	/** Protocol module phone.
	 */
	int phone;
};

/** Ethernet global data.
 */
struct	eth_globals{
	/** Networking module phone.
	 */
	int net_phone;
	/** Safety lock for devices.
	 */
	fibril_rwlock_t devices_lock;
	/** All known Ethernet devices.
	 */
	eth_devices_t devices;
	/** Safety lock for protocols.
	 */
	fibril_rwlock_t protos_lock;
	/** Protocol map.
	 *  Service phone map for each protocol.
	 */
	eth_protos_t protos;
	/** Broadcast device hardware address.
	 */
	measured_string_ref broadcast_addr;
};

/** Module initialization.
 *  Is called by the module_start() function.
 *  @param[in] net_phone The networking moduel phone.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for each specific module initialize function.
 */
extern int nil_initialize(int net_phone);

/** Message processing function.
 *  @param[in] callid The message identifier.
 *  @param[in] call The message parameters.
 *  @param[out] answer The message answer parameters.
 *  @param[out] answer_count The last parameter for the actual answer in the answer parameter.
 *  @returns EOK on success.
 *  @returns ENOTSUP if the message is not known.
 *  @returns Other error codes as defined for each specific module message function.
 *  @see nil_interface.h
 *  @see IS_NET_NIL_MESSAGE()
 */
extern int nil_message(ipc_callid_t callid, ipc_call_t * call, ipc_call_t * answer, int * answer_count);

#endif

/** @}
 */
