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
 * @file TCP transmission queue
 */

#include <adt/list.h>
#include <errno.h>
#include <fibril_synch.h>
#include <byteorder.h>
#include <io/log.h>
#include <macros.h>
#include <mem.h>
#include <stdlib.h>
#include "conn.h"
#include "header.h"
#include "ncsim.h"
#include "rqueue.h"
#include "segment.h"
#include "seq_no.h"
#include "tqueue.h"
#include "tcp_type.h"

#define RETRANSMIT_TIMEOUT	(2*1000*1000)

static void retransmit_timeout_func(void *arg);
static void tcp_tqueue_timer_set(tcp_conn_t *conn);
static void tcp_tqueue_timer_clear(tcp_conn_t *conn);

int tcp_tqueue_init(tcp_tqueue_t *tqueue, tcp_conn_t *conn)
{
	tqueue->conn = conn;
	tqueue->timer = fibril_timer_create();
	if (tqueue->timer == NULL)
		return ENOMEM;

	list_initialize(&tqueue->list);

	return EOK;
}

void tcp_tqueue_fini(tcp_tqueue_t *tqueue)
{
	if (tqueue->timer != NULL) {
		fibril_timer_destroy(tqueue->timer);
		tqueue->timer = NULL;
	}
}

void tcp_tqueue_ctrl_seg(tcp_conn_t *conn, tcp_control_t ctrl)
{
	tcp_segment_t *seg;

	log_msg(LVL_DEBUG, "tcp_tqueue_ctrl_seg(%p, %u)", conn, ctrl);

	seg = tcp_segment_make_ctrl(ctrl);
	tcp_tqueue_seg(conn, seg);
}

void tcp_tqueue_seg(tcp_conn_t *conn, tcp_segment_t *seg)
{
	tcp_segment_t *rt_seg;
	tcp_tqueue_entry_t *tqe;

	log_msg(LVL_DEBUG, "tcp_tqueue_seg(%p, %p)", conn, seg);

	/*
	 * Add segment to retransmission queue
	 */

	if (seg->len > 0) {
		rt_seg = tcp_segment_dup(seg);
		if (rt_seg == NULL) {
			log_msg(LVL_ERROR, "Memory allocation failed.");
			/* XXX Handle properly */
			return;
		}

		tqe = calloc(1, sizeof(tcp_tqueue_entry_t));
		if (tqe == NULL) {
			log_msg(LVL_ERROR, "Memory allocation failed.");
			/* XXX Handle properly */
			return;
		}

		tqe->conn = conn;
		tqe->seg = rt_seg;
		list_append(&tqe->link, &conn->retransmit.list);

		/* Set retransmission timer */
		tcp_tqueue_timer_set(conn);
	}

	tcp_prepare_transmit_segment(conn, seg);
}

void tcp_prepare_transmit_segment(tcp_conn_t *conn, tcp_segment_t *seg)
{
	/*
	 * Always send ACK once we have received SYN, except for RST segments.
	 * (Spec says we should always send ACK once connection has been
	 * established.)
	 */
	if (tcp_conn_got_syn(conn) && (seg->ctrl & CTL_RST) == 0)
		seg->ctrl |= CTL_ACK;

	seg->seq = conn->snd_nxt;
	conn->snd_nxt += seg->len;

	tcp_transmit_segment(&conn->ident, seg);
}

/** Transmit data from the send buffer.
 *
 * @param conn	Connection
 */
void tcp_tqueue_new_data(tcp_conn_t *conn)
{
	size_t avail_wnd;
	size_t xfer_seqlen;
	size_t snd_buf_seqlen;
	size_t data_size;
	tcp_control_t ctrl;

	tcp_segment_t *seg;

	log_msg(LVL_DEBUG, "tcp_tqueue_new_data()");

	/* Number of free sequence numbers in send window */
	avail_wnd = (conn->snd_una + conn->snd_wnd) - conn->snd_nxt;
	snd_buf_seqlen = conn->snd_buf_used + (conn->snd_buf_fin ? 1 : 0);

	xfer_seqlen = min(snd_buf_seqlen, avail_wnd);
	log_msg(LVL_DEBUG, "snd_buf_seqlen = %zu, SND.WND = %zu, "
	    "xfer_seqlen = %zu", snd_buf_seqlen, conn->snd_wnd,
	    xfer_seqlen);

	if (xfer_seqlen == 0)
		return;

	/* XXX Do not always send immediately */

	data_size = xfer_seqlen - (conn->snd_buf_fin ? 1 : 0);
	if (conn->snd_buf_fin && data_size + 1 == xfer_seqlen) {
		/* We are sending out FIN */
		ctrl = CTL_FIN;
		tcp_conn_fin_sent(conn);
	} else {
		ctrl = 0;
	}

	seg = tcp_segment_make_data(ctrl, conn->snd_buf, data_size);
	if (seg == NULL) {
		log_msg(LVL_ERROR, "Memory allocation failure.");
		return;
	}

	/* Remove data from send buffer */
	memmove(conn->snd_buf, conn->snd_buf + data_size,
	    conn->snd_buf_used - data_size);
	conn->snd_buf_used -= data_size;
	conn->snd_buf_fin = false;

	tcp_tqueue_seg(conn, seg);
}

/** Remove ACKed segments from retransmission queue and possibly transmit
 * more data.
 *
 * This should be called when SND.UNA is updated due to incoming ACK.
 */
void tcp_tqueue_ack_received(tcp_conn_t *conn)
{
	link_t *cur, *next;

	log_msg(LVL_DEBUG, "tcp_tqueue_ack_received(%p)", conn);

	cur = conn->retransmit.list.head.next;

	while (cur != &conn->retransmit.list.head) {
		next = cur->next;

		tcp_tqueue_entry_t *tqe = list_get_instance(cur,
		    tcp_tqueue_entry_t, link);

		if (seq_no_segment_acked(conn, tqe->seg, conn->snd_una)) {
			/* Remove acknowledged segment */
			list_remove(cur);

			if ((tqe->seg->ctrl & CTL_FIN) != 0) {
				/* Our FIN has been acked */
				conn->fin_is_acked = true;
			}

			tcp_segment_delete(tqe->seg);
			free(tqe);

			/* Reset retransmission timer */
			tcp_tqueue_timer_set(conn);
		}

		cur = next;
	}

	/* Clear retransmission timer if the queue is empty. */
	if (list_empty(&conn->retransmit.list))
		tcp_tqueue_timer_clear(conn);

	/* Possibly transmit more data */
	tcp_tqueue_new_data(conn);
}

void tcp_conn_transmit_segment(tcp_conn_t *conn, tcp_segment_t *seg)
{
	log_msg(LVL_DEBUG, "tcp_conn_transmit_segment(%p, %p)", conn, seg);

	seg->wnd = conn->rcv_wnd;

	if ((seg->ctrl & CTL_ACK) != 0)
		seg->ack = conn->rcv_nxt;
	else
		seg->ack = 0;

	tcp_transmit_segment(&conn->ident, seg);
}

void tcp_transmit_segment(tcp_sockpair_t *sp, tcp_segment_t *seg)
{
	log_msg(LVL_DEBUG, "tcp_transmit_segment(%p, %p)", sp, seg);

	log_msg(LVL_DEBUG, "SEG.SEQ=%" PRIu32 ", SEG.WND=%" PRIu32,
	    seg->seq, seg->wnd);
/*
	tcp_pdu_prepare(conn, seg, &data, &len);
	tcp_pdu_transmit(data, len);
*/
	tcp_rqueue_bounce_seg(sp, seg);
//	tcp_ncsim_bounce_seg(sp, seg);
}

static void retransmit_timeout_func(void *arg)
{
	tcp_conn_t *conn = (tcp_conn_t *) arg;
	tcp_tqueue_entry_t *tqe;
	tcp_segment_t *rt_seg;
	link_t *link;

	log_msg(LVL_DEBUG, "### %s: retransmit_timeout_func(%p)", conn->name, conn);
	link = list_first(&conn->retransmit.list);
	if (link == NULL) {
		log_msg(LVL_DEBUG, "Nothing to retransmit");
		return;
	}

	tqe = list_get_instance(link, tcp_tqueue_entry_t, link);

	rt_seg = tcp_segment_dup(tqe->seg);
	if (rt_seg == NULL) {
		log_msg(LVL_ERROR, "Memory allocation failed.");
		/* XXX Handle properly */
		return;
	}

	log_msg(LVL_DEBUG, "### %s: retransmitting segment", conn->name);
	tcp_conn_transmit_segment(tqe->conn, rt_seg);

	/* Reset retransmission timer */
	tcp_tqueue_timer_set(tqe->conn);
}

/** Set or re-set retransmission timer */
static void tcp_tqueue_timer_set(tcp_conn_t *conn)
{
	log_msg(LVL_DEBUG, "### %s: tcp_tqueue_timer_set()", conn->name);

	(void) retransmit_timeout_func;
	fibril_timer_set(conn->retransmit.timer, RETRANSMIT_TIMEOUT,
	    retransmit_timeout_func, (void *) conn);
}

/** Clear retransmission timer */
static void tcp_tqueue_timer_clear(tcp_conn_t *conn)
{
	log_msg(LVL_DEBUG, "### %s: tcp_tqueue_timer_clear()", conn->name);

	fibril_timer_clear(conn->retransmit.timer);
}

/**
 * @}
 */
