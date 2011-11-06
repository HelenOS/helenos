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

/** @addtogroup ip
 * @{
 */

/** @file
 * IP module.
 */

#ifndef NET_IP_H_
#define NET_IP_H_

#include <async.h>
#include <fibril_synch.h>
#include <ipc/services.h>
#include <net/device.h>
#include <net/inet.h>
#include <ip_interface.h>
#include <adt/int_map.h>
#include <adt/generic_field.h>
#include <adt/module_map.h>

/** Type definition of the IP global data.
 * @see ip_globals
 */
typedef struct ip_globals ip_globals_t;

/** Type definition of the IP network interface specific data.
 * @see ip_netif
 */
typedef struct ip_netif ip_netif_t;

/** Type definition of the IP protocol specific data.
 * @see ip_proto
 */
typedef struct ip_proto ip_proto_t;

/** Type definition of the IP route specific data.
 *  @see ip_route
 */
typedef struct ip_route	ip_route_t;

/** IP network interfaces.
 * Maps devices to the IP network interface specific data.
 * @see device.h
 */
DEVICE_MAP_DECLARE(ip_netifs, ip_netif_t);

/** IP registered protocols.
 * Maps protocols to the IP protocol specific data.
 * @see int_map.h
 */
INT_MAP_DECLARE(ip_protos, ip_proto_t);

/** IP routing table.
 * @see generic_field.h
 */
GENERIC_FIELD_DECLARE(ip_routes, ip_route_t);

/** IP network interface specific data. */
struct ip_netif {
	/** ARP module. Assigned if using ARP. */
	module_t *arp;
	/** Broadcast address. */
	in_addr_t broadcast;
	/** Device identifier. */
	nic_device_id_t device_id;
	/** Indicates whether using DHCP. */
	int dhcp;
	/** IP version. */
	int ipv;
	/** Packet dimension. */
	packet_dimension_t packet_dimension;
	/** Netif module session. */
	async_sess_t *sess;
	/** Routing table. */
	ip_routes_t routes;
	/** Indicates whether IP routing is enabled. */
	int routing;
	/** Netif module service. */
	services_t service;
	/** Device state. */
	nic_device_state_t state;
};

/** IP protocol specific data. */
struct ip_proto {
	/** Protocol module session. */
	async_sess_t *sess;
	/** Protocol number. */
	int protocol;
	/** Protocol packet receiving function. */
	tl_received_msg_t received_msg;
	/** Protocol module service. */
	services_t service;
};

/** IP route specific data. */
struct ip_route {
	/** Target address. */
	in_addr_t address;
	/** Gateway. */
	in_addr_t gateway;
	/** Parent netif. */
	ip_netif_t *netif;
	/** Target network mask. */
	in_addr_t netmask;
};

/** IP global data. */
struct ip_globals {
	/** Default gateway. */
	ip_route_t gateway;
	/** Safety lock. */
	fibril_rwlock_t lock;
	/** Known support modules. */
	modules_t modules;
	/** Networking module session. */
	async_sess_t *net_sess;
	/** Registered network interfaces. */
	ip_netifs_t netifs;
	/** Netifs safeyt lock. */
	fibril_rwlock_t netifs_lock;
	/** Packet counter. */
	uint16_t packet_counter;
	/** Registered protocols. */
	ip_protos_t protos;
	/** Protocols safety lock. */
	fibril_rwlock_t protos_lock;
};

#endif

/** @}
 */
