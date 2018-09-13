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
 * @file TCP connection processing and state machine
 */

#include <adt/list.h>
#include <errno.h>
#include <inet/endpoint.h>
#include <io/log.h>
#include <macros.h>
#include <nettl/amap.h>
#include <stdbool.h>
#include <stdlib.h>
#include "conn.h"
#include "inet.h"
#include "iqueue.h"
#include "pdu.h"
#include "rqueue.h"
#include "segment.h"
#include "seq_no.h"
#include "tcp_type.h"
#include "tqueue.h"
#include "ucall.h"

#define RCV_BUF_SIZE 4096/*2*/
#define SND_BUF_SIZE 4096

#define MAX_SEGMENT_LIFETIME	(15*1000*1000) //(2*60*1000*1000)
#define TIME_WAIT_TIMEOUT	(2*MAX_SEGMENT_LIFETIME)

/** List of all allocated connections */
static LIST_INITIALIZE(conn_list);
/** Taken after tcp_conn_t lock */
static FIBRIL_MUTEX_INITIALIZE(conn_list_lock);
/** Connection association map */
static amap_t *amap;
/** Taken after tcp_conn_t lock */
static FIBRIL_MUTEX_INITIALIZE(amap_lock);

/** Internal loopback configuration */
tcp_lb_t tcp_conn_lb = tcp_lb_none;

static void tcp_conn_seg_process(tcp_conn_t *, tcp_segment_t *);
static void tcp_conn_tw_timer_set(tcp_conn_t *);
static void tcp_conn_tw_timer_clear(tcp_conn_t *);
static void tcp_transmit_segment(inet_ep2_t *, tcp_segment_t *);
static void tcp_conn_trim_seg_to_wnd(tcp_conn_t *, tcp_segment_t *);
static void tcp_reply_rst(inet_ep2_t *, tcp_segment_t *);

static tcp_tqueue_cb_t tcp_conn_tqueue_cb = {
	.transmit_seg = tcp_transmit_segment
};

/** Initialize connections. */
errno_t tcp_conns_init(void)
{
	errno_t rc;

	rc = amap_create(&amap);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		return ENOMEM;
	}

	return EOK;
}

/** Finalize connections. */
void tcp_conns_fini(void)
{
	assert(list_empty(&conn_list));

	amap_destroy(amap);
	amap = NULL;
}

/** Create new connection structure.
 *
 * @param epp		Endpoint pair (will be deeply copied)
 * @return		New connection or NULL
 */
tcp_conn_t *tcp_conn_new(inet_ep2_t *epp)
{
	tcp_conn_t *conn = NULL;
	bool tqueue_inited = false;

	/* Allocate connection structure */
	conn = calloc(1, sizeof(tcp_conn_t));
	if (conn == NULL)
		goto error;

	fibril_mutex_initialize(&conn->lock);

	conn->tw_timer = fibril_timer_create(&conn->lock);
	if (conn->tw_timer == NULL)
		goto error;

	/* One for the user, one for not being in closed state */
	refcount_init(&conn->refcnt);
	refcount_up(&conn->refcnt);

	/* Allocate receive buffer */
	fibril_condvar_initialize(&conn->rcv_buf_cv);
	conn->rcv_buf_size = RCV_BUF_SIZE;
	conn->rcv_buf_used = 0;
	conn->rcv_buf_fin = false;

	conn->rcv_buf = calloc(1, conn->rcv_buf_size);
	if (conn->rcv_buf == NULL)
		goto error;

	/** Allocate send buffer */
	fibril_condvar_initialize(&conn->snd_buf_cv);
	conn->snd_buf_size = SND_BUF_SIZE;
	conn->snd_buf_used = 0;
	conn->snd_buf_fin = false;
	conn->snd_buf = calloc(1, conn->snd_buf_size);
	if (conn->snd_buf == NULL)
		goto error;

	/* Set up receive window. */
	conn->rcv_wnd = conn->rcv_buf_size;

	/* Initialize incoming segment queue */
	tcp_iqueue_init(&conn->incoming, conn);

	/* Initialize retransmission queue */
	if (tcp_tqueue_init(&conn->retransmit, conn, &tcp_conn_tqueue_cb) !=
	    EOK) {
		goto error;
	}

	tqueue_inited = true;

	/* Connection state change signalling */
	fibril_condvar_initialize(&conn->cstate_cv);

	conn->cb = NULL;

	conn->cstate = st_listen;
	conn->reset = false;
	conn->deleted = false;
	conn->ap = ap_passive;
	conn->fin_is_acked = false;
	if (epp != NULL)
		conn->ident = *epp;

	fibril_mutex_lock(&conn_list_lock);
	list_append(&conn->link, &conn_list);
	fibril_mutex_unlock(&conn_list_lock);

	return conn;

error:
	if (tqueue_inited)
		tcp_tqueue_fini(&conn->retransmit);
	if (conn != NULL && conn->rcv_buf != NULL)
		free(conn->rcv_buf);
	if (conn != NULL && conn->snd_buf != NULL)
		free(conn->snd_buf);
	if (conn != NULL && conn->tw_timer != NULL)
		fibril_timer_destroy(conn->tw_timer);
	if (conn != NULL)
		free(conn);

	return NULL;
}

/** Destroy connection structure.
 *
 * Connection structure should be destroyed when the folowing condtitions
 * are met:
 * (1) user has deleted the connection
 * (2) the connection has entered closed state
 * (3) nobody is holding references to the connection
 *
 * This happens when @a conn->refcnt is zero as we count (1) and (2)
 * as special references.
 *
 * @param conn		Connection
 */
static void tcp_conn_free(tcp_conn_t *conn)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: tcp_conn_free(%p)", conn->name, conn);

	assert(conn->mapped == false);
	tcp_tqueue_fini(&conn->retransmit);

	fibril_mutex_lock(&conn_list_lock);
	list_remove(&conn->link);
	fibril_mutex_unlock(&conn_list_lock);

	if (conn->rcv_buf != NULL)
		free(conn->rcv_buf);
	if (conn->snd_buf != NULL)
		free(conn->snd_buf);
	if (conn->tw_timer != NULL)
		fibril_timer_destroy(conn->tw_timer);
	free(conn);
}

/** Add reference to connection.
 *
 * Increase connection reference count by one.
 *
 * @param conn		Connection
 */
void tcp_conn_addref(tcp_conn_t *conn)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG2, "%s: tcp_conn_addref(%p)",
	    conn->name, conn);

	refcount_up(&conn->refcnt);
}

/** Remove reference from connection.
 *
 * Decrease connection reference count by one.
 *
 * @param conn		Connection
 */
void tcp_conn_delref(tcp_conn_t *conn)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG2, "%s: tcp_conn_delref(%p)",
	    conn->name, conn);

	if (refcount_down(&conn->refcnt))
		tcp_conn_free(conn);
}

/** Lock connection.
 *
 * Must be called before any other connection-manipulating function,
 * except tcp_conn_{add|del}ref(). Locks the connection including
 * its timers. Must not be called inside any of the connection
 * timer handlers.
 *
 * @param conn		Connection
 */
void tcp_conn_lock(tcp_conn_t *conn)
{
	fibril_mutex_lock(&conn->lock);
}

/** Unlock connection.
 *
 * @param conn		Connection
 */
void tcp_conn_unlock(tcp_conn_t *conn)
{
	fibril_mutex_unlock(&conn->lock);
}

/** Delete connection.
 *
 * The caller promises not make no further references to @a conn.
 * TCP will free @a conn eventually.
 *
 * @param conn		Connection
 */
void tcp_conn_delete(tcp_conn_t *conn)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: tcp_conn_delete(%p)", conn->name, conn);

	assert(conn->deleted == false);
	conn->deleted = true;
	conn->cb = NULL;
	conn->cb_arg = NULL;
	tcp_conn_delref(conn);
}

/** Enlist connection.
 *
 * Add connection to the connection map.
 */
errno_t tcp_conn_add(tcp_conn_t *conn)
{
	inet_ep2_t aepp;
	errno_t rc;

	tcp_conn_addref(conn);
	fibril_mutex_lock(&amap_lock);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_add: conn=%p", conn);

	rc = amap_insert(amap, &conn->ident, conn, af_allow_system, &aepp);
	if (rc != EOK) {
		tcp_conn_delref(conn);
		fibril_mutex_unlock(&amap_lock);
		return rc;
	}

	conn->ident = aepp;
	conn->mapped = true;
	fibril_mutex_unlock(&amap_lock);

	return EOK;
}

/** Delist connection.
 *
 * Remove connection from the connection map.
 */
static void tcp_conn_remove(tcp_conn_t *conn)
{
	if (!conn->mapped)
		return;

	fibril_mutex_lock(&amap_lock);
	amap_remove(amap, &conn->ident);
	conn->mapped = false;
	fibril_mutex_unlock(&amap_lock);
	tcp_conn_delref(conn);
}

static void tcp_conn_state_set(tcp_conn_t *conn, tcp_cstate_t nstate)
{
	tcp_cstate_t old_state;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_state_set(%p)", conn);

	old_state = conn->cstate;
	conn->cstate = nstate;
	fibril_condvar_broadcast(&conn->cstate_cv);

	/* Run user callback function */
	if (conn->cb != NULL && conn->cb->cstate_change != NULL) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_state_set() - run user CB");
		conn->cb->cstate_change(conn, conn->cb_arg, old_state);
	} else {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_state_set() - no user CB");
	}

	assert(old_state != st_closed);
	if (nstate == st_closed) {
		tcp_conn_remove(conn);
		/* Drop one reference for now being in closed state */
		tcp_conn_delref(conn);
	}
}

/** Synchronize connection.
 *
 * This is the first step of an active connection attempt,
 * sends out SYN and sets up ISS and SND.xxx.
 */
void tcp_conn_sync(tcp_conn_t *conn)
{
	assert(fibril_mutex_is_locked(&conn->lock));

	/* XXX select ISS */
	conn->iss = 1;
	conn->snd_nxt = conn->iss;
	conn->snd_una = conn->iss;
	conn->ap = ap_active;

	tcp_tqueue_ctrl_seg(conn, CTL_SYN);
	tcp_conn_state_set(conn, st_syn_sent);
}

/** FIN has been sent.
 *
 * This function should be called when FIN is sent over the connection,
 * as a result the connection state is changed appropriately.
 */
void tcp_conn_fin_sent(tcp_conn_t *conn)
{
	switch (conn->cstate) {
	case st_syn_received:
	case st_established:
		log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: FIN sent -> Fin-Wait-1", conn->name);
		tcp_conn_state_set(conn, st_fin_wait_1);
		break;
	case st_close_wait:
		log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: FIN sent -> Last-Ack", conn->name);
		tcp_conn_state_set(conn, st_last_ack);
		break;
	default:
		log_msg(LOG_DEFAULT, LVL_ERROR, "%s: Connection state %d", conn->name,
		    conn->cstate);
		assert(false);
	}

	conn->fin_is_acked = false;
}

/** Find connection structure for specified endpoint pair.
 *
 * A connection is uniquely identified by a endpoint pair. Look up our
 * connection map and return connection structure based on endpoint pair.
 * The connection reference count is bumped by one.
 *
 * @param epp	Endpoint pair
 * @return	Connection structure or NULL if not found.
 */
tcp_conn_t *tcp_conn_find_ref(inet_ep2_t *epp)
{
	errno_t rc;
	void *arg;
	tcp_conn_t *conn;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_find_ref(%p)", epp);

	fibril_mutex_lock(&amap_lock);

	rc = amap_find_match(amap, epp, &arg);
	if (rc != EOK) {
		assert(rc == ENOENT);
		fibril_mutex_unlock(&amap_lock);
		return NULL;
	}

	conn = (tcp_conn_t *)arg;
	tcp_conn_addref(conn);

	fibril_mutex_unlock(&amap_lock);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_find_ref: got conn=%p",
	    conn);
	return conn;
}

/** Reset connection.
 *
 * @param conn	Connection
 */
void tcp_conn_reset(tcp_conn_t *conn)
{
	assert(fibril_mutex_is_locked(&conn->lock));

	log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: tcp_conn_reset()", conn->name);

	if (conn->cstate == st_closed)
		return;

	conn->reset = true;
	tcp_conn_state_set(conn, st_closed);

	tcp_conn_tw_timer_clear(conn);
	tcp_tqueue_clear(&conn->retransmit);

	fibril_condvar_broadcast(&conn->rcv_buf_cv);
	fibril_condvar_broadcast(&conn->snd_buf_cv);
}

/** Signal to the user that connection has been reset.
 *
 * Send an out-of-band signal to the user.
 */
static void tcp_reset_signal(tcp_conn_t *conn)
{
	/* TODO */
	log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: tcp_reset_signal()", conn->name);
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
		log_msg(LOG_DEFAULT, LVL_WARN, "state=%d", (int) conn->cstate);
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
	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_sa_listen(%p, %p)", conn, seg);

	if ((seg->ctrl & CTL_RST) != 0) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Ignoring incoming RST.");
		return;
	}

	if ((seg->ctrl & CTL_ACK) != 0) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Incoming ACK, send acceptable RST.");
		tcp_reply_rst(&conn->ident, seg);
		return;
	}

	if ((seg->ctrl & CTL_SYN) == 0) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "SYN not present. Ignoring segment.");
		return;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Got SYN, sending SYN, ACK.");

	conn->rcv_nxt = seg->seq + 1;
	conn->irs = seg->seq;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "rcv_nxt=%u", conn->rcv_nxt);

	if (seg->len > 1)
		log_msg(LOG_DEFAULT, LVL_WARN, "SYN combined with data, ignoring data.");

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

	tcp_conn_state_set(conn, st_syn_received);

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
	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_sa_syn_sent(%p, %p)", conn, seg);

	if ((seg->ctrl & CTL_ACK) != 0) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "snd_una=%u, seg.ack=%u, snd_nxt=%u",
		    conn->snd_una, seg->ack, conn->snd_nxt);
		if (!seq_no_ack_acceptable(conn, seg->ack)) {
			if ((seg->ctrl & CTL_RST) == 0) {
				log_msg(LOG_DEFAULT, LVL_WARN, "ACK not acceptable, send RST");
				tcp_reply_rst(&conn->ident, seg);
			} else {
				log_msg(LOG_DEFAULT, LVL_WARN, "RST,ACK not acceptable, drop");
			}
			return;
		}
	}

	if ((seg->ctrl & CTL_RST) != 0) {
		/* If we get here, we have either an acceptable ACK or no ACK */
		if ((seg->ctrl & CTL_ACK) != 0) {
			log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: Connection reset. -> Closed",
			    conn->name);
			/* Reset connection */
			tcp_conn_reset(conn);
			return;
		} else {
			log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: RST without ACK, drop",
			    conn->name);
			return;
		}
	}

	/* XXX precedence */

	if ((seg->ctrl & CTL_SYN) == 0) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "No SYN bit, ignoring segment.");
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

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Sent SYN, got SYN.");

	/*
	 * Surprisingly the spec does not deal with initial window setting.
	 * Set SND.WND = SEG.WND and set SND.WL1 so that next segment
	 * will always be accepted as new window setting.
	 */
	log_msg(LOG_DEFAULT, LVL_DEBUG, "SND.WND := %" PRIu32 ", SND.WL1 := %" PRIu32 ", "
	    "SND.WL2 = %" PRIu32, seg->wnd, seg->seq, seg->seq);
	conn->snd_wnd = seg->wnd;
	conn->snd_wl1 = seg->seq;
	conn->snd_wl2 = seg->seq;

	if (seq_no_syn_acked(conn)) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: syn acked -> Established", conn->name);
		tcp_conn_state_set(conn, st_established);
		tcp_tqueue_ctrl_seg(conn, CTL_ACK /* XXX */);
	} else {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: syn not acked -> Syn-Received",
		    conn->name);
		tcp_conn_state_set(conn, st_syn_received);
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

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_sa_seq(%p, %p)", conn, seg);

	/* Discard unacceptable segments ("old duplicates") */
	if (!seq_no_segment_acceptable(conn, seg)) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Replying ACK to unacceptable segment.");
		tcp_tqueue_ctrl_seg(conn, CTL_ACK);
		tcp_segment_delete(seg);
		return;
	}

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
	if ((seg->ctrl & CTL_RST) == 0)
		return cp_continue;

	switch (conn->cstate) {
	case st_syn_received:
		/* XXX In case of passive open, revert to Listen state */
		if (conn->ap == ap_passive) {
			tcp_conn_state_set(conn, st_listen);
			/* XXX Revert conn->ident */
			tcp_conn_tw_timer_clear(conn);
			tcp_tqueue_clear(&conn->retransmit);
		} else {
			tcp_conn_reset(conn);
		}
		break;
	case st_established:
	case st_fin_wait_1:
	case st_fin_wait_2:
	case st_close_wait:
		/* General "connection reset" signal */
		tcp_reset_signal(conn);
		tcp_conn_reset(conn);
		break;
	case st_closing:
	case st_last_ack:
	case st_time_wait:
		tcp_conn_reset(conn);
		break;
	case st_listen:
	case st_syn_sent:
	case st_closed:
		assert(false);
	}

	return cp_done;
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
	if ((seg->ctrl & CTL_SYN) == 0)
		return cp_continue;

	/*
	 * Assert SYN is in receive window, otherwise this step should not
	 * be reached.
	 */
	assert(seq_no_in_rcv_wnd(conn, seg->seq));

	log_msg(LOG_DEFAULT, LVL_WARN, "SYN is in receive window, should send reset. XXX");

	/*
	 * TODO
	 *
	 * Send a reset, resond "reset" to all outstanding RECEIVEs and SEND,
	 * flush segment queues. Send unsolicited "connection reset" signal
	 * to user, connection -> closed state, delete TCB, return.
	 */
	return cp_done;
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
		log_msg(LOG_DEFAULT, LVL_WARN, "Segment ACK not acceptable, sending RST.");
		tcp_reply_rst(&conn->ident, seg);
		tcp_segment_delete(seg);
		return cp_done;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: SYN ACKed -> Established", conn->name);

	tcp_conn_state_set(conn, st_established);

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
	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_seg_proc_ack_est(%p, %p)", conn, seg);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "SEG.ACK=%u, SND.UNA=%u, SND.NXT=%u",
	    (unsigned)seg->ack, (unsigned)conn->snd_una,
	    (unsigned)conn->snd_nxt);

	if (!seq_no_ack_acceptable(conn, seg->ack)) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "ACK not acceptable.");
		if (!seq_no_ack_duplicate(conn, seg->ack)) {
			log_msg(LOG_DEFAULT, LVL_WARN, "Not acceptable, not duplicate. "
			    "Send ACK and drop.");
			/* Not acceptable, not duplicate. Send ACK and drop. */
			tcp_tqueue_ctrl_seg(conn, CTL_ACK);
			tcp_segment_delete(seg);
			return cp_done;
		} else {
			log_msg(LOG_DEFAULT, LVL_DEBUG, "Ignoring duplicate ACK.");
		}
	} else {
		/* Update SND.UNA */
		conn->snd_una = seg->ack;
	}

	if (seq_no_new_wnd_update(conn, seg)) {
		conn->snd_wnd = seg->wnd;
		conn->snd_wl1 = seg->seq;
		conn->snd_wl2 = seg->ack;

		log_msg(LOG_DEFAULT, LVL_DEBUG, "Updating send window, SND.WND=%" PRIu32
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

	if (conn->fin_is_acked) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: FIN acked -> Fin-Wait-2", conn->name);
		tcp_conn_state_set(conn, st_fin_wait_2);
	}

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

	if (conn->fin_is_acked) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: FIN acked -> Time-Wait",
		    conn->name);
		tcp_conn_state_set(conn, st_time_wait);
	}

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

	if (conn->fin_is_acked) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: FIN acked -> Closed", conn->name);
		tcp_conn_state_set(conn, st_closed);
		return cp_done;
	}

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
	log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: tcp_conn_seg_proc_ack(%p, %p)",
	    conn->name, conn, seg);

	if ((seg->ctrl & CTL_ACK) == 0) {
		log_msg(LOG_DEFAULT, LVL_WARN, "Segment has no ACK. Dropping.");
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

	log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: tcp_conn_seg_proc_text(%p, %p)",
	    conn->name, conn, seg);

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

	/* Determine how many bytes to copy */
	text_size = tcp_segment_text_size(seg);
	xfer_size = min(text_size, conn->rcv_buf_size - conn->rcv_buf_used);

	/* Copy data to receive buffer */
	tcp_segment_text_copy(seg, conn->rcv_buf + conn->rcv_buf_used,
	    xfer_size);
	conn->rcv_buf_used += xfer_size;

	/* Signal to the receive function that new data has arrived */
	if (xfer_size > 0) {
		fibril_condvar_broadcast(&conn->rcv_buf_cv);
		if (conn->cb != NULL && conn->cb->recv_data != NULL)
			conn->cb->recv_data(conn, conn->cb_arg);
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "Received %zu bytes of data.", xfer_size);

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
		log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: Nothing left in segment, dropping "
		    "(xfer_size=%zu, SEG.LEN=%" PRIu32 ", seg->ctrl=%u)",
		    conn->name, xfer_size, seg->len, (unsigned int) seg->ctrl);
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
	log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: tcp_conn_seg_proc_fin(%p, %p)",
	    conn->name, conn, seg);
	log_msg(LOG_DEFAULT, LVL_DEBUG, " seg->len=%zu, seg->ctl=%u", (size_t) seg->len,
	    (unsigned) seg->ctrl);

	/* Only process FIN if no text is left in segment. */
	if (tcp_segment_text_size(seg) == 0 && (seg->ctrl & CTL_FIN) != 0) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, " - FIN found in segment.");

		conn->rcv_nxt++;
		conn->rcv_wnd--;

		/* Send ACK */
		tcp_tqueue_ctrl_seg(conn, CTL_ACK);

		/* Change connection state */
		switch (conn->cstate) {
		case st_listen:
		case st_syn_sent:
		case st_closed:
			/* Connection not synchronized */
			assert(false);
			/* Fallthrough */
		case st_syn_received:
		case st_established:
			log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: FIN received -> Close-Wait",
			    conn->name);
			tcp_conn_state_set(conn, st_close_wait);
			break;
		case st_fin_wait_1:
			log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: FIN received -> Closing",
			    conn->name);
			tcp_conn_state_set(conn, st_closing);
			break;
		case st_fin_wait_2:
			log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: FIN received -> Time-Wait",
			    conn->name);
			tcp_conn_state_set(conn, st_time_wait);
			/* Start the Time-Wait timer */
			tcp_conn_tw_timer_set(conn);
			break;
		case st_close_wait:
		case st_closing:
		case st_last_ack:
			/* Do nothing */
			break;
		case st_time_wait:
			/* Restart the Time-Wait timer */
			tcp_conn_tw_timer_set(conn);
			break;
		}

		/* Add FIN to the receive buffer */
		conn->rcv_buf_fin = true;
		fibril_condvar_broadcast(&conn->rcv_buf_cv);
		if (conn->cb != NULL && conn->cb->recv_data != NULL)
			conn->cb->recv_data(conn, conn->cb_arg);

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
	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_conn_seg_process(%p, %p)", conn, seg);
	tcp_segment_dump(seg);

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
	if (seg->len > 0) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Re-insert segment %p. seg->len=%zu",
		    seg, (size_t) seg->len);
		tcp_iqueue_insert_seg(&conn->incoming, seg);
	} else {
		tcp_segment_delete(seg);
	}
}

/** Segment arrived on a connection.
 *
 * @param conn		Connection
 * @param epp		Endpoint pair on which segment was received
 * @param seg		Segment
 */
void tcp_conn_segment_arrived(tcp_conn_t *conn, inet_ep2_t *epp,
    tcp_segment_t *seg)
{
	inet_ep2_t aepp;
	inet_ep2_t oldepp;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: tcp_conn_segment_arrived(%p)",
	    conn->name, seg);

	tcp_conn_lock(conn);

	if (conn->cstate == st_closed) {
		log_msg(LOG_DEFAULT, LVL_WARN, "Connection is closed.");
		tcp_unexpected_segment(epp, seg);
		tcp_conn_unlock(conn);
		return;
	}

	if (inet_addr_is_any(&conn->ident.remote.addr) ||
	    conn->ident.remote.port == inet_port_any ||
	    inet_addr_is_any(&conn->ident.local.addr)) {

		log_msg(LOG_DEFAULT, LVL_DEBUG2, "tcp_conn_segment_arrived: "
		    "Changing connection ID, updating amap.");
		oldepp = conn->ident;

		/* Need to remove and re-insert connection with new identity */
		fibril_mutex_lock(&amap_lock);

		if (inet_addr_is_any(&conn->ident.remote.addr))
			conn->ident.remote.addr = epp->remote.addr;

		if (conn->ident.remote.port == inet_port_any)
			conn->ident.remote.port = epp->remote.port;

		if (inet_addr_is_any(&conn->ident.local.addr))
			conn->ident.local.addr = epp->local.addr;

		rc = amap_insert(amap, &conn->ident, conn, af_allow_system, &aepp);
		if (rc != EOK) {
			assert(rc != EEXIST);
			assert(rc == ENOMEM);
			log_msg(LOG_DEFAULT, LVL_ERROR, "Out of memory.");
			fibril_mutex_unlock(&amap_lock);
			tcp_conn_unlock(conn);
			return;
		}

		amap_remove(amap, &oldepp);
		fibril_mutex_unlock(&amap_lock);

		conn->name = (char *) "a";
	}

	switch (conn->cstate) {
	case st_listen:
		tcp_conn_sa_listen(conn, seg);
		break;
	case st_syn_sent:
		tcp_conn_sa_syn_sent(conn, seg);
		break;
	case st_syn_received:
	case st_established:
	case st_fin_wait_1:
	case st_fin_wait_2:
	case st_close_wait:
	case st_closing:
	case st_last_ack:
	case st_time_wait:
		/* Process segments in order of sequence number */
		tcp_conn_sa_queue(conn, seg);
		break;
	case st_closed:
		log_msg(LOG_DEFAULT, LVL_DEBUG, "state=%d", (int) conn->cstate);
		assert(false);
	}

	tcp_conn_unlock(conn);
}

/** Time-Wait timeout handler.
 *
 * @param arg	Connection
 */
static void tw_timeout_func(void *arg)
{
	tcp_conn_t *conn = (tcp_conn_t *) arg;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tw_timeout_func(%p)", conn);

	tcp_conn_lock(conn);

	if (conn->cstate == st_closed) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Connection already closed.");
		tcp_conn_unlock(conn);
		tcp_conn_delref(conn);
		return;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "%s: TW Timeout -> Closed", conn->name);
	tcp_conn_state_set(conn, st_closed);

	tcp_conn_unlock(conn);
	tcp_conn_delref(conn);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tw_timeout_func(%p) end", conn);
}

/** Start or restart the Time-Wait timeout.
 *
 * @param conn		Connection
 */
void tcp_conn_tw_timer_set(tcp_conn_t *conn)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG2, "tcp_conn_tw_timer_set() begin");
	tcp_conn_addref(conn);
	fibril_timer_set_locked(conn->tw_timer, TIME_WAIT_TIMEOUT,
	    tw_timeout_func, (void *)conn);
	log_msg(LOG_DEFAULT, LVL_DEBUG2, "tcp_conn_tw_timer_set() end");
}

/** Clear the Time-Wait timeout.
 *
 * @param conn		Connection
 */
void tcp_conn_tw_timer_clear(tcp_conn_t *conn)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG2, "tcp_conn_tw_timer_clear() begin");
	if (fibril_timer_clear_locked(conn->tw_timer) == fts_active)
		tcp_conn_delref(conn);
	log_msg(LOG_DEFAULT, LVL_DEBUG2, "tcp_conn_tw_timer_clear() end");
}

/** Trim segment to the receive window.
 *
 * @param conn		Connection
 * @param seg		Segment
 */
static void tcp_conn_trim_seg_to_wnd(tcp_conn_t *conn, tcp_segment_t *seg)
{
	uint32_t left, right;

	seq_no_seg_trim_calc(conn, seg, &left, &right);
	tcp_segment_trim(seg, left, right);
}

/** Handle unexpected segment received on an endpoint pair.
 *
 * We reply with an RST unless the received segment has RST.
 *
 * @param sp		Endpoint pair which received the segment
 * @param seg		Unexpected segment
 */
void tcp_unexpected_segment(inet_ep2_t *epp, tcp_segment_t *seg)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_unexpected_segment(%p, %p)", epp,
	    seg);

	if ((seg->ctrl & CTL_RST) == 0)
		tcp_reply_rst(epp, seg);
}

/** Transmit segment over network.
 *
 * @param epp Endpoint pair with source and destination information
 * @param seg Segment (ownership retained by caller)
 */
static void tcp_transmit_segment(inet_ep2_t *epp, tcp_segment_t *seg)
{
	tcp_pdu_t *pdu;
	tcp_segment_t *dseg;
	inet_ep2_t rident;

	log_msg(LOG_DEFAULT, LVL_DEBUG,
	    "tcp_transmit_segment(l:(%u),f:(%u), %p)",
	    epp->local.port, epp->remote.port, seg);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "SEG.SEQ=%" PRIu32 ", SEG.WND=%" PRIu32,
	    seg->seq, seg->wnd);

	tcp_segment_dump(seg);

	if (tcp_conn_lb == tcp_lb_segment) {
		/* Loop back segment */
#if 0
		tcp_ncsim_bounce_seg(sp, seg);
#endif

		/* Reverse the identification */
		tcp_ep2_flipped(epp, &rident);

		/* Insert segment back into rqueue */
		dseg = tcp_segment_dup(seg);
		tcp_rqueue_insert_seg(&rident, dseg);
		return;
	}

	if (tcp_pdu_encode(epp, seg, &pdu) != EOK) {
		log_msg(LOG_DEFAULT, LVL_WARN, "Not enough memory. Segment dropped.");
		return;
	}

	if (tcp_conn_lb == tcp_lb_pdu) {
		/* Loop back PDU */
		if (tcp_pdu_decode(pdu, &rident, &dseg) != EOK) {
			log_msg(LOG_DEFAULT, LVL_WARN, "Not enough memory. Segment dropped.");
			tcp_pdu_delete(pdu);
			return;
		}

		tcp_pdu_delete(pdu);

		/* Insert decoded segment into rqueue */
		tcp_rqueue_insert_seg(&rident, dseg);
		return;
	}

	tcp_transmit_pdu(pdu);
	tcp_pdu_delete(pdu);
}

/** Compute flipped endpoint pair for response.
 *
 * Flipped endpoint pair has local and remote endpoints exchanged.
 *
 * @param epp		Endpoint pair
 * @param fepp		Place to store flipped endpoint pair
 */
void tcp_ep2_flipped(inet_ep2_t *epp, inet_ep2_t *fepp)
{
	fepp->local = epp->remote;
	fepp->remote = epp->local;
}

/** Send RST in response to an incoming segment.
 *
 * @param epp		Endpoint pair which received the segment
 * @param seg		Incoming segment
 */
static void tcp_reply_rst(inet_ep2_t *epp, tcp_segment_t *seg)
{
	tcp_segment_t *rseg;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "tcp_reply_rst(%p, %p)", epp, seg);

	rseg = tcp_segment_make_rst(seg);
	tcp_transmit_segment(epp, rseg);
}

/**
 * @}
 */
