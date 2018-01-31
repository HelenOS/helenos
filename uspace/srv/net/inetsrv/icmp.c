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
#include <mem.h>
#include <stdlib.h>
#include <types/inetping.h>
#include "icmp.h"
#include "icmp_std.h"
#include "inetsrv.h"
#include "inetping.h"
#include "pdu.h"

/* XXX */
#define INET_TTL_MAX 255

static errno_t icmp_recv_echo_request(inet_dgram_t *);
static errno_t icmp_recv_echo_reply(inet_dgram_t *);

errno_t icmp_recv(inet_dgram_t *dgram)
{
	uint8_t type;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "icmp_recv()");

	if (dgram->size < 1)
		return EINVAL;

	type = *(uint8_t *)dgram->data;

	switch (type) {
	case ICMP_ECHO_REQUEST:
		return icmp_recv_echo_request(dgram);
	case ICMP_ECHO_REPLY:
		return icmp_recv_echo_reply(dgram);
	default:
		break;
	}

	return EINVAL;
}

static errno_t icmp_recv_echo_request(inet_dgram_t *dgram)
{
	icmp_echo_t *request, *reply;
	uint16_t checksum;
	size_t size;
	inet_dgram_t rdgram;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "icmp_recv_echo_request()");

	if (dgram->size < sizeof(icmp_echo_t))
		return EINVAL;

	request = (icmp_echo_t *)dgram->data;
	size = dgram->size;

	reply = calloc(size, 1);
	if (reply == NULL)
		return ENOMEM;

	memcpy(reply, request, size);

	reply->type = ICMP_ECHO_REPLY;
	reply->code = 0;
	reply->checksum = 0;

	checksum = inet_checksum_calc(INET_CHECKSUM_INIT, reply, size);
	reply->checksum = host2uint16_t_be(checksum);

	rdgram.iplink = 0;
	rdgram.src = dgram->dest;
	rdgram.dest = dgram->src;
	rdgram.tos = ICMP_TOS;
	rdgram.data = reply;
	rdgram.size = size;

	rc = inet_route_packet(&rdgram, IP_PROTO_ICMP, INET_TTL_MAX, 0);

	free(reply);

	return rc;
}

static errno_t icmp_recv_echo_reply(inet_dgram_t *dgram)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "icmp_recv_echo_reply()");

	if (dgram->size < sizeof(icmp_echo_t))
		return EINVAL;

	icmp_echo_t *reply = (icmp_echo_t *) dgram->data;

	inetping_sdu_t sdu;

	sdu.src = dgram->src;
	sdu.dest = dgram->dest;
	sdu.seq_no = uint16_t_be2host(reply->seq_no);
	sdu.data = reply + sizeof(icmp_echo_t);
	sdu.size = dgram->size - sizeof(icmp_echo_t);

	uint16_t ident = uint16_t_be2host(reply->ident);

	return inetping_recv(ident, &sdu);
}

errno_t icmp_ping_send(uint16_t ident, inetping_sdu_t *sdu)
{
	size_t rsize = sizeof(icmp_echo_t) + sdu->size;
	void *rdata = calloc(rsize, 1);
	if (rdata == NULL)
		return ENOMEM;

	icmp_echo_t *request = (icmp_echo_t *) rdata;

	request->type = ICMP_ECHO_REQUEST;
	request->code = 0;
	request->checksum = 0;
	request->ident = host2uint16_t_be(ident);
	request->seq_no = host2uint16_t_be(sdu->seq_no);

	memcpy(rdata + sizeof(icmp_echo_t), sdu->data, sdu->size);

	uint16_t checksum = inet_checksum_calc(INET_CHECKSUM_INIT, rdata, rsize);
	request->checksum = host2uint16_t_be(checksum);

	inet_dgram_t dgram;

	dgram.src = sdu->src;
	dgram.dest = sdu->dest;
	dgram.iplink = 0;
	dgram.tos = ICMP_TOS;
	dgram.data = rdata;
	dgram.size = rsize;

	errno_t rc = inet_route_packet(&dgram, IP_PROTO_ICMP, INET_TTL_MAX, 0);

	free(rdata);
	return rc;
}

/** @}
 */
