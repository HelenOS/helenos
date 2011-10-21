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
 * @file TCP connection processing and state machine
 */

#include <adt/list.h>
#include <bool.h>
#include <errno.h>
#include <io/log.h>
#include <macros.h>
#include <stdlib.h>
#include "conn.h"
#include "iqueue.h"
#include "segment.h"
#include "seq_no.h"
#include "tcp_type.h"
#include "tqueue.h"

#define RCV_BUF_SIZE 4096
#define SND_BUF_SIZE 4096

LIST_INITIALIZE(conn_list);

static void tcp_conn_seg_process(tcp_conn_t *conn, tcp_segment_t *seg);

/** Create new segment structure.
 *
 * @param lsock		Local socket (will be deeply copied)
 * @param fsock		Foreign socket (will be deeply copied)
 * @return		New segment or NULL
 */
tcp_conn_t *tcp_conn_new(tcp_sock_t *lsock, tcp_sock_t *fsock)
{
	tcp_conn_t *conn;

	/* Allocate connection structure */
	conn = calloc(1, sizeof(tcp_conn_t));
	if (conn == NULL)
		return NULL;

	/* Allocate receive buffer */
	fibril_mutex_initialize(&conn->rcv_buf_lock);
	fibril_condvar_initialize(&conn->rcv_buf_cv);
	conn->rcv_buf_size = RCV_BUF_SIZE;
	conn->rcv_buf_used = 0;
	conn->rcv_buf_fin = false;

	conn->rcv_buf = calloc(1, conn->rcv_buf_size);
	if (conn->rcv_buf == NULL) {
		free(conn);
		return NULL;
	}

	/** Allocate send buffer */
	conn->snd_buf_size = SND_BUF_SIZE;
	conn->snd_buf_used = 0;
	conn->snd_buf_fin = false;
	conn->snd_buf = calloc(1, conn->snd_buf_size);
	if (conn->snd_buf == NULL) {
		free(conn);
		return NULL;
	}

	/* Set up receive window. */
	conn->rcv_wnd = conn->rcv_buf_size;

	/* Initialize incoming segment queue */
	tcp_iqueue_init(&conn->incoming, conn);

	conn->cstate = st_listen;
	conn->ident.local = *lsock;
	if (fsock != NULL)
		conn->ident.foreign = *fsock;

	return conn;
}

/** Enlist connection.
 *
 * Add connection to the connection map.
 */
void tcp_conn_add(tcp_conn_t *conn)
{
	list_append(&conn->link, &conn_list);
}

/** Synchronize connection.
 *
 * This is the first step of an active connection attempt,
 * sends out SYN and sets up ISS and SND.xxx.
 */
void tcp_conn_sync(tcp_conn_t *conn)
{
	/* XXX select ISS */
	conn->iss = 1;
	conn->snd_nxt = conn->iss;
	conn->snd_una = conn->iss;

	tcp_tqueue_ctrl_seg(conn, CTL_SYN);
	conn->cstate = st_syn_sent;
}

/** Compare two sockets.
 *
 * Two sockets are equal if the address is equal and the port number
 * is equal.
 */
static bool tcp_socket_equal(tcp_sock_t *a, tcp_sock_t *b)
{
	log_msg(LVL_DEBUG, "tcp_socket_equal((%x,%u), (%x,%u))",
	    a->addr.ipv4, a->port, b->addr.ipv4, b->port);

	if (a->addr.ipv4 != b->addr.ipv4)
		return false;

	if (a->port != b->port)
		return false;

	log_msg(LVL_DEBUG, " -> match");

	return true;
}

/** Match socket with pattern. */
static bool tcp_sockpair_match(tcp_sockpair_t *sp, tcp_sockpair_t *pattern)
{
	log_msg(LVL_DEBUG, "tcp_sockpair_match(%p, %p)", sp, pattern);

	if (!tcp_socket_equal(&sp->local, &pattern->local))
		return false;

	if (!tcp_socket_equal(&sp->foreign, &pattern->foreign))
		return false;

	return true;
}

/** Find connection structure for specified socket pair.
 *
 * A connection is uniquely identified by a socket pair. Look up our
 * connection map and return connection structure based on socket pair.
 *
 * @param sp	Socket pair
 * @return	Connection structure or NULL if not found.
 */
tcp_conn_t *tcp_conn_find(tcp_sockpair_t *sp)
{
	log_msg(LVL_DEBUG, "tcp_conn_find(%p)", sp);

	list_foreach(conn_list, link) {
		tcp_conn_t *conn = list_get_instance(link, tcp_conn_t, link);
		if (tcp_sockpair_match(sp, &conn->ident)) {
			return conn;
		}
	}

	return NULL;
}

/** Determine if SYN has been received.
 *
 * @param conn	Connection
 * @return	@c true if SYN has been received, @c false otherwise.
 */
bool tcp_conn_got_syn(tcp_conn_t *conn)
{
	switch (conn->cstate) {
	case st_listen:
	case st_syn_sent:
		return false;
	case st_syn_received:
	case st_established:
	case st_fin_wait_1:
	case st_fin_wait_2:
	case st_close_wait:
	case st_closing:
	case st_last_ack:
	case st_time_wait:
		return true;
	case st_closed:
		assert(false);
	}

	assert(false);
}

/** Segment arrived in Listen state.
 *
 * @param conn		Connection
 * @param seg		Segment
 */
static void tcp_conn_sa_listen(tcp_conn_t *conn, tcp_segment_t *seg)
{
	log_msg(LVL_DEBUG, "tcp_conn_sa_listen(%p, %p)", conn, seg);

	if ((seg->ctrl & CTL_RST) != 0) {
		log_msg(LVL_DEBUG, "Ignoring incoming RST.");
		return;
	}

	if ((seg->ctrl & CTL_ACK) != 0) {
		log_msg(LVL_DEBUG, "Incoming ACK, send acceptable RST.");
		tcp_reply_rst(&conn->ident, seg);
		return;
	}

	if ((seg->ctrl & CTL_SYN) == 0) {
		log_msg(LVL_DEBUG, "SYN not present. Ignoring segment.");
		return;
	}

	log_msg(LVL_DEBUG, "Got SYN, sending SYN, ACK.");

	conn->rcv_nxt = seg->seq + 1;
	conn->irs = seg->seq;


	log_msg(LVL_DEBUG, "rcv_nxt=%u", conn->rcv_nxt);

	if (seg->len > 1)
		log_msg(LVL_WARN, "SYN combined with data, ignoring data.");

	/* XXX select ISS */
	conn->iss = 1;
	conn->snd_nxt = conn->iss;
	conn->snd_una = conn->iss;

	/*
	 * Surprisingly the spec does not deal with initial window setting.
	 * Set SND.WND = SEG.WND and set SND.WL1 so that next segment
	 * will always be accepted as new window setting.
	 */
	conn->snd_wnd = seg->wnd;
	conn->snd_wl1 = seg->seq;
	conn->snd_wl2 = seg->seq;

	conn->cstate = st_syn_received;

	tcp_tqueue_ctrl_seg(conn, CTL_SYN | CTL_ACK /* XXX */);

	tcp_segment_delete(seg);
}

/** Segment arrived in Syn-Sent state.
 *
 * @param conn		Connection
 * @param seg		Segment
 */
static void tcp_conn_sa_syn_sent(tcp_conn_t *conn, tcp_segment_t *seg)
{
	log_msg(LVL_DEBUG, "tcp_conn_sa_syn_sent(%p, %p)", conn, seg);

	if ((seg->ctrl & CTL_ACK) != 0) {
		log_msg(LVL_DEBUG, "snd_una=%u, seg.ack=%u, snd_nxt=%u",
		    conn->snd_una, seg->ack, conn->snd_nxt);
		if (!seq_no_ack_acceptable(conn, seg->ack)) {
			log_msg(LVL_WARN, "ACK not acceptable, send RST.");
			tcp_reply_rst(&conn->ident, seg);
			return;
		}
	}

	if ((seg->ctrl & CTL_RST) != 0) {
		log_msg(LVL_DEBUG, "Connection reset.");
		/* XXX Signal user error */
		conn->cstate = st_closed;
		/* XXX delete connection */
		return;
	}

	/* XXX precedence */

	if ((seg->ctrl & CTL_SYN) == 0) {
		log_msg(LVL_DEBUG, "No SYN bit, ignoring segment.");
		return;
	}

	conn->rcv_nxt = seg->seq + 1;
	conn->irs = seg->seq;

	if ((seg->ctrl & CTL_ACK) != 0) {
		conn->snd_una = seg->ack;

		/*
		 * Prune acked segments from retransmission queue and
		 * possibly transmit more data.
		 */
		tcp_tqueue_ack_received(conn);
	}

	log_msg(LVL_DEBUG, "Sent SYN, got SYN.");

	/*
	 * Surprisingly the spec does not deal with initial window setting.
	 * Set SND.WND = SEG.WND and set SND.WL1 so that next segment
	 * will always be accepted as new window setting.
	 */
	log_msg(LVL_DEBUG, "SND.WND := %" PRIu32 ", SND.WL1 := %" PRIu32 ", "
	    "SND.WL2 = %" PRIu32, seg->wnd, seg->seq, seg->seq);
	conn->snd_wnd = seg->wnd;
	conn->snd_wl1 = seg->seq;
	conn->snd_wl2 = seg->seq;

	if (seq_no_syn_acked(conn)) {
		log_msg(LVL_DEBUG, " syn acked -> Established");
		conn->cstate = st_established;
		tcp_tqueue_ctrl_seg(conn, CTL_ACK /* XXX */);
	} else {
		log_msg(LVL_DEBUG, " syn not acked -> Syn-Received");
		conn->cstate = st_syn_received;
		tcp_tqueue_ctrl_seg(conn, CTL_SYN | CTL_ACK /* XXX */);
	}

	tcp_segment_delete(seg);
}

/** Segment arrived in state where segments are processed in sequence order.
 *
 * Queue segment in incoming segments queue for processing.
 *
 * @param conn		Connection
 * @param seg		Segment
 */
static void tcp_conn_sa_queue(tcp_conn_t *conn, tcp_segment_t *seg)
{
	tcp_segment_t *pseg;

	log_msg(LVL_DEBUG, "tcp_conn_sa_seq(%p, %p)", conn, seg);

	/* XXX Discard old duplicates */

	/* Queue for processing */
	tcp_iqueue_insert_seg(&conn->incoming, seg);

	/*
	 * Process all segments from incoming queue that are ready.
	 * Unacceptable segments are discarded by tcp_iqueue_get_ready_seg().
	 *
	 * XXX Need to return ACK for unacceptable segments
	 */
	while (tcp_iqueue_get_ready_seg(&conn->incoming, &pseg) == EOK)
		tcp_conn_seg_process(conn, pseg);
}

/** Process segment RST field.
 *
 * @param conn		Connection
 * @param seg		Segment
 * @return		cp_done if we are done with this segment, cp_continue
 *			if not
 */
static cproc_t tcp_conn_seg_proc_rst(tcp_conn_t *conn, tcp_segment_t *seg)
{
	/* TODO */
	return cp_continue;
}

/** Process segment security and precedence fields.
 *
 * @param conn		Connection
 * @param seg		Segment
 * @return		cp_done if we are done with this segment, cp_continue
 *			if not
 */
static cproc_t tcp_conn_seg_proc_sp(tcp_conn_t *conn, tcp_segment_t *seg)
{
	/* TODO */
	return cp_continue;
}

/** Process segment SYN field.
 *
 * @param conn		Connection
 * @param seg		Segment
 * @return		cp_done if we are done with this segment, cp_continue
 *			if not
 */
static cproc_t tcp_conn_seg_proc_syn(tcp_conn_t *conn, tcp_segment_t *seg)
{
	/* TODO */
	return cp_continue;
}

/** Process segment ACK field in Syn-Received state.
 *
 * @param conn		Connection
 * @param seg		Segment
 * @return		cp_done if we are done with this segment, cp_continue
 *			if not
 */
static cproc_t tcp_conn_seg_proc_ack_sr(tcp_conn_t *conn, tcp_segment_t *seg)
{
	if (!seq_no_ack_acceptable(conn, seg->ack)) {
		/* ACK is not acceptable, send RST. */
		log_msg(LVL_WARN, "Segment ACK not acceptable, sending RST.");
		tcp_reply_rst(&conn->ident, seg);
		tcp_segment_delete(seg);
		return cp_done;
	}

	log_msg(LVL_DEBUG, "SYN ACKed -> Established");

	conn->cstate = st_established;

	/* XXX Not mentioned in spec?! */
	conn->snd_una = seg->ack;

	return cp_continue;
}

/** Process segment ACK field in Established state.
 *
 * @param conn		Connection
 * @param seg		Segment
 * @return		cp_done if we are done with this segment, cp_continue
 *			if not
 */
static cproc_t tcp_conn_seg_proc_ack_est(tcp_conn_t *conn, tcp_segment_t *seg)
{
	log_msg(LVL_DEBUG, "tcp_conn_seg_proc_ack_est(%p, %p)", conn, seg);

	log_msg(LVL_DEBUG, "SEG.ACK=%u, SND.UNA=%u, SND.NXT=%u",
	    (unsigned)seg->ack, (unsigned)conn->snd_una,
	    (unsigned)conn->snd_nxt);

	if (!seq_no_ack_acceptable(conn, seg->ack)) {
		log_msg(LVL_DEBUG, "ACK not acceptable.");
		if (!seq_no_ack_duplicate(conn, seg->ack)) {
			log_msg(LVL_WARN, "Not acceptable, not duplicate. "
			    "Send ACK and drop.");
			/* Not acceptable, not duplicate. Send ACK and drop. */
			tcp_tqueue_ctrl_seg(conn, CTL_ACK);
			tcp_segment_delete(seg);
			return cp_done;
		} else {
			log_msg(LVL_DEBUG, "Ignoring duplicate ACK.");
		}
	} else {
		/* Update SND.UNA */
		conn->snd_una = seg->ack;
	}

	if (seq_no_new_wnd_update(conn, seg)) {
		conn->snd_wnd = seg->wnd;
		conn->snd_wl1 = seg->seq;
		conn->snd_wl2 = seg->ack;

		log_msg(LVL_DEBUG, "Updating send window, SND.WND=%" PRIu32
		    ", SND.WL1=%" PRIu32 ", SND.WL2=%" PRIu32,
		    conn->snd_wnd, conn->snd_wl1, conn->snd_wl2);
	}

	/*
	 * Prune acked segments from retransmission queue and
	 * possibly transmit more data.
	 */
	tcp_tqueue_ack_received(conn);

	return cp_continue;
}

/** Process segment ACK field in Fin-Wait-1 state.
 *
 * @param conn		Connection
 * @param seg		Segment
 * @return		cp_done if we are done with this segment, cp_continue
 *			if not
 */
static cproc_t tcp_conn_seg_proc_ack_fw1(tcp_conn_t *conn, tcp_segment_t *seg)
{
	if (tcp_conn_seg_proc_ack_est(conn, seg) == cp_done)
		return cp_done;

	/* TODO */
	return cp_continue;
}

/** Process segment ACK field in Fin-Wait-2 state.
 *
 * @param conn		Connection
 * @param seg		Segment
 * @return		cp_done if we are done with this segment, cp_continue
 *			if not
 */
static cproc_t tcp_conn_seg_proc_ack_fw2(tcp_conn_t *conn, tcp_segment_t *seg)
{
	if (tcp_conn_seg_proc_ack_est(conn, seg) == cp_done)
		return cp_done;

	/* TODO */
	return cp_continue;
}

/** Process segment ACK field in Close-Wait state.
 *
 * @param conn		Connection
 * @param seg		Segment
 * @return		cp_done if we are done with this segment, cp_continue
 *			if not
 */
static cproc_t tcp_conn_seg_proc_ack_cw(tcp_conn_t *conn, tcp_segment_t *seg)
{
	/* The same processing as in Established state */
	return tcp_conn_seg_proc_ack_est(conn, seg);
}

/** Process segment ACK field in Closing state.
 *
 * @param conn		Connection
 * @param seg		Segment
 * @return		cp_done if we are done with this segment, cp_continue
 *			if not
 */
static cproc_t tcp_conn_seg_proc_ack_cls(tcp_conn_t *conn, tcp_segment_t *seg)
{
	if (tcp_conn_seg_proc_ack_est(conn, seg) == cp_done)
		return cp_done;

	/* TODO */
	return cp_continue;
}

/** Process segment ACK field in Last-Ack state.
 *
 * @param conn		Connection
 * @param seg		Segment
 * @return		cp_done if we are done with this segment, cp_continue
 *			if not
 */
static cproc_t tcp_conn_seg_proc_ack_la(tcp_conn_t *conn, tcp_segment_t *seg)
{
	if (tcp_conn_seg_proc_ack_est(conn, seg) == cp_done)
		return cp_done;

	/* TODO */
	return cp_continue;
}

/** Process segment ACK field in Time-Wait state.
 *
 * @param conn		Connection
 * @param seg		Segment
 * @return		cp_done if we are done with this segment, cp_continue
 *			if not
 */
static cproc_t tcp_conn_seg_proc_ack_tw(tcp_conn_t *conn, tcp_segment_t *seg)
{
	/* Nothing to do */
	return cp_continue;
}

/** Process segment ACK field.
 *
 * @param conn		Connection
 * @param seg		Segment
 * @return		cp_done if we are done with this segment, cp_continue
 *			if not
 */
static cproc_t tcp_conn_seg_proc_ack(tcp_conn_t *conn, tcp_segment_t *seg)
{
	log_msg(LVL_DEBUG, "tcp_conn_seg_proc_ack(%p, %p)", conn, seg);

	if ((seg->ctrl & CTL_ACK) == 0) {
		log_msg(LVL_WARN, "Segment has no ACK. Dropping.");
		tcp_segment_delete(seg);
		return cp_done;
	}

	switch (conn->cstate) {
	case st_syn_received:
		return tcp_conn_seg_proc_ack_sr(conn, seg);
	case st_established:
		return tcp_conn_seg_proc_ack_est(conn, seg);
	case st_fin_wait_1:
		return tcp_conn_seg_proc_ack_fw1(conn, seg);
	case st_fin_wait_2:
		return tcp_conn_seg_proc_ack_fw2(conn, seg);
	case st_close_wait:
		return tcp_conn_seg_proc_ack_cw(conn, seg);
	case st_closing:
		return tcp_conn_seg_proc_ack_cls(conn, seg);
	case st_last_ack:
		return tcp_conn_seg_proc_ack_la(conn, seg);
	case st_time_wait:
		return tcp_conn_seg_proc_ack_tw(conn, seg);
	case st_listen:
	case st_syn_sent:
	case st_closed:
		assert(false);
	}

	assert(false);
}

/** Process segment URG field.
 *
 * @param conn		Connection
 * @param seg		Segment
 * @return		cp_done if we are done with this segment, cp_continue
 *			if not
 */
static cproc_t tcp_conn_seg_proc_urg(tcp_conn_t *conn, tcp_segment_t *seg)
{
	return cp_continue;
}

/** Process segment text.
 *
 * @param conn		Connection
 * @param seg		Segment
 * @return		cp_done if we are done with this segment, cp_continue
 *			if not
 */
static cproc_t tcp_conn_seg_proc_text(tcp_conn_t *conn, tcp_segment_t *seg)
{
	size_t text_size;
	size_t xfer_size;

	log_msg(LVL_DEBUG, "tcp_conn_seg_proc_text(%p, %p)", conn, seg);

	switch (conn->cstate) {
	case st_established:
	case st_fin_wait_1:
	case st_fin_wait_2:
		/* OK */
		break;
	case st_close_wait:
	case st_closing:
	case st_last_ack:
	case st_time_wait:
		/* Invalid since FIN has been received. Ignore text. */
		return cp_continue;
	case st_listen:
	case st_syn_sent:
	case st_syn_received:
	case st_closed:
		assert(false);
	}

	/*
	 * Process segment text
	 */
	assert(seq_no_segment_ready(conn, seg));

	/* Trim anything outside our receive window */
	tcp_conn_trim_seg_to_wnd(conn, seg);

	fibril_mutex_lock(&conn->rcv_buf_lock);

	/* Determine how many bytes to copy */
	text_size = tcp_segment_text_size(seg);
	xfer_size = min(text_size, conn->rcv_buf_size - conn->rcv_buf_used);

	/* Copy data to receive buffer */
	tcp_segment_text_copy(seg, conn->rcv_buf + conn->rcv_buf_used,
	    xfer_size);
	conn->rcv_buf_used += xfer_size;

	/* Signal to the receive function that new data has arrived */
	fibril_condvar_broadcast(&conn->rcv_buf_cv);
	fibril_mutex_unlock(&conn->rcv_buf_lock);

	log_msg(LVL_DEBUG, "Received %zu bytes of data.", xfer_size);

	/* Advance RCV.NXT */
	conn->rcv_nxt += xfer_size;

	/* Update receive window. XXX Not an efficient strategy. */
	conn->rcv_wnd -= xfer_size;

	/* Send ACK */
	if (xfer_size > 0)
		tcp_tqueue_ctrl_seg(conn, CTL_ACK);

	if (xfer_size < seg->len) {
		/* Trim part of segment which we just received */
		tcp_conn_trim_seg_to_wnd(conn, seg);
	} else {
		/* Nothing left in segment */
		tcp_segment_delete(seg);
		return cp_done;
	}

	return cp_continue;
}

/** Process segment FIN field.
 *
 * @param conn		Connection
 * @param seg		Segment
 * @return		cp_done if we are done with this segment, cp_continue
 *			if not
 */
static cproc_t tcp_conn_seg_proc_fin(tcp_conn_t *conn, tcp_segment_t *seg)
{
	log_msg(LVL_DEBUG, "tcp_conn_seg_proc_fin(%p, %p)", conn, seg);

	/* Only process FIN if no text is left in segment. */
	if (tcp_segment_text_size(seg) == 0 && (seg->ctrl & CTL_FIN) != 0) {
		log_msg(LVL_DEBUG, " - FIN found in segment.");

		conn->rcv_nxt++;
		conn->rcv_wnd--;

		/* TODO Change connection state */

		/* Add FIN to the receive buffer */
		fibril_mutex_lock(&conn->rcv_buf_lock);
		conn->rcv_buf_fin = true;
		fibril_condvar_broadcast(&conn->rcv_buf_cv);
		fibril_mutex_unlock(&conn->rcv_buf_lock);

		tcp_segment_delete(seg);
		return cp_done;
	}

	return cp_continue;
}

/** Process incoming segment.
 *
 * We are in connection state where segments are processed in order
 * of sequence number. This processes one segment taken from the
 * connection incoming segments queue.
 *
 * @param conn		Connection
 * @param seg		Segment
 */
static void tcp_conn_seg_process(tcp_conn_t *conn, tcp_segment_t *seg)
{
	log_msg(LVL_DEBUG, "tcp_conn_seg_process(%p, %p)", conn, seg);

	/* Check whether segment is acceptable */
	/* XXX Permit valid ACKs, URGs and RSTs */
/*	if (!seq_no_segment_acceptable(conn, seg)) {
		log_msg(LVL_WARN, "Segment not acceptable, dropping.");
		if ((seg->ctrl & CTL_RST) == 0) {
			tcp_tqueue_ctrl_seg(conn, CTL_ACK);
		}
		return;
	}
*/

	if (tcp_conn_seg_proc_rst(conn, seg) == cp_done)
		return;

	if (tcp_conn_seg_proc_sp(conn, seg) == cp_done)
		return;

	if (tcp_conn_seg_proc_syn(conn, seg) == cp_done)
		return;

	if (tcp_conn_seg_proc_ack(conn, seg) == cp_done)
		return;

	if (tcp_conn_seg_proc_urg(conn, seg) == cp_done)
		return;

	if (tcp_conn_seg_proc_text(conn, seg) == cp_done)
		return;

	if (tcp_conn_seg_proc_fin(conn, seg) == cp_done)
		return;

	/*
	 * If anything is left from the segment, insert it back into the
	 * incoming segments queue.
	 */
	if (seg->len > 0)
		tcp_iqueue_insert_seg(&conn->incoming, seg);
	else
		tcp_segment_delete(seg);
}

/** Segment arrived on a connection.
 *
 * @param conn		Connection
 * @param seg		Segment
 */
void tcp_conn_segment_arrived(tcp_conn_t *conn, tcp_segment_t *seg)
{
	log_msg(LVL_DEBUG, "tcp_conn_segment_arrived(%p, %p)", conn, seg);

	switch (conn->cstate) {
	case st_listen:
		tcp_conn_sa_listen(conn, seg); break;
	case st_syn_sent:
		tcp_conn_sa_syn_sent(conn, seg); break;
	case st_syn_received:
	case st_established:
	case st_fin_wait_1:
	case st_fin_wait_2:
	case st_close_wait:
	case st_closing:
	case st_last_ack:
	case st_time_wait:
		/* Process segments in order of sequence number */
		tcp_conn_sa_queue(conn, seg); break;
	case st_closed:
		assert(false);
	}
}

/** Trim segment to the receive window.
 *
 * @param conn		Connection
 * @param seg		Segment
 */
void tcp_conn_trim_seg_to_wnd(tcp_conn_t *conn, tcp_segment_t *seg)
{
	uint32_t left, right;

	seq_no_seg_trim_calc(conn, seg, &left, &right);
	tcp_segment_trim(seg, left, right);
}

/** Handle unexpected segment received on a socket pair.
 *
 * We reply with an RST unless the received segment has RST.
 *
 * @param sp		Socket pair which received the segment
 * @param seg		Unexpected segment
 */
void tcp_unexpected_segment(tcp_sockpair_t *sp, tcp_segment_t *seg)
{
	log_msg(LVL_DEBUG, "tcp_unexpected_segment(%p, %p)", sp, seg);

	if ((seg->ctrl & CTL_RST) == 0)
		tcp_reply_rst(sp, seg);
}

/** Compute flipped socket pair for response.
 *
 * Flipped socket pair has local and foreign sockets exchanged.
 *
 * @param sp		Socket pair
 * @param fsp		Place to store flipped socket pair
 */
void tcp_sockpair_flipped(tcp_sockpair_t *sp, tcp_sockpair_t *fsp)
{
	fsp->local = sp->foreign;
	fsp->foreign = sp->local;
}

/** Send RST in response to an incoming segment.
 *
 * @param sp		Socket pair which received the segment
 * @param seg		Incoming segment
 */
void tcp_reply_rst(tcp_sockpair_t *sp, tcp_segment_t *seg)
{
	tcp_sockpair_t rsp;
	tcp_segment_t *rseg;

	log_msg(LVL_DEBUG, "tcp_reply_rst(%p, %p)", sp, seg);

	tcp_sockpair_flipped(sp, &rsp);
	rseg = tcp_segment_make_rst(seg);
	tcp_transmit_segment(&rsp, rseg);
}

/**
 * @}
 */
