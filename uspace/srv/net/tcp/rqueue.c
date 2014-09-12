/*
 * Copyright (c) 2011 Jiri Svoboda
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
 * @file Global segment receive queue
 */

#include <adt/prodcons.h>
#include <errno.h>
#include <io/log.h>
#include <stdlib.h>
#include <fibril.h>
#include "conn.h"
#include "pdu.h"
#include "rqueue.h"
#include "segment.h"
#include "tcp_type.h"
#include "ucall.h"

/** Transcode bounced segments.
 *
 * If defined, segments bounced via the internal debugging loopback will
 * be encoded to a PDU and the decoded. Otherwise they will be bounced back
 * directly without passing the encoder-decoder.
 */
#define BOUNCE_TRANSCODE

static prodcons_t rqueue;

/** Initialize segment receive queue. */
void tcp_rqueue_init(void)
{
	prodcons_initialize(&rqueue);
}

/** Bounce segment directy into receive queue without constructing the PDU.
 *
 * This is for testing purposes only.
 *
 * @param sp	Socket pair, oriented for transmission
 * @param seg	Segment
 */
void tcp_rqueue_bounce_seg(tcp_sockpair_t *sp, tcp_segment_t *seg)
{
	tcp_sockpair_t rident;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_rqueue_bounce_seg()");

#ifdef BOUNCE_TRANSCODE
	tcp_pdu_t *pdu;
	tcp_segment_t *dseg;

	if (tcp_pdu_encode(sp, seg, &pdu) != EOK) {
		log_msg(LOG_DEFAULT, LVL_WARN, "Not enough memory. Segment dropped.");
		return;
	}

	if (tcp_pdu_decode(pdu, &rident, &dseg) != EOK) {
		log_msg(LOG_DEFAULT, LVL_WARN, "Not enough memory. Segment dropped.");
		return;
	}

	tcp_pdu_delete(pdu);

	/** Insert decoded segment into rqueue */
	tcp_rqueue_insert_seg(&rident, dseg);
	tcp_segment_delete(seg);
#else
	/* Reverse the identification */
	tcp_sockpair_flipped(sp, &rident);

	/* Insert segment back into rqueue */
	tcp_rqueue_insert_seg(&rident, seg);
#endif
}

/** Insert segment into receive queue.
 *
 * @param sp	Socket pair, oriented for reception
 * @param seg	Segment
 */
void tcp_rqueue_insert_seg(tcp_sockpair_t *sp, tcp_segment_t *seg)
{
	tcp_rqueue_entry_t *rqe;
	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_rqueue_insert_seg()");

	tcp_segment_dump(seg);

	rqe = calloc(1, sizeof(tcp_rqueue_entry_t));
	if (rqe == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed allocating RQE.");
		return;
	}

	rqe->sp = *sp;
	rqe->seg = seg;

	prodcons_produce(&rqueue, &rqe->link);
}

/** Receive queue handler fibril. */
static int tcp_rqueue_fibril(void *arg)
{
	link_t *link;
	tcp_rqueue_entry_t *rqe;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_rqueue_fibril()");

	while (true) {
		link = prodcons_consume(&rqueue);
		rqe = list_get_instance(link, tcp_rqueue_entry_t, link);

		tcp_as_segment_arrived(&rqe->sp, rqe->seg);
		free(rqe);
	}

	/* Not reached */
	return 0;
}

/** Start receive queue handler fibril. */
void tcp_rqueue_fibril_start(void)
{
	fid_t fid;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_rqueue_fibril_start()");

	fid = fibril_create(tcp_rqueue_fibril, NULL);
	if (fid == 0) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed creating rqueue fibril.");
		return;
	}

	fibril_add_ready(fid);
}

/**
 * @}
 */
