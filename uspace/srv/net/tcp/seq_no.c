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
 * @file Sequence number computations
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include "seq_no.h"
#include "tcp_type.h"

/** a <= b < c modulo sequence space */
static bool seq_no_le_lt(uint32_t a, uint32_t b, uint32_t c)
{
	if (a <= c) {
		return (a <= b) && (b < c);
	} else {
		return (b < c) || (a <= b);
	}
}

/** a < b <= c modulo sequence space */
static bool seq_no_lt_le(uint32_t a, uint32_t b, uint32_t c)
{
	if (a <= c) {
		return (a < b) && (b <= c);
	} else {
		return (b <= c) || (a < b);
	}
}

/** Determine wheter ack is acceptable (new acknowledgement) */
bool seq_no_ack_acceptable(tcp_conn_t *conn, uint32_t seg_ack)
{
	/* SND.UNA < SEG.ACK <= SND.NXT */
	return seq_no_lt_le(conn->snd_una, seg_ack, conn->snd_nxt);
}

/** Determine wheter ack is duplicate.
 *
 * ACK is duplicate if it refers to a sequence number that has
 * aleady been acked (SEG.ACK <= SND.UNA).
 */
bool seq_no_ack_duplicate(tcp_conn_t *conn, uint32_t seg_ack)
{
	uint32_t diff;

	/*
	 * There does not seem to be a three-point comparison
	 * equivalent of SEG.ACK < SND.UNA. Thus we do it
	 * on a best-effort basis, based on the difference.
	 * [-2^31, 0) means less-than, 0 means equal, [0, 2^31)
	 * means greater-than. Less-than or equal means duplicate.
	 */
	diff = seg_ack - conn->snd_una;
	return diff == 0 || (diff & (0x1 << 31)) != 0;
}

/** Determine if sequence number is in receive window. */
bool seq_no_in_rcv_wnd(tcp_conn_t *conn, uint32_t sn)
{
	return seq_no_le_lt(conn->rcv_nxt, sn, conn->rcv_nxt + conn->rcv_wnd);
}

/** Determine segment has new window update.
 *
 * Window update is new if either SND.WL1 < SEG.SEQ or
 * (SND.WL1 = SEG.SEQ and SND.WL2 <= SEG.ACK).
 */
bool seq_no_new_wnd_update(tcp_conn_t *conn, tcp_segment_t *seg)
{
	bool n_seq, n_ack;

	assert(seq_no_segment_acceptable(conn, seg));

	/*
	 * We make use of the fact that the peer should not ACK anything
	 * beyond our send window (we surely haven't sent that yet)
	 * as we should have filtered those acks out.
	 * We use SND.UNA+SND.WND as the third point of comparison.
	 */

	n_seq = seq_no_lt_le(conn->snd_wl1, seg->seq,
	    conn->snd_una + conn->snd_wnd);

	n_ack = conn->snd_wl1 == seg->seq &&
	    seq_no_le_lt(conn->snd_wl2, seg->ack,
	    conn->snd_una + conn->snd_wnd + 1);

	return n_seq || n_ack;
}

/** Determine if segment is ready for processing.
 *
 * Assuming segment is acceptable, a segment is ready if it intersects
 * RCV.NXT, that is we can process it immediately.
 */
bool seq_no_segment_ready(tcp_conn_t *conn, tcp_segment_t *seg)
{
	assert(seq_no_segment_acceptable(conn, seg));

	return seq_no_le_lt(seg->seq, conn->rcv_nxt, seg->seq + seg->len + 1);
}

/** Determine whether segment is fully acked.
 *
 * @param conn Connection
 * @param seg  Segment
 * @param ack  Last received ACK (i.e. SND.UNA)
 *
 * @return @c true if segment is fully acked, @c false otherwise
 */
bool seq_no_segment_acked(tcp_conn_t *conn, tcp_segment_t *seg, uint32_t ack)
{
	assert(seg->len > 0);
	return seq_no_lt_le(seg->seq, seg->seq + seg->len, ack);
}

/** Determine whether initial SYN is acked.
 *
 * @param conn Connection
 * @return @c true if initial SYN is acked, @c false otherwise
 */
bool seq_no_syn_acked(tcp_conn_t *conn)
{
	return seq_no_lt_le(conn->iss, conn->snd_una, conn->snd_nxt);
}

/** Determine whether segment overlaps the receive window.
 *
 * @param conn Connection
 * @param seg  Segment
 * @return @c true if segment overlaps the receive window, @c false otherwise
 */
bool seq_no_segment_acceptable(tcp_conn_t *conn, tcp_segment_t *seg)
{
	bool b_in, e_in;
	bool wb_in, we_in;

	/* Beginning of segment is inside window */
	b_in = seq_no_le_lt(conn->rcv_nxt, seg->seq, conn->rcv_nxt +
	    conn->rcv_wnd);

	/* End of segment is inside window */
	e_in = seq_no_le_lt(conn->rcv_nxt, seg->seq + seg->len - 1,
	    conn->rcv_nxt + conn->rcv_wnd);

	/* Beginning of window is inside segment */
	wb_in = seq_no_le_lt(seg->seq, conn->rcv_nxt,
	    seg->seq + seg->len);

	/* End of window is inside segment */
	we_in = seq_no_le_lt(seg->seq, conn->rcv_nxt + conn->rcv_wnd - 1,
	    seg->seq + seg->len);

	if (seg->len == 0 && conn->rcv_wnd == 0) {
		return seg->seq == conn->rcv_nxt;
	} else if (seg->len == 0 && conn->rcv_wnd != 0) {
		return b_in;
	} else if (seg->len > 0 && conn->rcv_wnd == 0) {
		return false;
	} else {
		return b_in || e_in || wb_in || we_in;
	}
}

/** Determine size that control bits occupy in sequence space.
 *
 * @param ctrl Control bits combination
 * @return Number of sequence space units occupied
 */
uint32_t seq_no_control_len(tcp_control_t ctrl)
{
	uint32_t len = 0;

	if ((ctrl & CTL_SYN) != 0)
		++len;

	if ((ctrl & CTL_FIN) != 0)
		++len;

	return len;
}

/** Calculate the amount of trim needed to fit segment in receive window.
 *
 * @param conn  Connection
 * @param seg   Segment
 * @param left  Place to store number of units to trim at the beginning
 * @param right Place to store number of units to trim at the end
 */
void seq_no_seg_trim_calc(tcp_conn_t *conn, tcp_segment_t *seg,
    uint32_t *left, uint32_t *right)
{
	assert(seq_no_segment_acceptable(conn, seg));

	/*
	 * If RCV.NXT is between SEG.SEQ and RCV.NXT+RCV.WND, then
	 * left trim amount is positive
	 */
	if (seq_no_lt_le(seg->seq, conn->rcv_nxt,
	    conn->rcv_nxt + conn->rcv_wnd)) {
		*left = conn->rcv_nxt - seg->seq;
	} else {
		*left = 0;
	}

	/*
	 * If SEG.SEQ+SEG.LEN is between SEG.SEQ and RCV.NXT+RCV.WND,
	 * then right trim is zero.
	 */
	if (seq_no_lt_le(seg->seq - 1, seg->seq + seg->len,
	    conn->rcv_nxt + conn->rcv_wnd)) {
		*right = 0;
	} else {
		*right = (seg->seq + seg->len) -
		    (conn->rcv_nxt + conn->rcv_wnd);
	}
}

/** Segment order comparison.
 *
 * Compare sequence order of two acceptable segments.
 *
 * @param conn		Connection
 * @param sa		Segment A
 * @param sb		Segment B
 *
 * @return		-1, 0, 1, resp. if A < B, A == B, A > B in terms
 *			of sequence order of the beginning of the segment.
 */
int seq_no_seg_cmp(tcp_conn_t *conn, tcp_segment_t *sa, tcp_segment_t *sb)
{
	assert(seq_no_segment_acceptable(conn, sa));
	assert(seq_no_segment_acceptable(conn, sb));

	if (seq_no_lt_le(sa->seq, sb->seq, conn->rcv_nxt + conn->rcv_wnd))
		return -1;

	if (seq_no_lt_le(sb->seq, sa->seq, conn->rcv_nxt + conn->rcv_wnd))
		return +1;

	assert(sa->seq == sb->seq);
	return 0;
}

/**
 * @}
 */
