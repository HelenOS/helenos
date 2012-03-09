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
#include <mem.h>
#include <stdlib.h>

#include "inet.h"
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
 */
int inet_pdu_encode(inet_packet_t *packet, void **rdata, size_t *rsize)
{
	void *data;
	size_t size;
	ip_header_t *hdr;
	size_t hdr_size;
	size_t data_offs;
	uint16_t chksum;
	uint16_t ident;

	fibril_mutex_lock(&ip_ident_lock);
	ident = ++ip_ident;
	fibril_mutex_unlock(&ip_ident_lock);

	hdr_size = sizeof(ip_header_t);
	size = hdr_size + packet->size;
	data_offs = ROUND_UP(hdr_size, 4);

	data = calloc(size, 1);
	if (data == NULL)
		return ENOMEM;

	hdr = (ip_header_t *)data;
	hdr->ver_ihl = (4 << VI_VERSION_l) | (hdr_size / sizeof(uint32_t));
	hdr->tos = packet->tos;
	hdr->tot_len = host2uint16_t_be(size);
	hdr->id = host2uint16_t_be(ident);
	hdr->flags_foff = host2uint16_t_be(packet->df ?
	    BIT_V(uint16_t, FF_FLAG_DF) : 0);
	hdr->ttl = packet->ttl;
	hdr->proto = packet->proto;
	hdr->chksum = 0;
	hdr->src_addr = host2uint32_t_be(packet->src.ipv4);
	hdr->dest_addr = host2uint32_t_be(packet->dest.ipv4);

	chksum = inet_checksum_calc(INET_CHECKSUM_INIT, (void *)hdr, hdr_size);
	hdr->chksum = host2uint16_t_be(chksum);

	memcpy((uint8_t *)data + data_offs, packet->data, packet->size);

	*rdata = data;
	*rsize = size;
	return EOK;
}

int inet_pdu_decode(void *data, size_t size, inet_packet_t *packet)
{
	ip_header_t *hdr;
	size_t tot_len;
	size_t data_offs;
	uint8_t version;
	uint16_t ident;

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
	(void)ident;
	/* XXX Flags */
	/* XXX Fragment offset */
	/* XXX Checksum */

	packet->src.ipv4 = uint32_t_be2host(hdr->src_addr);
	packet->dest.ipv4 = uint32_t_be2host(hdr->dest_addr);
	packet->tos = hdr->tos;
	packet->proto = hdr->proto;
	packet->ttl = hdr->ttl;
	packet->df = (uint16_t_be2host(hdr->tos) & BIT_V(uint16_t, FF_FLAG_DF))
	    ? 1 : 0;

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
