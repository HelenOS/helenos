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

/** Encode Internet PDU.
 *
 * Encode internet packet into PDU (serialized form). Will encode a
 * fragment of the payload starting at offset @a offs. The resulting
 * PDU will have at most @a mtu bytes. @a *roffs will be set to the offset
 * of remaining payload. If some data is remaining, the MF flag will
 * be set in the header, otherwise the offset will equal @a packet->size.
 *
 * @param packet	Packet to encode
 * @param offs		Offset into packet payload (in bytes)
 * @param mtu		MTU (Maximum Transmission Unit) in bytes
 * @param rdata		Place to store pointer to allocated data buffer
 * @param rsize		Place to store size of allocated data buffer
 * @param roffs		Place to store offset of remaning data
 */
int inet_pdu_encode(inet_packet_t *packet, size_t offs, size_t mtu,
    void **rdata, size_t *rsize, size_t *roffs)
{
	void *data;
	size_t size;
	ip_header_t *hdr;
	size_t hdr_size;
	size_t data_offs;
	uint16_t chksum;
	uint16_t ident;
	uint16_t flags_foff;
	uint16_t foff;
	size_t fragoff_limit;
	size_t xfer_size;
	size_t spc_avail;
	size_t rem_offs;

	/* Upper bound for fragment offset field */
	fragoff_limit = 1 << (FF_FRAGOFF_h - FF_FRAGOFF_l);

	/* Verify that total size of datagram is within reasonable bounds */
	if (offs + packet->size > FRAG_OFFS_UNIT * fragoff_limit)
		return ELIMIT;

	hdr_size = sizeof(ip_header_t);
	data_offs = ROUND_UP(hdr_size, 4);

	assert(offs % FRAG_OFFS_UNIT == 0);
	assert(offs / FRAG_OFFS_UNIT < fragoff_limit);

	/* Value for the fragment offset field */
	foff = offs / FRAG_OFFS_UNIT;

	if (hdr_size >= mtu)
		return EINVAL;

	/* Amount of space in the PDU available for payload */
	spc_avail = mtu - hdr_size;
	spc_avail -= (spc_avail % FRAG_OFFS_UNIT);

	/* Amount of data (payload) to transfer */
	xfer_size = min(packet->size - offs, spc_avail);

	/* Total PDU size */
	size = hdr_size + xfer_size;

	/* Offset of remaining payload */
	rem_offs = offs + xfer_size;

	/* Flags */
	flags_foff =
	    (packet->df ? BIT_V(uint16_t, FF_FLAG_DF) : 0) +
	    (rem_offs < packet->size ? BIT_V(uint16_t, FF_FLAG_MF) : 0) +
	    (foff << FF_FRAGOFF_l);

	data = calloc(size, 1);
	if (data == NULL)
		return ENOMEM;

	/* Allocate identifier */
	fibril_mutex_lock(&ip_ident_lock);
	ident = ++ip_ident;
	fibril_mutex_unlock(&ip_ident_lock);

	/* Encode header fields */
	hdr = (ip_header_t *)data;
	hdr->ver_ihl = (4 << VI_VERSION_l) | (hdr_size / sizeof(uint32_t));
	hdr->tos = packet->tos;
	hdr->tot_len = host2uint16_t_be(size);
	hdr->id = host2uint16_t_be(ident);
	hdr->flags_foff = host2uint16_t_be(flags_foff);
	hdr->ttl = packet->ttl;
	hdr->proto = packet->proto;
	hdr->chksum = 0;
	hdr->src_addr = host2uint32_t_be(packet->src.ipv4);
	hdr->dest_addr = host2uint32_t_be(packet->dest.ipv4);

	/* Compute checksum */
	chksum = inet_checksum_calc(INET_CHECKSUM_INIT, (void *)hdr, hdr_size);
	hdr->chksum = host2uint16_t_be(chksum);

	/* Copy payload */
	memcpy((uint8_t *)data + data_offs, packet->data + offs, xfer_size);

	*rdata = data;
	*rsize = size;
	*roffs = rem_offs;

	return EOK;
}

int inet_pdu_decode(void *data, size_t size, inet_packet_t *packet)
{
	ip_header_t *hdr;
	size_t tot_len;
	size_t data_offs;
	uint8_t version;
	uint16_t ident;
	uint16_t flags_foff;
	uint16_t foff;

	log_msg(LVL_DEBUG, "inet_pdu_decode()");

	if (size < sizeof(ip_header_t)) {
		log_msg(LVL_DEBUG, "PDU too short (%zu)", size);
		return EINVAL;
	}

	hdr = (ip_header_t *)data;

	version = BIT_RANGE_EXTRACT(uint8_t, VI_VERSION_h, VI_VERSION_l,
	    hdr->ver_ihl);
	if (version != 4) {
		log_msg(LVL_DEBUG, "Version (%d) != 4", version);
		return EINVAL;
	}

	tot_len = uint16_t_be2host(hdr->tot_len);
	if (tot_len < sizeof(ip_header_t)) {
		log_msg(LVL_DEBUG, "Total Length too small (%zu)", tot_len);
		return EINVAL;
	}

	if (tot_len > size) {
		log_msg(LVL_DEBUG, "Total Length = %zu > PDU size = %zu",
			tot_len, size);
		return EINVAL;
	}

	ident = uint16_t_be2host(hdr->id);
	flags_foff = uint16_t_be2host(hdr->flags_foff);
	foff = BIT_RANGE_EXTRACT(uint16_t, FF_FRAGOFF_h, FF_FRAGOFF_l,
	    flags_foff);
	/* XXX Checksum */

	packet->src.ipv4 = uint32_t_be2host(hdr->src_addr);
	packet->dest.ipv4 = uint32_t_be2host(hdr->dest_addr);
	packet->tos = hdr->tos;
	packet->proto = hdr->proto;
	packet->ttl = hdr->ttl;
	packet->ident = ident;

	packet->df = (flags_foff & BIT_V(uint16_t, FF_FLAG_DF)) != 0;
	packet->mf = (flags_foff & BIT_V(uint16_t, FF_FLAG_MF)) != 0;
	packet->offs = foff * FRAG_OFFS_UNIT;

	/* XXX IP options */
	data_offs = sizeof(uint32_t) * BIT_RANGE_EXTRACT(uint8_t, VI_IHL_h,
	    VI_IHL_l, hdr->ver_ihl);

	packet->size = tot_len - data_offs;
	packet->data = calloc(packet->size, 1);
	if (packet->data == NULL) {
		log_msg(LVL_WARN, "Out of memory.");
		return ENOMEM;
	}

	memcpy(packet->data, (uint8_t *)data + data_offs, packet->size);

	return EOK;
}

/** @}
 */
