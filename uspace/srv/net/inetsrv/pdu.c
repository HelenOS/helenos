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

#include <align.h>
#include <bitops.h>
#include <byteorder.h>
#include <errno.h>
#include <fibril_synch.h>
#include <io/log.h>
#include <macros.h>
#include <mem.h>
#include <stdlib.h>
#include <net/socket_codes.h>
#include "inetsrv.h"
#include "inet_std.h"
#include "pdu.h"

static FIBRIL_MUTEX_INITIALIZE(ip_ident_lock);
static uint16_t ip_ident = 0;

/** One's complement addition.
 *
 * Result is a + b + carry.
 */
static uint16_t inet_ocadd16(uint16_t a, uint16_t b)
{
	uint32_t s;

	s = (uint32_t)a + (uint32_t)b;
	return (s & 0xffff) + (s >> 16);
}

uint16_t inet_checksum_calc(uint16_t ivalue, void *data, size_t size)
{
	uint16_t sum;
	uint16_t w;
	size_t words, i;
	uint8_t *bdata;

	sum = ~ivalue;
	words = size / 2;
	bdata = (uint8_t *)data;

	for (i = 0; i < words; i++) {
		w = ((uint16_t)bdata[2*i] << 8) | bdata[2*i + 1];
		sum = inet_ocadd16(sum, w);
	}

	if (size % 2 != 0) {
		w = ((uint16_t)bdata[2*words] << 8);
		sum = inet_ocadd16(sum, w);
	}

	return ~sum;
}

/** Encode IPv4 PDU.
 *
 * Encode internet packet into PDU (serialized form). Will encode a
 * fragment of the payload starting at offset @a offs. The resulting
 * PDU will have at most @a mtu bytes. @a *roffs will be set to the offset
 * of remaining payload. If some data is remaining, the MF flag will
 * be set in the header, otherwise the offset will equal @a packet->size.
 *
 * @param packet Packet to encode
 * @param src    Source address
 * @param dest   Destination address
 * @param offs   Offset into packet payload (in bytes)
 * @param mtu    MTU (Maximum Transmission Unit) in bytes
 * @param rdata  Place to store pointer to allocated data buffer
 * @param rsize  Place to store size of allocated data buffer
 * @param roffs  Place to store offset of remaning data
 *
 */
int inet_pdu_encode(inet_packet_t *packet, addr32_t src, addr32_t dest,
   size_t offs, size_t mtu, void **rdata, size_t *rsize, size_t *roffs)
{
	/* Upper bound for fragment offset field */
	size_t fragoff_limit = 1 << (FF_FRAGOFF_h - FF_FRAGOFF_l);
	
	/* Verify that total size of datagram is within reasonable bounds */
	if (offs + packet->size > FRAG_OFFS_UNIT * fragoff_limit)
		return ELIMIT;
	
	size_t hdr_size = sizeof(ip_header_t);
	
	size_t data_offs = ROUND_UP(hdr_size, 4);
	
	assert(offs % FRAG_OFFS_UNIT == 0);
	assert(offs / FRAG_OFFS_UNIT < fragoff_limit);
	
	/* Value for the fragment offset field */
	uint16_t foff = offs / FRAG_OFFS_UNIT;
	
	if (hdr_size >= mtu)
		return EINVAL;
	
	/* Amount of space in the PDU available for payload */
	size_t spc_avail = mtu - hdr_size;
	spc_avail -= (spc_avail % FRAG_OFFS_UNIT);
	
	/* Amount of data (payload) to transfer */
	size_t xfer_size = min(packet->size - offs, spc_avail);
	
	/* Total PDU size */
	size_t size = hdr_size + xfer_size;
	
	/* Offset of remaining payload */
	size_t rem_offs = offs + xfer_size;
	
	/* Flags */
	uint16_t flags_foff =
	    (packet->df ? BIT_V(uint16_t, FF_FLAG_DF) : 0) +
	    (rem_offs < packet->size ? BIT_V(uint16_t, FF_FLAG_MF) : 0) +
	    (foff << FF_FRAGOFF_l);
	
	void *data = calloc(size, 1);
	if (data == NULL)
		return ENOMEM;
	
	/* Allocate identifier */
	fibril_mutex_lock(&ip_ident_lock);
	uint16_t ident = ++ip_ident;
	fibril_mutex_unlock(&ip_ident_lock);
	
	/* Encode header fields */
	ip_header_t *hdr = (ip_header_t *) data;
	
	hdr->ver_ihl =
	    (4 << VI_VERSION_l) | (hdr_size / sizeof(uint32_t));
	hdr->tos = packet->tos;
	hdr->tot_len = host2uint16_t_be(size);
	hdr->id = host2uint16_t_be(ident);
	hdr->flags_foff = host2uint16_t_be(flags_foff);
	hdr->ttl = packet->ttl;
	hdr->proto = packet->proto;
	hdr->chksum = 0;
	hdr->src_addr = host2uint32_t_be(src);
	hdr->dest_addr = host2uint32_t_be(dest);
	
	/* Compute checksum */
	uint16_t chksum = inet_checksum_calc(INET_CHECKSUM_INIT,
	    (void *) hdr, hdr_size);
	hdr->chksum = host2uint16_t_be(chksum);
	
	/* Copy payload */
	memcpy((uint8_t *) data + data_offs, packet->data + offs, xfer_size);
	
	*rdata = data;
	*rsize = size;
	*roffs = rem_offs;
	
	return EOK;
}

/** Encode IPv6 PDU.
 *
 * Encode internet packet into PDU (serialized form). Will encode a
 * fragment of the payload starting at offset @a offs. The resulting
 * PDU will have at most @a mtu bytes. @a *roffs will be set to the offset
 * of remaining payload. If some data is remaining, the MF flag will
 * be set in the header, otherwise the offset will equal @a packet->size.
 *
 * @param packet Packet to encode
 * @param src    Source address
 * @param dest   Destination address
 * @param offs   Offset into packet payload (in bytes)
 * @param mtu    MTU (Maximum Transmission Unit) in bytes
 * @param rdata  Place to store pointer to allocated data buffer
 * @param rsize  Place to store size of allocated data buffer
 * @param roffs Place to store offset of remaning data
 *
 */
int inet_pdu_encode6(inet_packet_t *packet, addr128_t src, addr128_t dest,
    size_t offs, size_t mtu, void **rdata, size_t *rsize, size_t *roffs)
{
	/* Upper bound for fragment offset field */
	size_t fragoff_limit = 1 << (FF_FRAGOFF_h - FF_FRAGOFF_l);
	
	/* Verify that total size of datagram is within reasonable bounds */
	if (offs + packet->size > FRAG_OFFS_UNIT * fragoff_limit)
		return ELIMIT;
	
	size_t hdr_size = sizeof(ip6_header_t);
	
	size_t data_offs = ROUND_UP(hdr_size, 4);
	
	assert(offs % FRAG_OFFS_UNIT == 0);
	assert(offs / FRAG_OFFS_UNIT < fragoff_limit);
	
#if 0
	// FIXME TODO fragmentation
	
	/* Value for the fragment offset field */
	uint16_t foff = offs / FRAG_OFFS_UNIT;
#endif
	
	if (hdr_size >= mtu)
		return EINVAL;
	
	/* Amount of space in the PDU available for payload */
	size_t spc_avail = mtu - hdr_size;
	spc_avail -= (spc_avail % FRAG_OFFS_UNIT);
	
	/* Amount of data (payload) to transfer */
	size_t xfer_size = min(packet->size - offs, spc_avail);
	
	/* Total PDU size */
	size_t size = hdr_size + xfer_size;
	
	/* Offset of remaining payload */
	size_t rem_offs = offs + xfer_size;
	
#if 0
	// FIXME TODO fragmentation
	
	/* Flags */
	uint16_t flags_foff =
	    (packet->df ? BIT_V(uint16_t, FF_FLAG_DF) : 0) +
	    (rem_offs < packet->size ? BIT_V(uint16_t, FF_FLAG_MF) : 0) +
	    (foff << FF_FRAGOFF_l);
#endif
	
	void *data = calloc(size, 1);
	if (data == NULL)
		return ENOMEM;
	
#if 0
	// FIXME TODO fragmentation
	
	/* Allocate identifier */
	fibril_mutex_lock(&ip_ident_lock);
	uint16_t ident = ++ip_ident;
	fibril_mutex_unlock(&ip_ident_lock);
#endif
	
	/* Encode header fields */
	ip6_header_t *hdr6 = (ip6_header_t *) data;
	
	hdr6->ver_tc = (6 << (VI_VERSION_l));
	memset(hdr6->tc_fl, 0, 3);
	hdr6->payload_len = host2uint16_t_be(packet->size);
	hdr6->next = packet->proto;
	hdr6->hop_limit = packet->ttl;
	
	host2addr128_t_be(src, hdr6->src_addr);
	host2addr128_t_be(dest, hdr6->dest_addr);
	
	/* Copy payload */
	memcpy((uint8_t *) data + data_offs, packet->data + offs, xfer_size);
	
	*rdata = data;
	*rsize = size;
	*roffs = rem_offs;
	
	return EOK;
}

int inet_pdu_decode(void *data, size_t size, inet_packet_t *packet)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_pdu_decode()");
	
	if (size < sizeof(ip_header_t)) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "PDU too short (%zu)", size);
		return EINVAL;
	}
	
	ip_header_t *hdr = (ip_header_t *) data;
	
	uint8_t version = BIT_RANGE_EXTRACT(uint8_t, VI_VERSION_h,
	    VI_VERSION_l, hdr->ver_ihl);
	if (version != 4) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Version (%d) != 4", version);
		return EINVAL;
	}
	
	size_t tot_len = uint16_t_be2host(hdr->tot_len);
	if (tot_len < sizeof(ip_header_t)) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Total Length too small (%zu)", tot_len);
		return EINVAL;
	}
	
	if (tot_len > size) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Total Length = %zu > PDU size = %zu",
		    tot_len, size);
		return EINVAL;
	}
	
	uint16_t ident = uint16_t_be2host(hdr->id);
	uint16_t flags_foff = uint16_t_be2host(hdr->flags_foff);
	uint16_t foff = BIT_RANGE_EXTRACT(uint16_t, FF_FRAGOFF_h, FF_FRAGOFF_l,
	    flags_foff);
	/* XXX Checksum */
	
	inet_addr_set(uint32_t_be2host(hdr->src_addr), &packet->src);
	inet_addr_set(uint32_t_be2host(hdr->dest_addr), &packet->dest);
	packet->tos = hdr->tos;
	packet->proto = hdr->proto;
	packet->ttl = hdr->ttl;
	packet->ident = ident;
	
	packet->df = (flags_foff & BIT_V(uint16_t, FF_FLAG_DF)) != 0;
	packet->mf = (flags_foff & BIT_V(uint16_t, FF_FLAG_MF)) != 0;
	packet->offs = foff * FRAG_OFFS_UNIT;
	
	/* XXX IP options */
	size_t data_offs = sizeof(uint32_t) *
	    BIT_RANGE_EXTRACT(uint8_t, VI_IHL_h, VI_IHL_l, hdr->ver_ihl);
	
	packet->size = tot_len - data_offs;
	packet->data = calloc(packet->size, 1);
	if (packet->data == NULL) {
		log_msg(LOG_DEFAULT, LVL_WARN, "Out of memory.");
		return ENOMEM;
	}
	
	memcpy(packet->data, (uint8_t *) data + data_offs, packet->size);
	
	return EOK;
}

int inet_pdu_decode6(void *data, size_t size, inet_packet_t *packet)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_pdu_decode6()");
	
	if (size < sizeof(ip6_header_t)) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "PDU too short (%zu)", size);
		return EINVAL;
	}
	
	ip6_header_t *hdr6 = (ip6_header_t *) data;
	
	uint8_t version = BIT_RANGE_EXTRACT(uint8_t, VI_VERSION_h,
	    VI_VERSION_l, hdr6->ver_tc);
	if (version != 6) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Version (%d) != 6", version);
		return EINVAL;
	}
	
	size_t payload_len = uint16_t_be2host(hdr6->payload_len);
	if (payload_len + sizeof(ip6_header_t) > size) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Total Length = %zu > PDU size = %zu",
		    payload_len + sizeof(ip6_header_t), size);
		return EINVAL;
	}
	
#if 0
	// FIXME TODO fragmentation
	
	uint16_t ident = uint16_t_be2host(hdr->id);
	uint16_t flags_foff = uint16_t_be2host(hdr->flags_foff);
	uint16_t foff = BIT_RANGE_EXTRACT(uint16_t, FF_FRAGOFF_h, FF_FRAGOFF_l,
	    flags_foff);
#endif
	
	/* XXX Checksum */
	
	addr128_t src;
	addr128_t dest;
	
	addr128_t_be2host(hdr6->src_addr, src);
	inet_addr_set6(src, &packet->src);
	
	addr128_t_be2host(hdr6->dest_addr, dest);
	inet_addr_set6(dest, &packet->dest);
	
	packet->tos = 0;
	packet->proto = hdr6->next;
	packet->ttl = hdr6->hop_limit;
	
#if 0
	// FIXME TODO fragmentation
	
	packet->ident = ident;
	packet->df = (flags_foff & BIT_V(uint16_t, FF_FLAG_DF)) != 0;
	packet->mf = (flags_foff & BIT_V(uint16_t, FF_FLAG_MF)) != 0;
	packet->offs = foff * FRAG_OFFS_UNIT;
	
	/* XXX IP options */
	size_t data_offs = sizeof(uint32_t) *
	    BIT_RANGE_EXTRACT(uint8_t, VI_IHL_h, VI_IHL_l, hdr->ver_ihl);
#endif
	
	packet->ident = 0;
	packet->df = 0;
	packet->mf = 0;
	packet->offs = 0;
	
	packet->size = payload_len;
	packet->data = calloc(packet->size, 1);
	if (packet->data == NULL) {
		log_msg(LOG_DEFAULT, LVL_WARN, "Out of memory.");
		return ENOMEM;
	}
	
	memcpy(packet->data, (uint8_t *) data + sizeof(ip6_header_t), packet->size);
	
	return EOK;
}

int ndp_pdu_encode(ndp_packet_t *ndp, inet_dgram_t *dgram)
{
	inet_addr_set6(ndp->sender_proto_addr, &dgram->src);
	inet_addr_set6(ndp->target_proto_addr, &dgram->dest);
	dgram->tos = 0;
	dgram->size = sizeof(icmpv6_message_t) + sizeof(ndp_message_t);
	
	dgram->data = calloc(1, dgram->size);
	if (dgram->data == NULL)
		return ENOMEM;
	
	icmpv6_message_t *icmpv6 = (icmpv6_message_t *) dgram->data;
	
	icmpv6->type = ndp->opcode;
	icmpv6->code = 0;
	memset(icmpv6->un.ndp.reserved, 0, 3);
	
	ndp_message_t *message = (ndp_message_t *) (icmpv6 + 1);
	
	if (ndp->opcode == ICMPV6_NEIGHBOUR_SOLICITATION) {
		host2addr128_t_be(ndp->solicited_ip, message->target_address);
		message->option = 1;
		icmpv6->un.ndp.flags = 0;
	} else {
		host2addr128_t_be(ndp->sender_proto_addr, message->target_address);
		message->option = 2;
		icmpv6->un.ndp.flags = NDP_FLAG_OVERRIDE | NDP_FLAG_SOLICITED;
	}
	
	message->length = 1;
	addr48(ndp->sender_hw_addr, message->mac);
	
	icmpv6_phdr_t phdr;
	
	host2addr128_t_be(ndp->sender_proto_addr, phdr.src_addr);
	host2addr128_t_be(ndp->target_proto_addr, phdr.dest_addr);
	phdr.length = host2uint32_t_be(dgram->size);
	memset(phdr.zeroes, 0, 3);
	phdr.next = IP_PROTO_ICMPV6;
	
	uint16_t cs_phdr =
	    inet_checksum_calc(INET_CHECKSUM_INIT, &phdr,
	    sizeof(icmpv6_phdr_t));
	
	uint16_t cs_all = inet_checksum_calc(cs_phdr, dgram->data,
	    dgram->size);
	
	icmpv6->checksum = host2uint16_t_be(cs_all);
	
	return EOK;
}

int ndp_pdu_decode(inet_dgram_t *dgram, ndp_packet_t *ndp)
{
	uint16_t src_af = inet_addr_get(&dgram->src, NULL,
	    &ndp->sender_proto_addr);
	if (src_af != AF_INET6)
		return EINVAL;
	
	if (dgram->size < sizeof(icmpv6_message_t) + sizeof(ndp_message_t))
		return EINVAL;
	
	icmpv6_message_t *icmpv6 = (icmpv6_message_t *) dgram->data;
	
	ndp->opcode = icmpv6->type;
	
	ndp_message_t *message = (ndp_message_t *) (icmpv6 + 1);
	
	addr128_t_be2host(message->target_address, ndp->target_proto_addr);
	addr48(message->mac, ndp->sender_hw_addr);
	
	return EOK;
}

/** @}
 */
