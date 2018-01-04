/*
 * Copyright (c) 2013 Antonin Steinhauser
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

/** @addtogroup ethip
 * @{
 */
/**
 * @file
 * @brief
 */

#include <errno.h>
#include <mem.h>
#include <stdlib.h>
#include <io/log.h>
#include "ntrans.h"
#include "addrobj.h"
#include "pdu.h"
#include "inet_link.h"
#include "ndp.h"

/** Time to wait for NDP reply in microseconds */
#define NDP_REQUEST_TIMEOUT  (3 * 1000 * 1000)

static addr128_t solicited_node_ip =
    {0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01, 0xff, 0, 0, 0};

/** Compute solicited node IPv6 multicast address from target IPv6 address
 *
 * @param ip_addr      Target IPv6 address
 * @param ip_solicited Solicited IPv6 address to be assigned
 *
 */
static void ndp_solicited_node_ip(addr128_t ip_addr,
    addr128_t ip_solicited)
{
	memcpy(ip_solicited, solicited_node_ip, 13);
	memcpy(ip_solicited + 13, ip_addr + 13, 3);
}

static errno_t ndp_send_packet(inet_link_t *link, ndp_packet_t *packet)
{
	inet_dgram_t dgram;
	ndp_pdu_encode(packet, &dgram);
	
	inet_link_send_dgram6(link, packet->target_hw_addr, &dgram,
	    IP_PROTO_ICMPV6, INET6_HOP_LIMIT_MAX, 0);
	
	free(dgram.data);
	
	return EOK;
}

static errno_t ndp_router_advertisement(inet_dgram_t *dgram, inet_addr_t *router)
{
	// FIXME TODO
	return ENOTSUP;
}

errno_t ndp_received(inet_dgram_t *dgram)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "ndp_received()");
	
	ndp_packet_t packet;
	errno_t rc = ndp_pdu_decode(dgram, &packet);
	if (rc != EOK)
		return rc;
	
	inet_addr_t sender;
	inet_addr_set6(packet.sender_proto_addr, &sender);
	
	inet_addr_t target;
	inet_addr_set6(packet.target_proto_addr, &target);
	
	inet_addrobj_t *laddr;
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, "NDP PDU decoded; opcode: %d",
	    packet.opcode);
	
	switch (packet.opcode) {
	case ICMPV6_NEIGHBOUR_SOLICITATION:
		laddr = inet_addrobj_find(&target, iaf_addr);
		if (laddr != NULL) {
			rc = ntrans_add(packet.sender_proto_addr,
			    packet.sender_hw_addr);
			if (rc != EOK)
				return rc;
			
			ndp_packet_t reply;
			
			reply.opcode = ICMPV6_NEIGHBOUR_ADVERTISEMENT;
			addr48(laddr->ilink->mac, reply.sender_hw_addr);
			addr128(packet.target_proto_addr, reply.sender_proto_addr);
			addr48(packet.sender_hw_addr, reply.target_hw_addr);
			addr128(packet.sender_proto_addr, reply.target_proto_addr);
			
			ndp_send_packet(laddr->ilink, &reply);
		}
		
		break;
	case ICMPV6_NEIGHBOUR_ADVERTISEMENT:
		laddr = inet_addrobj_find(&dgram->dest, iaf_addr);
		if (laddr != NULL)
			return ntrans_add(packet.sender_proto_addr,
			    packet.sender_hw_addr);
		
		break;
	case ICMPV6_ROUTER_ADVERTISEMENT:
		return ndp_router_advertisement(dgram, &sender);
	default:
		return ENOTSUP;
	}
	
	return EOK;
}

/** Translate IPv6 to MAC address
 *
 * @param src  Source IPv6 address
 * @param dest Destination IPv6 address
 * @param mac  Target MAC address to be assigned
 * @param link Network interface
 *
 * @return EOK on success
 * @return ENOENT when NDP translation failed
 *
 */
errno_t ndp_translate(addr128_t src_addr, addr128_t ip_addr, addr48_t mac_addr,
    inet_link_t *ilink)
{
	if (!ilink->mac_valid) {
		/* The link does not support NDP */
		memset(mac_addr, 0, 6);
		return EOK;
	}
	
	errno_t rc = ntrans_lookup(ip_addr, mac_addr);
	if (rc == EOK)
		return EOK;
	
	ndp_packet_t packet;
	
	packet.opcode = ICMPV6_NEIGHBOUR_SOLICITATION;
	addr48(ilink->mac, packet.sender_hw_addr);
	addr128(src_addr, packet.sender_proto_addr);
	addr128(ip_addr, packet.solicited_ip);
	addr48_solicited_node(ip_addr, packet.target_hw_addr);
	ndp_solicited_node_ip(ip_addr, packet.target_proto_addr);
	
	rc = ndp_send_packet(ilink, &packet);
	if (rc != EOK)
		return rc;
	
	(void) ntrans_wait_timeout(NDP_REQUEST_TIMEOUT);
	
	return ntrans_lookup(ip_addr, mac_addr);
}
