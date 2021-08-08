/*
 * Copyright (c) 2021 Jiri Svoboda
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
#include <inet/iplink_srv.h>
#include <inet/addr.h>
#include <inet/eth_addr.h>
#include <io/log.h>
#include <stdlib.h>
#include "arp.h"
#include "atrans.h"
#include "ethip.h"
#include "ethip_nic.h"
#include "pdu.h"
#include "std.h"

/** Time to wait for ARP reply in microseconds */
#define ARP_REQUEST_TIMEOUT (3 * 1000 * 1000)

static errno_t arp_send_packet(ethip_nic_t *nic, arp_eth_packet_t *packet);

void arp_received(ethip_nic_t *nic, eth_frame_t *frame)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "arp_received()");

	arp_eth_packet_t packet;
	errno_t rc = arp_pdu_decode(frame->data, frame->size, &packet);
	if (rc != EOK)
		return;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ARP PDU decoded, opcode=%d, tpa=%x",
	    packet.opcode, packet.target_proto_addr);

	inet_addr_t addr;
	inet_addr_set(packet.target_proto_addr, &addr);

	ethip_link_addr_t *laddr = ethip_nic_addr_find(nic, &addr);
	if (laddr == NULL)
		return;

	addr32_t laddr_v4;
	ip_ver_t laddr_ver = inet_addr_get(&laddr->addr, &laddr_v4, NULL);
	if (laddr_ver != ip_v4)
		return;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Request/reply to my address");

	(void) atrans_add(packet.sender_proto_addr,
	    &packet.sender_hw_addr);

	if (packet.opcode == aop_request) {
		arp_eth_packet_t reply;

		reply.opcode = aop_reply;
		reply.sender_hw_addr = nic->mac_addr;
		reply.sender_proto_addr = laddr_v4;
		reply.target_hw_addr = packet.sender_hw_addr;
		reply.target_proto_addr = packet.sender_proto_addr;

		arp_send_packet(nic, &reply);
	}
}

errno_t arp_translate(ethip_nic_t *nic, addr32_t src_addr, addr32_t ip_addr,
    eth_addr_t *mac_addr)
{
	/* Broadcast address */
	if (ip_addr == addr32_broadcast_all_hosts) {
		*mac_addr = eth_addr_broadcast;
		return EOK;
	}

	errno_t rc = atrans_lookup(ip_addr, mac_addr);
	if (rc == EOK)
		return EOK;

	arp_eth_packet_t packet;

	packet.opcode = aop_request;
	packet.sender_hw_addr = nic->mac_addr;
	packet.sender_proto_addr = src_addr;
	packet.target_hw_addr = eth_addr_broadcast;
	packet.target_proto_addr = ip_addr;

	rc = arp_send_packet(nic, &packet);
	if (rc != EOK)
		return rc;

	return atrans_lookup_timeout(ip_addr, ARP_REQUEST_TIMEOUT, mac_addr);
}

static errno_t arp_send_packet(ethip_nic_t *nic, arp_eth_packet_t *packet)
{
	errno_t rc;
	void *pdata;
	size_t psize;

	eth_frame_t frame;
	void *fdata;
	size_t fsize;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "arp_send_packet()");

	rc = arp_pdu_encode(packet, &pdata, &psize);
	if (rc != EOK)
		return rc;

	frame.dest = packet->target_hw_addr;
	frame.src = packet->sender_hw_addr;
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
