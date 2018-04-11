/*
 * Copyright (c) 2015 Jiri Svoboda
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

/** @addtogroup tcp
 * @{
 */

/**
 * @file TCP header encoding and decoding
 */

#include <bitops.h>
#include <byteorder.h>
#include <errno.h>
#include <inet/endpoint.h>
#include <mem.h>
#include <stdlib.h>
#include "pdu.h"
#include "segment.h"
#include "seq_no.h"
#include "std.h"
#include "tcp_type.h"

#define TCP_CHECKSUM_INIT 0xffff

/** One's complement addition.
 *
 * Result is a + b + carry.
 */
static uint16_t tcp_ocadd16(uint16_t a, uint16_t b)
{
	uint32_t s;

	s = (uint32_t)a + (uint32_t)b;
	return (s & 0xffff) + (s >> 16);
}

static uint16_t tcp_checksum_calc(uint16_t ivalue, void *data, size_t size)
{
	uint16_t sum;
	uint16_t w;
	size_t words, i;
	uint8_t *bdata;

	sum = ~ivalue;
	words = size / 2;
	bdata = (uint8_t *)data;

	for (i = 0; i < words; i++) {
		w = ((uint16_t)bdata[2 * i] << 8) | bdata[2 * i + 1];
		sum = tcp_ocadd16(sum, w);
	}

	if (size % 2 != 0) {
		w = ((uint16_t)bdata[2 * words] << 8);
		sum = tcp_ocadd16(sum, w);
	}

	return ~sum;
}

static void tcp_header_decode_flags(uint16_t doff_flags, tcp_control_t *rctl)
{
	tcp_control_t ctl;

	ctl = 0;

	if ((doff_flags & BIT_V(uint16_t, DF_URG)) != 0)
		ctl |= 0 /* XXX */;
	if ((doff_flags & BIT_V(uint16_t, DF_ACK)) != 0)
		ctl |= CTL_ACK;
	if ((doff_flags & BIT_V(uint16_t, DF_PSH)) != 0)
		ctl |= 0 /* XXX */;
	if ((doff_flags & BIT_V(uint16_t, DF_RST)) != 0)
		ctl |= CTL_RST;
	if ((doff_flags & BIT_V(uint16_t, DF_SYN)) != 0)
		ctl |= CTL_SYN;
	if ((doff_flags & BIT_V(uint16_t, DF_FIN)) != 0)
		ctl |= CTL_FIN;

	*rctl = ctl;
}

static void tcp_header_encode_flags(tcp_control_t ctl, uint16_t doff_flags0,
    uint16_t *rdoff_flags)
{
	uint16_t doff_flags;

	doff_flags = doff_flags0;

	if ((ctl & CTL_ACK) != 0)
		doff_flags |= BIT_V(uint16_t, DF_ACK);
	if ((ctl & CTL_RST) != 0)
		doff_flags |= BIT_V(uint16_t, DF_RST);
	if ((ctl & CTL_SYN) != 0)
		doff_flags |= BIT_V(uint16_t, DF_SYN);
	if ((ctl & CTL_FIN) != 0)
		doff_flags |= BIT_V(uint16_t, DF_FIN);

	*rdoff_flags = doff_flags;
}

static void tcp_header_setup(inet_ep2_t *epp, tcp_segment_t *seg, tcp_header_t *hdr)
{
	uint16_t doff_flags;
	uint16_t doff;

	hdr->src_port = host2uint16_t_be(epp->local.port);
	hdr->dest_port = host2uint16_t_be(epp->remote.port);
	hdr->seq = host2uint32_t_be(seg->seq);
	hdr->ack = host2uint32_t_be(seg->ack);

	doff = (sizeof(tcp_header_t) / sizeof(uint32_t)) << DF_DATA_OFFSET_l;
	tcp_header_encode_flags(seg->ctrl, doff, &doff_flags);

	hdr->doff_flags = host2uint16_t_be(doff_flags);
	hdr->window = host2uint16_t_be(seg->wnd);
	hdr->checksum = 0;
	hdr->urg_ptr = host2uint16_t_be(seg->up);
}

static ip_ver_t tcp_phdr_setup(tcp_pdu_t *pdu, tcp_phdr_t *phdr,
    tcp_phdr6_t *phdr6)
{
	addr32_t src_v4;
	addr128_t src_v6;
	uint16_t src_ver = inet_addr_get(&pdu->src, &src_v4, &src_v6);

	addr32_t dest_v4;
	addr128_t dest_v6;
	uint16_t dest_ver = inet_addr_get(&pdu->dest, &dest_v4, &dest_v6);

	assert(src_ver == dest_ver);

	switch (src_ver) {
	case ip_v4:
		phdr->src = host2uint32_t_be(src_v4);
		phdr->dest = host2uint32_t_be(dest_v4);
		phdr->zero = 0;
		phdr->protocol = IP_PROTO_TCP;
		phdr->tcp_length =
		    host2uint16_t_be(pdu->header_size + pdu->text_size);
		break;
	case ip_v6:
		host2addr128_t_be(src_v6, phdr6->src);
		host2addr128_t_be(dest_v6, phdr6->dest);
		phdr6->tcp_length =
		    host2uint32_t_be(pdu->header_size + pdu->text_size);
		memset(phdr6->zeroes, 0, 3);
		phdr6->next = IP_PROTO_TCP;
		break;
	default:
		assert(false);
	}

	return src_ver;
}

static void tcp_header_decode(tcp_header_t *hdr, tcp_segment_t *seg)
{
	tcp_header_decode_flags(uint16_t_be2host(hdr->doff_flags), &seg->ctrl);
	seg->seq = uint32_t_be2host(hdr->seq);
	seg->ack = uint32_t_be2host(hdr->ack);
	seg->wnd = uint16_t_be2host(hdr->window);
	seg->up = uint16_t_be2host(hdr->urg_ptr);
}

static errno_t tcp_header_encode(inet_ep2_t *epp, tcp_segment_t *seg,
    void **header, size_t *size)
{
	tcp_header_t *hdr;

	hdr = calloc(1, sizeof(tcp_header_t));
	if (hdr == NULL)
		return ENOMEM;

	tcp_header_setup(epp, seg, hdr);
	*header = hdr;
	*size = sizeof(tcp_header_t);

	return EOK;
}

static tcp_pdu_t *tcp_pdu_new(void)
{
	return calloc(1, sizeof(tcp_pdu_t));
}

/** Create PDU with the specified header and text data.
 *
 * Note that you still need to set addresses in the returned PDU.
 *
 * @param hdr		Header data
 * @param hdr_size      Header size in bytes
 * @param text		Text data
 * @param text_size	Text size in bytes
 * @return		New PDU
 */
tcp_pdu_t *tcp_pdu_create(void *hdr, size_t hdr_size, void *text,
    size_t text_size)
{
	tcp_pdu_t *pdu;

	pdu = tcp_pdu_new();
	if (pdu == NULL)
		return NULL;

	pdu->header = malloc(hdr_size);
	pdu->text = malloc(text_size);
	if (pdu->header == NULL || pdu->text == NULL)
		goto error;

	memcpy(pdu->header, hdr, hdr_size);
	memcpy(pdu->text, text, text_size);

	pdu->header_size = hdr_size;
	pdu->text_size = text_size;

	return pdu;

error:
	if (pdu->header != NULL)
		free(pdu->header);
	if (pdu->text != NULL)
		free(pdu->text);
	free(pdu);

	return NULL;
}

void tcp_pdu_delete(tcp_pdu_t *pdu)
{
	free(pdu->header);
	free(pdu->text);
	free(pdu);
}

static uint16_t tcp_pdu_checksum_calc(tcp_pdu_t *pdu)
{
	uint16_t cs_phdr;
	uint16_t cs_headers;
	tcp_phdr_t phdr;
	tcp_phdr6_t phdr6;

	ip_ver_t ver = tcp_phdr_setup(pdu, &phdr, &phdr6);
	switch (ver) {
	case ip_v4:
		cs_phdr = tcp_checksum_calc(TCP_CHECKSUM_INIT, (void *) &phdr,
		    sizeof(tcp_phdr_t));
		break;
	case ip_v6:
		cs_phdr = tcp_checksum_calc(TCP_CHECKSUM_INIT, (void *) &phdr6,
		    sizeof(tcp_phdr6_t));
		break;
	default:
		assert(false);
	}

	cs_headers = tcp_checksum_calc(cs_phdr, pdu->header, pdu->header_size);
	return tcp_checksum_calc(cs_headers, pdu->text, pdu->text_size);
}

static void tcp_pdu_set_checksum(tcp_pdu_t *pdu, uint16_t checksum)
{
	tcp_header_t *hdr;

	hdr = (tcp_header_t *)pdu->header;
	hdr->checksum = host2uint16_t_be(checksum);
}

/** Decode incoming PDU */
errno_t tcp_pdu_decode(tcp_pdu_t *pdu, inet_ep2_t *epp, tcp_segment_t **seg)
{
	tcp_segment_t *nseg;
	tcp_header_t *hdr;

	nseg = tcp_segment_make_data(0, pdu->text, pdu->text_size);
	if (nseg == NULL)
		return ENOMEM;

	tcp_header_decode(pdu->header, nseg);
	nseg->len += seq_no_control_len(nseg->ctrl);

	hdr = (tcp_header_t *)pdu->header;

	epp->local.port = uint16_t_be2host(hdr->dest_port);
	epp->local.addr = pdu->dest;
	epp->remote.port = uint16_t_be2host(hdr->src_port);
	epp->remote.addr = pdu->src;

	*seg = nseg;
	return EOK;
}

/** Encode outgoing PDU */
errno_t tcp_pdu_encode(inet_ep2_t *epp, tcp_segment_t *seg, tcp_pdu_t **pdu)
{
	tcp_pdu_t *npdu;
	size_t text_size;
	uint16_t checksum;
	errno_t rc;

	npdu = tcp_pdu_new();
	if (npdu == NULL)
		return ENOMEM;

	npdu->src = epp->local.addr;
	npdu->dest = epp->remote.addr;
	rc = tcp_header_encode(epp, seg, &npdu->header, &npdu->header_size);
	if (rc != EOK) {
		free(npdu);
		return rc;
	}

	text_size = tcp_segment_text_size(seg);
	npdu->text = calloc(1, text_size);
	if (npdu->text == NULL) {
		free(npdu->header);
		free(npdu);
		return ENOMEM;
	}

	npdu->text_size = text_size;
	memcpy(npdu->text, seg->data, text_size);

	/* Checksum calculation */
	checksum = tcp_pdu_checksum_calc(npdu);
	tcp_pdu_set_checksum(npdu, checksum);

	*pdu = npdu;
	return EOK;
}

/**
 * @}
 */
