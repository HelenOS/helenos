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
 *  @{
 */

/** @file
 *  IP module interface.
 *  The same interface is used for standalone remote modules as well as for bundle modules.
 *  The standalone remote modules have to be compiled with the ip_remote.c source file.
 *  The bundle modules with the ip.c source file.
 */

#ifndef __NET_IP_INTERFACE_H__
#define __NET_IP_INTERFACE_H__

#include <async.h>

#include <ipc/services.h>

#include "../include/device.h"

#include "../structures/packet/packet.h"

#include "in.h"
#include "ip_codes.h"
#include "socket_codes.h"

/** @name IP module interface
 *  This interface is used by other modules.
 */
/*@{*/

/** Type definition of the internet pseudo header pointer.
 */
typedef void *		ip_pseudo_header_ref;

/** The transport layer notification function type definition.
 *  Notifies the transport layer modules about the received packet/s.
 *  @param[in] device_id The device identifier.
 *  @param[in] packet The received packet or the received packet queue.
 *  @param[in] receiver The receiving module service.
 *  @param[in] error The packet error reporting service. Prefixes the received packet.
 *  @returns EOK on success.
 */
typedef int	( * tl_received_msg_t )( device_id_t device_id, packet_t packet, services_t receiver, services_t error );

/** Creates bidirectional connection with the ip module service and registers the message receiver.
 *  @param[in] service The IP module service.
 *  @param[in] protocol The transport layer protocol.
 *  @param[in] me The requesting module service.
 *  @param[in] receiver The message receiver. Used for remote connection.
 *  @param[in] tl_received_msg The message processing function. Used if bundled together.
 *  @returns The phone of the needed service.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the bind_service() function.
 */
int	ip_bind_service( services_t service, int protocol, services_t me, async_client_conn_t receiver, tl_received_msg_t tl_received_msg );

/** Registers the new device.
 *  Registers itself as the ip packet receiver.
 *  If the device uses ARP registers also the new ARP device.
 *  @param[in] ip_phone The IP module phone used for (semi)remote calls.
 *  @param[in] device_id The new device identifier.
 *  @param[in] netif The underlying device network interface layer service.
 *  @returns EOK on success.
 *  @returns ENOMEM if there is not enough memory left.
 *  @returns EINVAL if the device configuration is invalid.
 *  @returns ENOTSUP if the device uses IPv6.
 *  @returns ENOTSUP if the device uses DHCP.
 *  @returns Other error codes as defined for the net_get_device_conf_req() function.
 *  @returns Other error codes as defined for the arp_device_req() function.
 */
int	ip_device_req( int ip_phone, device_id_t device_id, services_t netif );

/** Sends the packet queue.
 *  The packets may get fragmented if needed.
 *  @param[in] ip_phone The IP module phone used for (semi)remote calls.
 *  @param[in] device_id The device identifier.
 *  @param[in] packet The packet fragments as a~packet queue. All the packets have to have the same destination address.
 *  @param[in] sender The sending module service.
 *  @param[in] error The packet error reporting service. Prefixes the received packet.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the generic_send_msg() function.
 */
int	ip_send_msg( int ip_phone, device_id_t device_id, packet_t packet, services_t sender, services_t error );

/** Connects to the IP module.
 *  @param service The IP module service. Ignored parameter.
 *  @returns The IP module phone on success.
 *  @returns 0 if called by the bundle module.
 */
int	ip_connect_module( services_t service );

/** Adds a route to the device routing table.
 *  The target network is routed using this device.
 *  @param[in] ip_phone The IP module phone used for (semi)remote calls.
 *  @param[in] device_id The device identifier.
 *  @param[in] address The target network address.
 *  @param[in] netmask The target network mask.
 *  @param[in] gateway The target network gateway. Not used if zero.
 */
int	ip_add_route_req( int ip_phone, device_id_t device_id, in_addr_t address, in_addr_t netmask, in_addr_t gateway );

/** Sets the default gateway.
 *  This gateway is used if no other route is found.
 *  @param[in] ip_phone The IP module phone used for (semi)remote calls.
 *  @param[in] device_id The device identifier.
 *  @param[in] gateway The default gateway.
 */
int	ip_set_gateway_req( int ip_phone, device_id_t device_id, in_addr_t gateway );

/** Returns the device packet dimension for sending.
 *  @param[in] ip_phone The IP module phone used for (semi)remote calls.
 *  @param[in] device_id The device identifier.
 *  @param[out] packet_dimension The packet dimension.
 *  @returns EOK on success.
 *  @returns ENOENT if there is no such device.
 *  @returns Other error codes as defined for the generic_packet_size_req() function.
 */
int	ip_packet_size_req( int ip_phone, device_id_t device_id, packet_dimension_ref packet_dimension );

/** Notifies the IP module about the received error notification packet.
 *  @param[in] ip_phone The IP module phone used for (semi)remote calls.
 *  @param[in] device_id The device identifier.
 *  @param[in] packet The received packet or the received packet queue.
 *  @param[in] target The target internetwork module service to be delivered to.
 *  @param[in] error The packet error reporting service. Prefixes the received packet.
 *  @returns EOK on success.
 */
int	ip_received_error_msg( int ip_phone, device_id_t device_id, packet_t packet, services_t target, services_t error );

/** Returns the device identifier and the IP pseudo header based on the destination address.
 *  @param[in] ip_phone The IP module phone used for (semi)remote calls.
 *  @param[in] protocol The transport protocol.
 *  @param[in] destination The destination address.
 *  @param[in] addrlen The destination address length.
 *  @param[out] device_id The device identifier.
 *  @param[out] header The constructed IP pseudo header.
 *  @param[out] headerlen The IP pseudo header length.
 */
int	ip_get_route_req( int ip_phone, ip_protocol_t protocol, const struct sockaddr * destination, socklen_t addrlen, device_id_t * device_id, ip_pseudo_header_ref * header, size_t * headerlen );

/*@}*/

#endif

/** @}
 */
