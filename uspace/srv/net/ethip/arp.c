/*
 * Copyright (c) 2012 Jiri Svoboda
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

/** @addtogroup inet
 * @{
 */
/**
 * @file
 * @brief
 */

#include <errno.h>
#include <io/log.h>
#include <inet/iplink_srv.h>
#include <stdlib.h>

#include "arp.h"
#include "atrans.h"
#include "ethip.h"
#include "ethip_nic.h"
#include "pdu.h"
#include "std.h"

/** Time to wait for ARP reply in microseconds */
#define ARP_REQUEST_TIMEOUT (3 * 1000 * 1000)

static int arp_send_packet(ethip_nic_t *nic, arp_eth_packet_t *packet);

void arp_received(ethip_nic_t *nic, eth_frame_t *frame)
{
	int rc;
	arp_eth_packet_t packet;
	arp_eth_packet_t reply;
	ethip_link_addr_t *laddr;

	log_msg(LVL_DEBUG, "arp_received()");

	rc = arp_pdu_decode(frame->data, frame->size, &packet);
	if (rc != EOK)
		return;

	log_msg(LVL_DEBUG, "ARP PDU decoded, opcode=%d, tpa=%x",
	    packet.opcode, packet.target_proto_addr.ipv4);

	laddr = ethip_nic_addr_find(nic, &packet.target_proto_addr);
	if (laddr != NULL) {
		log_msg(LVL_DEBUG, "Request/reply to my address");

		(void) atrans_add(&packet.sender_proto_addr,
		    &packet.sender_hw_addr);

		if (packet.opcode == aop_request) {
        		reply.opcode = aop_reply;
        		reply.sender_hw_addr = nic->mac_addr;
			reply.sender_proto_addr = laddr->addr;
			reply.target_hw_addr = packet.sender_hw_addr;
			reply.target_proto_addr = packet.sender_proto_addr;

			arp_send_packet(nic, &reply);
		}
	}
}

int arp_translate(ethip_nic_t *nic, iplink_srv_addr_t *src_addr,
    iplink_srv_addr_t *ip_addr, mac48_addr_t *mac_addr)
{
	int rc;
	arp_eth_packet_t packet;

	rc = atrans_lookup(ip_addr, mac_addr);
	if (rc == EOK)
		return EOK;

	packet.opcode = aop_request;
	packet.sender_hw_addr = nic->mac_addr;
	packet.sender_proto_addr = *src_addr;
	packet.target_hw_addr.addr = MAC48_BROADCAST;
	packet.target_proto_addr = *ip_addr;

	rc = arp_send_packet(nic, &packet);
	if (rc != EOK)
		return rc;

	(void) atrans_wait_timeout(ARP_REQUEST_TIMEOUT);

	return atrans_lookup(ip_addr, mac_addr);
}

static int arp_send_packet(ethip_nic_t *nic, arp_eth_packet_t *packet)
{
	int rc;
	void *pdata;
	size_t psize;

	eth_frame_t frame;
	void *fdata;
	size_t fsize;

	log_msg(LVL_DEBUG, "arp_send_packet()");

	rc = arp_pdu_encode(packet, &pdata, &psize);
	if (rc != EOK)
		return rc;

	frame.dest.addr = packet->target_hw_addr.addr;
	frame.src.addr =  packet->sender_hw_addr.addr;
	frame.etype_len = ETYPE_ARP;
	frame.data = pdata;
	frame.size = psize;

	rc = eth_pdu_encode(&frame, &fdata, &fsize);
	if (rc != EOK) {
		free(pdata);
		return rc;
	}

	rc = ethip_nic_send(nic, fdata, fsize);
	free(fdata);
	free(pdata);

	return rc;

}

/** @}
 */
