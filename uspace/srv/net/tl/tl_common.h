/*
 * Copyright (c) 2008 Lukas Mejdrech
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

/** @addtogroup net_tl
 *  @{
 */

/** @file
 *  Transport layer common functions.
 */

#ifndef __NET_TL_COMMON_H__
#define __NET_TL_COMMON_H__

#include "../structures/packet/packet.h"

#include "../include/device.h"
#include "../include/inet.h"
#include "../include/socket_codes.h"

/** Device packet dimensions.
 *  Maps devices to the packet dimensions.
 *  @see device.h
 */
DEVICE_MAP_DECLARE(packet_dimensions, packet_dimension_t);

/** Gets the address port.
 *  Supports AF_INET and AF_INET6 address families.
 *  @param[in,out] addr The address to be updated.
 *  @param[in] addrlen The address length.
 *  @param[out] port The set port.
 *  @returns EOK on success.
 *  @returns EINVAL if the address length does not match the address family.
 *  @returns EAFNOSUPPORT if the address family is not supported.
 */
int tl_get_address_port(const struct sockaddr * addr, int addrlen, uint16_t * port);

/** Gets IP packet dimensions.
 *  Tries to search a cache and queries the IP module if not found.
 *  The reply is cached then.
 *  @param[in] ip_phone The IP moduel phone for (semi)remote calls.
 *  @param[in] packet_dimensions The packet dimensions cache.
 *  @param[in] device_id The device identifier.
 *  @param[out] packet_dimension The IP packet dimensions.
 *  @returns EOK on success.
 *  @returns EBADMEM if the packet_dimension parameter is NULL.
 *  @return ENOMEM if there is not enough memory left.
 *  @returns EINVAL if the packet_dimensions cache is not valid.
 *  @returns Other codes as defined for the ip_packet_size_req() function.
 */
int tl_get_ip_packet_dimension(int ip_phone, packet_dimensions_ref packet_dimensions, device_id_t device_id, packet_dimension_ref * packet_dimension);

/** Updates IP device packet dimensions cache.
 *  @param[in,out] packet_dimensions The packet dimensions cache.
 *  @param[in] device_id The device identifier.
 *  @param[in] content The new maximum content size.
 *  @returns EOK on success.
 *  @returns ENOENT if the packet dimension is not cached.
 */
int tl_update_ip_packet_dimension(packet_dimensions_ref packet_dimensions, device_id_t device_id, size_t content);

/** Sets the address port.
 *  Supports AF_INET and AF_INET6 address families.
 *  @param[in,out] addr The address to be updated.
 *  @param[in] addrlen The address length.
 *  @param[in] port The port to be set.
 *  @returns EOK on success.
 *  @returns EINVAL if the address length does not match the address family.
 *  @returns EAFNOSUPPORT if the address family is not supported.
 */
int tl_set_address_port(struct sockaddr * addr, int addrlen, uint16_t port);

/** Prepares the packet for ICMP error notification.
 *  Keeps the first packet and releases all the others.
 *  Releases all the packets on error.
 *  @param[in] packet_phone The packet server module phone.
 *  @param[in] icmp_phone The ICMP module phone.
 *  @param[in] packet The packet to be send.
 *  @param[in] error The packet error reporting service. Prefixes the received packet.
 *  @returns EOK on success.
 *  @returns ENOENT if no packet may be sent.
 */
int tl_prepare_icmp_packet(int packet_phone, int icmp_phone, packet_t packet, services_t error);

/** Receives data from the socket into a packet.
 *  @param[in] packet_phone The packet server module phone.
 *  @param[out] packet The new created packet.
 *  @param[in] prefix Reserved packet data prefix length.
 *  @param[in] dimension The packet dimension.
 *  @param[in] addr The destination address.
 *  @param[in] addrlen The address length.
 *  @returns Number of bytes received.
 *  @returns EINVAL if the client does not send data.
 *  @returns ENOMEM if there is not enough memory left.
 *  @returns Other error codes as defined for the async_data_read_finalize() function.
 */
int tl_socket_read_packet_data(int packet_phone, packet_ref packet, size_t prefix, const packet_dimension_ref dimension, const struct sockaddr * addr, socklen_t addrlen);

#endif

/** @}
 */

