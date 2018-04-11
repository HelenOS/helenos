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

/** @addtogroup udp
 * @{
 */

/**
 * @file UDP PDU encoding and decoding
 */

#include <bitops.h>
#include <byteorder.h>
#include <errno.h>
#include <mem.h>
#include <stdlib.h>
#include <inet/addr.h>
#include "msg.h"
#include "pdu.h"
#include "std.h"
#include "udp_type.h"

#define UDP_CHECKSUM_INIT 0xffff

/** One's complement addition.
 *
 * Result is a + b + carry.
 */
static uint16_t udp_ocadd16(uint16_t a, uint16_t b)
{
	uint32_t s;

	s = (uint32_t)a + (uint32_t)b;
	return (s & 0xffff) + (s >> 16);
}

static uint16_t udp_checksum_calc(uint16_t ivalue, void *data, size_t size)
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
		sum = udp_ocadd16(sum, w);
	}

	if (size % 2 != 0) {
		w = ((uint16_t)bdata[2 * words] << 8);
		sum = udp_ocadd16(sum, w);
	}

	return ~sum;
}

static ip_ver_t udp_phdr_setup(udp_pdu_t *pdu, udp_phdr_t *phdr,
    udp_phdr6_t *phdr6)
{
	addr32_t src_v4;
	addr128_t src_v6;
	ip_ver_t src_ver = inet_addr_get(&pdu->src, &src_v4, &src_v6);

	addr32_t dest_v4;
	addr128_t dest_v6;
	ip_ver_t dest_ver = inet_addr_get(&pdu->dest, &dest_v4, &dest_v6);

	assert(src_ver == dest_ver);

	switch (src_ver) {
	case ip_v4:
		phdr->src_addr = host2uint32_t_be(src_v4);
		phdr->dest_addr = host2uint32_t_be(dest_v4);
		phdr->zero = 0;
		phdr->protocol = IP_PROTO_UDP;
		phdr->udp_length = host2uint16_t_be(pdu->data_size);
		break;
	case ip_v6:
		host2addr128_t_be(src_v6, phdr6->src_addr);
		host2addr128_t_be(dest_v6, phdr6->dest_addr);
		phdr6->udp_length = host2uint32_t_be(pdu->data_size);
		memset(phdr6->zeroes, 0, 3);
		phdr6->next = IP_PROTO_UDP;
		break;
	default:
		assert(false);
	}

	return src_ver;
}

udp_pdu_t *udp_pdu_new(void)
{
	return calloc(1, sizeof(udp_pdu_t));
}

void udp_pdu_delete(udp_pdu_t *pdu)
{
	free(pdu->data);
	free(pdu);
}

static uint16_t udp_pdu_checksum_calc(udp_pdu_t *pdu)
{
	uint16_t cs_phdr;
	udp_phdr_t phdr;
	udp_phdr6_t phdr6;

	ip_ver_t ver = udp_phdr_setup(pdu, &phdr, &phdr6);
	switch (ver) {
	case ip_v4:
		cs_phdr = udp_checksum_calc(UDP_CHECKSUM_INIT, (void *) &phdr,
		    sizeof(udp_phdr_t));
		break;
	case ip_v6:
		cs_phdr = udp_checksum_calc(UDP_CHECKSUM_INIT, (void *) &phdr6,
		    sizeof(udp_phdr6_t));
		break;
	default:
		assert(false);
	}

	return udp_checksum_calc(cs_phdr, pdu->data, pdu->data_size);
}

static void udp_pdu_set_checksum(udp_pdu_t *pdu, uint16_t checksum)
{
	udp_header_t *hdr;

	hdr = (udp_header_t *)pdu->data;
	hdr->checksum = host2uint16_t_be(checksum);
}

/** Decode incoming PDU */
errno_t udp_pdu_decode(udp_pdu_t *pdu, inet_ep2_t *epp, udp_msg_t **msg)
{
	udp_msg_t *nmsg;
	udp_header_t *hdr;
	void *text;
	size_t text_size;
	uint16_t length;
	uint16_t checksum;

	if (pdu->data_size < sizeof(udp_header_t))
		return EINVAL;

	text = pdu->data + sizeof(udp_header_t);
	text_size = pdu->data_size - sizeof(udp_header_t);

	hdr = (udp_header_t *)pdu->data;

	epp->local_link = pdu->iplink;
	epp->remote.port = uint16_t_be2host(hdr->src_port);
	epp->remote.addr = pdu->src;
	epp->local.port = uint16_t_be2host(hdr->dest_port);
	epp->local.addr = pdu->dest;

	length = uint16_t_be2host(hdr->length);
	checksum = uint16_t_be2host(hdr->checksum);
	(void) checksum;

	if (length < sizeof(udp_header_t) ||
	    length > sizeof(udp_header_t) + text_size)
		return EINVAL;

	nmsg = udp_msg_new();
	if (nmsg == NULL)
		return ENOMEM;

	nmsg->data_size = length - sizeof(udp_header_t);
	nmsg->data = malloc(nmsg->data_size);
	if (nmsg->data == NULL)
		return ENOMEM;

	memcpy(nmsg->data, text, nmsg->data_size);

	*msg = nmsg;
	return EOK;
}

/** Encode outgoing PDU */
errno_t udp_pdu_encode(inet_ep2_t *epp, udp_msg_t *msg, udp_pdu_t **pdu)
{
	udp_pdu_t *npdu;
	udp_header_t *hdr;
	uint16_t checksum;

	npdu = udp_pdu_new();
	if (npdu == NULL)
		return ENOMEM;

	npdu->iplink = epp->local_link;
	npdu->src = epp->local.addr;
	npdu->dest = epp->remote.addr;

	npdu->data_size = sizeof(udp_header_t) + msg->data_size;
	npdu->data = calloc(1, npdu->data_size);
	if (npdu->data == NULL) {
		udp_pdu_delete(npdu);
		return ENOMEM;
	}

	hdr = (udp_header_t *)npdu->data;
	hdr->src_port = host2uint16_t_be(epp->local.port);
	hdr->dest_port = host2uint16_t_be(epp->remote.port);
	hdr->length = host2uint16_t_be(npdu->data_size);
	hdr->checksum = 0;

	memcpy((uint8_t *)npdu->data + sizeof(udp_header_t), msg->data,
	    msg->data_size);

	/* Checksum calculation */
	checksum = udp_pdu_checksum_calc(npdu);
	udp_pdu_set_checksum(npdu, checksum);

	*pdu = npdu;
	return EOK;
}

/**
 * @}
 */
