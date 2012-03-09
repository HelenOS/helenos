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

#include "icmp.h"
#include "icmp_std.h"
#include "inet.h"
#include "pdu.h"

/* XXX */
#define INET_TTL_MAX 255

static int icmp_echo_request(inet_dgram_t *);

int icmp_recv(inet_dgram_t *dgram)
{
	uint8_t type;

	log_msg(LVL_DEBUG, "icmp_recv()");

	if (dgram->size < 1)
		return EINVAL;

	type = *(uint8_t *)dgram->data;

	switch (type) {
	case ICMP_ECHO_REQUEST:
		return icmp_echo_request(dgram);
	default:
		break;
	}

	return EINVAL;
}

static int icmp_echo_request(inet_dgram_t *dgram)
{
	icmp_echo_t *request, *reply;
	uint16_t checksum;
	size_t size;
	inet_dgram_t rdgram;
	int rc;

	log_msg(LVL_DEBUG, "icmp_echo_request()");

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

	rdgram.src = dgram->dest;
	rdgram.dest = dgram->src;
	rdgram.tos = 0;
	rdgram.data = reply;
	rdgram.size = size;

	rc = inet_route_packet(&rdgram, IP_PROTO_ICMP, INET_TTL_MAX, 0);

	free(reply);

	return rc;
}

/** @}
 */
