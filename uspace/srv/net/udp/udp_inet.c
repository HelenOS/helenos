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
 * @file
 */

#include <errno.h>
#include <inet/inet.h>
#include <io/log.h>

#include "assoc.h"
#include "pdu.h"
#include "std.h"
#include "udp_inet.h"
#include "udp_type.h"

static errno_t udp_inet_ev_recv(inet_dgram_t *dgram);
static void udp_received_pdu(udp_pdu_t *pdu);

static inet_ev_ops_t udp_inet_ev_ops = {
	.recv = udp_inet_ev_recv
};

/** Received datagram callback */
static errno_t udp_inet_ev_recv(inet_dgram_t *dgram)
{
	udp_pdu_t *pdu;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_inet_ev_recv()");

	pdu = udp_pdu_new();
	pdu->iplink = dgram->iplink;
	pdu->data = dgram->data;
	pdu->data_size = dgram->size;

	pdu->src = dgram->src;
	pdu->dest = dgram->dest;

	udp_received_pdu(pdu);

	/* We don't want udp_pdu_delete() to free dgram->data */
	pdu->data = NULL;
	udp_pdu_delete(pdu);

	return EOK;
}

/** Transmit PDU over network layer. */
errno_t udp_transmit_pdu(udp_pdu_t *pdu)
{
	errno_t rc;
	inet_dgram_t dgram;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_transmit_pdu()");

	dgram.iplink = pdu->iplink;
	dgram.src = pdu->src;
	dgram.dest = pdu->dest;
	dgram.tos = 0;
	dgram.data = pdu->data;
	dgram.size = pdu->data_size;

	rc = inet_send(&dgram, INET_TTL_MAX, 0);
	if (rc != EOK)
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed to transmit PDU.");

	return rc;
}

/** Process received PDU. */
static void udp_received_pdu(udp_pdu_t *pdu)
{
	udp_msg_t *dmsg;
	inet_ep2_t rident;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_received_pdu()");

	if (udp_pdu_decode(pdu, &rident, &dmsg) != EOK) {
		log_msg(LOG_DEFAULT, LVL_WARN, "Not enough memory. PDU dropped.");
		return;
	}

	/*
	 * Insert decoded message into appropriate receive queue.
	 * This transfers ownership of dmsg to the callee, we do not
	 * free it.
	 */
	udp_assoc_received(&rident, dmsg);
}

errno_t udp_inet_init(void)
{
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_inet_init()");

	rc = inet_init(IP_PROTO_UDP, &udp_inet_ev_ops);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed connecting to internet service.");
		return ENOENT;
	}

	return EOK;
}

/**
 * @}
 */
