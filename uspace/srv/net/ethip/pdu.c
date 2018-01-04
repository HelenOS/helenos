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

#include <byteorder.h>
#include <errno.h>
#include <io/log.h>
#include <macros.h>
#include <mem.h>
#include <stdlib.h>

#include "ethip.h"
#include "std.h"
#include "pdu.h"

/** Encode Ethernet PDU. */
errno_t eth_pdu_encode(eth_frame_t *frame, void **rdata, size_t *rsize)
{
	void *data;
	size_t size;
	eth_header_t *hdr;

	size = max(sizeof(eth_header_t) + frame->size, ETH_FRAME_MIN_SIZE);

	data = calloc(size, 1);
	if (data == NULL)
		return ENOMEM;

	hdr = (eth_header_t *)data;
	addr48(frame->src, hdr->src);
	addr48(frame->dest, hdr->dest);
	hdr->etype_len = host2uint16_t_be(frame->etype_len);

	memcpy((uint8_t *)data + sizeof(eth_header_t), frame->data,
	    frame->size);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Encoded Ethernet frame (%zu bytes)", size);

	*rdata = data;
	*rsize = size;
	return EOK;
}

/** Decode Ethernet PDU. */
errno_t eth_pdu_decode(void *data, size_t size, eth_frame_t *frame)
{
	eth_header_t *hdr;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "eth_pdu_decode()");

	if (size < sizeof(eth_header_t)) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "PDU too short (%zu)", size);
		return EINVAL;
	}

	hdr = (eth_header_t *)data;

	frame->size = size - sizeof(eth_header_t);
	frame->data = calloc(frame->size, 1);
	if (frame->data == NULL)
		return ENOMEM;

	addr48(hdr->src, frame->src);
	addr48(hdr->dest, frame->dest);
	frame->etype_len = uint16_t_be2host(hdr->etype_len);

	memcpy(frame->data, (uint8_t *)data + sizeof(eth_header_t),
	    frame->size);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Decoded Ethernet frame payload (%zu bytes)", frame->size);

	return EOK;
}

/** Encode ARP PDU. */
errno_t arp_pdu_encode(arp_eth_packet_t *packet, void **rdata, size_t *rsize)
{
	void *data;
	size_t size;
	arp_eth_packet_fmt_t *pfmt;
	uint16_t fopcode;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "arp_pdu_encode()");

	size = sizeof(arp_eth_packet_fmt_t);

	data = calloc(size, 1);
	if (data == NULL)
		return ENOMEM;

	pfmt = (arp_eth_packet_fmt_t *)data;

	switch (packet->opcode) {
	case aop_request: fopcode = AOP_REQUEST; break;
	case aop_reply: fopcode = AOP_REPLY; break;
	default:
		assert(false);
		fopcode = 0;
	}

	pfmt->hw_addr_space = host2uint16_t_be(AHRD_ETHERNET);
	pfmt->proto_addr_space = host2uint16_t_be(ETYPE_IP);
	pfmt->hw_addr_size = ETH_ADDR_SIZE;
	pfmt->proto_addr_size = IPV4_ADDR_SIZE;
	pfmt->opcode = host2uint16_t_be(fopcode);
	addr48(packet->sender_hw_addr, pfmt->sender_hw_addr);
	pfmt->sender_proto_addr =
	    host2uint32_t_be(packet->sender_proto_addr);
	addr48(packet->target_hw_addr, pfmt->target_hw_addr);
	pfmt->target_proto_addr =
	    host2uint32_t_be(packet->target_proto_addr);

	*rdata = data;
	*rsize = size;
	return EOK;
}

/** Decode ARP PDU. */
errno_t arp_pdu_decode(void *data, size_t size, arp_eth_packet_t *packet)
{
	arp_eth_packet_fmt_t *pfmt;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "arp_pdu_decode()");

	if (size < sizeof(arp_eth_packet_fmt_t)) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "ARP PDU too short (%zu)", size);
		return EINVAL;
	}

	pfmt = (arp_eth_packet_fmt_t *)data;

	if (uint16_t_be2host(pfmt->hw_addr_space) != AHRD_ETHERNET) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "HW address space != %u (%" PRIu16 ")",
		    AHRD_ETHERNET, uint16_t_be2host(pfmt->hw_addr_space));
		return EINVAL;
	}

	if (uint16_t_be2host(pfmt->proto_addr_space) != 0x0800) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Proto address space != %u (%" PRIu16 ")",
		    ETYPE_IP, uint16_t_be2host(pfmt->proto_addr_space));
		return EINVAL;
	}

	if (pfmt->hw_addr_size != ETH_ADDR_SIZE) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "HW address size != %zu (%zu)",
		    (size_t)ETH_ADDR_SIZE, (size_t)pfmt->hw_addr_size);
		return EINVAL;
	}

	if (pfmt->proto_addr_size != IPV4_ADDR_SIZE) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Proto address size != %zu (%zu)",
		    (size_t)IPV4_ADDR_SIZE, (size_t)pfmt->proto_addr_size);
		return EINVAL;
	}

	switch (uint16_t_be2host(pfmt->opcode)) {
	case AOP_REQUEST: packet->opcode = aop_request; break;
	case AOP_REPLY: packet->opcode = aop_reply; break;
	default:
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Invalid ARP opcode (%" PRIu16 ")",
		    uint16_t_be2host(pfmt->opcode));
		return EINVAL;
	}

	addr48(pfmt->sender_hw_addr, packet->sender_hw_addr);
	packet->sender_proto_addr =
	    uint32_t_be2host(pfmt->sender_proto_addr);
	addr48(pfmt->target_hw_addr, packet->target_hw_addr);
	packet->target_proto_addr =
	    uint32_t_be2host(pfmt->target_proto_addr);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "packet->tpa = %x\n", pfmt->target_proto_addr);

	return EOK;
}

/** @}
 */
