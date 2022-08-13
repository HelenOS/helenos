/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

/** Get source address.
 *
 * @param remote Remote address
 * @param tos Type of service
 * @param local Place to store local address
 * @return EOK on success or an error code
 */
errno_t udp_get_srcaddr(inet_addr_t *remote, uint8_t tos, inet_addr_t *local)
{
	return inet_get_srcaddr(remote, tos, local);
}

/** Transmit message over network layer. */
errno_t udp_transmit_msg(inet_ep2_t *epp, udp_msg_t *msg)
{
	udp_pdu_t *pdu;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_transmit_msg()");

	rc = udp_pdu_encode(epp, msg, &pdu);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed encoding PDU");
		return rc;
	}

	rc = udp_transmit_pdu(pdu);
	udp_pdu_delete(pdu);

	return rc;
}

/**
 * @}
 */
