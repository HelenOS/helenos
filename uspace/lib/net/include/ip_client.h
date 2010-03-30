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
 *  IP client interface.
 */

#ifndef __NET_IP_CLIENT_H__
#define __NET_IP_CLIENT_H__

#include <sys/types.h>

#include <packet/packet.h>
#include <ip_codes.h>
#include <ip_interface.h>
#include <socket_codes.h>

/** Prepares the packet to be transfered via IP.
 *  The IP header is prefixed.
 *  @param[in,out] packet The packet to be prepared.
 *  @param[in] protocol The transport protocol.
 *  @param[in] ttl The time to live counter. The IPDEFTTL is set if zero (0).
 *  @param[in] tos The type of service.
 *  @param[in] dont_fragment The value indicating whether fragmentation is disabled.
 *  @param[in] ipopt_length The prefixed IP options length in bytes.
 *  @returns EOK on success.
 *  @returns ENOMEM if there is not enough memory left in the packet.
 */
extern int ip_client_prepare_packet(packet_t packet, ip_protocol_t protocol, ip_ttl_t ttl, ip_tos_t tos, int dont_fragment, size_t ipopt_length);

/** Processes the received IP packet.
 *  Fills set header fields.
 *  Returns the prefixed IP header length.
 *  @param[in] packet The received packet.
 *  @param[out] protocol The transport protocol. May be NULL if not desired.
 *  @param[out] ttl The time to live counter. May be NULL if not desired.
 *  @param[out] tos The type of service. May be NULL if not desired.
 *  @param[out] dont_fragment The value indicating whether the fragmentation is disabled. May be NULL if not desired.
 *  @param[out] ipopt_length The IP options length in bytes. May be NULL if not desired.
 *  @returns The prefixed IP header length in bytes on success.
 *  @returns ENOMEM if the packet is too short to contain the IP header.
 */
extern int ip_client_process_packet(packet_t packet, ip_protocol_t * protocol, ip_ttl_t * ttl, ip_tos_t * tos, int * dont_fragment, size_t * ipopt_length);

/** Returns the IP header length.
 *  @param[in] packet The packet.
 *  @returns The IP header length in bytes.
 *  @returns Zero (0) if there is no IP header.
 */
extern size_t ip_client_header_length(packet_t packet);

/** Updates the IPv4 pseudo header data length field.
 *  @param[in,out] header The IPv4 pseudo header to be updated.
 *  @param[in] headerlen The length of the IP pseudo header in bytes.
 *  @param[in] data_length The data length to be set.
 *  @returns EOK on success.
 *  @returns EBADMEM if the header parameter is NULL.
 *  @returns EINVAL if the headerlen parameter is not IPv4 pseudo header length.
 */
extern int ip_client_set_pseudo_header_data_length(ip_pseudo_header_ref header, size_t headerlen, size_t data_length);

/** Constructs the IPv4 pseudo header.
 *  @param[in] protocol The transport protocol.
 *  @param[in] src The source address.
 *  @param[in] srclen The source address length.
 *  @param[in] dest The destination address.
 *  @param[in] destlen The destination address length.
 *  @param[in] data_length The data length to be set.
 *  @param[out] header The constructed IPv4 pseudo header.
 *  @param[out] headerlen The length of the IP pseudo header in bytes.
 *  @returns EOK on success.
 *  @returns EBADMEM if the header and/or the headerlen parameter is NULL.
 *  @returns EINVAL if the source address and/or the destination address parameter is NULL.
 *  @returns EINVAL if the source address length is less than struct sockaddr length.
 *  @returns EINVAL if the source address length differs from the destination address length.
 *  @returns EINVAL if the source address family differs from the destination family.
 *  @returns EAFNOSUPPORT if the address family is not supported.
 *  @returns ENOMEM if there is not enough memory left.
 */
extern int ip_client_get_pseudo_header(ip_protocol_t protocol, struct sockaddr * src, socklen_t srclen, struct sockaddr * dest, socklen_t destlen, size_t data_length, ip_pseudo_header_ref * header, size_t * headerlen);

// TODO ipopt manipulation

#endif

/** @}
 */
