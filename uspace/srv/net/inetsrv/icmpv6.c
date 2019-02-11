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
#include <mem.h>
#include <stdlib.h>
#include <types/inetping.h>
#include "icmpv6.h"
#include "icmpv6_std.h"
#include "inetsrv.h"
#include "inetping.h"
#include "pdu.h"

static errno_t icmpv6_recv_echo_request(inet_dgram_t *dgram)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "icmpv6_recv_echo_request()");

	if (dgram->size < sizeof(icmpv6_message_t))
		return EINVAL;

	icmpv6_message_t *request = (icmpv6_message_t *) dgram->data;
	size_t size = dgram->size;

	addr128_t src_v6;
	ip_ver_t src_ver = inet_addr_get(&dgram->src, NULL, &src_v6);

	addr128_t dest_v6;
	ip_ver_t dest_ver = inet_addr_get(&dgram->dest, NULL, &dest_v6);

	if ((src_ver != dest_ver) || (src_ver != ip_v6))
		return EINVAL;

	icmpv6_message_t *reply = calloc(1, size);
	if (reply == NULL)
		return ENOMEM;

	memcpy(reply, request, size);

	reply->type = ICMPV6_ECHO_REPLY;
	reply->code = 0;
	reply->checksum = 0;

	inet_dgram_t rdgram;

	inet_get_srcaddr(&dgram->src, 0, &rdgram.src);
	rdgram.dest = dgram->src;
	rdgram.iplink = 0;
	rdgram.tos = 0;
	rdgram.data = reply;
	rdgram.size = size;

	icmpv6_phdr_t phdr;

	host2addr128_t_be(dest_v6, phdr.src_addr);
	host2addr128_t_be(src_v6, phdr.dest_addr);
	phdr.length = host2uint32_t_be(dgram->size);
	memset(phdr.zeroes, 0, 3);
	phdr.next = IP_PROTO_ICMPV6;

	uint16_t cs_phdr =
	    inet_checksum_calc(INET_CHECKSUM_INIT, &phdr,
	    sizeof(icmpv6_phdr_t));

	uint16_t cs_all = inet_checksum_calc(cs_phdr, reply, size);

	reply->checksum = host2uint16_t_be(cs_all);

	errno_t rc = inet_route_packet(&rdgram, IP_PROTO_ICMPV6,
	    INET6_HOP_LIMIT_MAX, 0);

	free(reply);

	return rc;
}

static errno_t icmpv6_recv_echo_reply(inet_dgram_t *dgram)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "icmpv6_recv_echo_reply()");

	if (dgram->size < sizeof(icmpv6_message_t))
		return EINVAL;

	inetping_sdu_t sdu;

	sdu.src = dgram->src;
	sdu.dest = dgram->dest;

	icmpv6_message_t *reply = (icmpv6_message_t *) dgram->data;

	sdu.seq_no = uint16_t_be2host(reply->un.echo.seq_no);
	sdu.data = dgram->data + sizeof(icmpv6_message_t);
	sdu.size = dgram->size - sizeof(icmpv6_message_t);

	uint16_t ident = uint16_t_be2host(reply->un.echo.ident);

	return inetping_recv(ident, &sdu);
}

errno_t icmpv6_recv(inet_dgram_t *dgram)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "icmpv6_recv()");

	if (dgram->size < 1)
		return EINVAL;

	uint8_t type = *(uint8_t *) dgram->data;

	switch (type) {
	case ICMPV6_ECHO_REQUEST:
		return icmpv6_recv_echo_request(dgram);
	case ICMPV6_ECHO_REPLY:
		return icmpv6_recv_echo_reply(dgram);
	case ICMPV6_NEIGHBOUR_SOLICITATION:
	case ICMPV6_NEIGHBOUR_ADVERTISEMENT:
	case ICMPV6_ROUTER_ADVERTISEMENT:
		return ndp_received(dgram);
	default:
		break;
	}

	return EINVAL;
}

errno_t icmpv6_ping_send(uint16_t ident, inetping_sdu_t *sdu)
{
	size_t rsize = sizeof(icmpv6_message_t) + sdu->size;
	void *rdata = calloc(1, rsize);
	if (rdata == NULL)
		return ENOMEM;

	icmpv6_message_t *request = (icmpv6_message_t *) rdata;

	request->type = ICMPV6_ECHO_REQUEST;
	request->code = 0;
	request->checksum = 0;
	request->un.echo.ident = host2uint16_t_be(ident);
	request->un.echo.seq_no = host2uint16_t_be(sdu->seq_no);

	memcpy(rdata + sizeof(icmpv6_message_t), sdu->data, sdu->size);

	inet_dgram_t dgram;

	dgram.src = sdu->src;
	dgram.dest = sdu->dest;
	dgram.iplink = 0;
	dgram.tos = 0;
	dgram.data = rdata;
	dgram.size = rsize;

	icmpv6_phdr_t phdr;

	assert(sdu->src.version == ip_v6);
	assert(sdu->dest.version == ip_v6);

	host2addr128_t_be(sdu->src.addr6, phdr.src_addr);
	host2addr128_t_be(sdu->dest.addr6, phdr.dest_addr);
	phdr.length = host2uint32_t_be(dgram.size);
	memset(phdr.zeroes, 0, 3);
	phdr.next = IP_PROTO_ICMPV6;

	uint16_t cs_phdr =
	    inet_checksum_calc(INET_CHECKSUM_INIT, &phdr,
	    sizeof(icmpv6_phdr_t));

	uint16_t cs_all = inet_checksum_calc(cs_phdr, rdata, rsize);

	request->checksum = host2uint16_t_be(cs_all);

	errno_t rc = inet_route_packet(&dgram, IP_PROTO_ICMPV6,
	    INET6_HOP_LIMIT_MAX, 0);

	free(rdata);

	return rc;
}

/** @}
 */
