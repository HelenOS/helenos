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
 * @file
 */

#include <assert.h>
#include <bool.h>
#include <sys/types.h>
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

/** Determine whether segment is fully acked */
bool seq_no_segment_acked(tcp_conn_t *conn, tcp_segment_t *seg, uint32_t ack)
{
	assert(seg->len > 0);
	return seq_no_lt_le(seg->seq, seg->seq + seg->len, ack);
}

/** Determine whether initial SYN is acked */
bool seq_no_syn_acked(tcp_conn_t *conn)
{
	return seq_no_lt_le(conn->iss, conn->snd_una, conn->snd_nxt);
}

/** Determine whether segment overlaps the receive window */
bool seq_no_segment_acceptable(tcp_conn_t *conn, tcp_segment_t *seg)
{
	bool b_in, e_in;

	b_in = seq_no_le_lt(conn->rcv_nxt, seg->seq, conn->rcv_nxt
	    + conn->rcv_wnd);

	e_in = seq_no_le_lt(conn->rcv_nxt, seg->seq + seg->len - 1,
	    conn->rcv_nxt + conn->rcv_wnd);

	if (seg->len == 0 && conn->rcv_wnd == 0) {
		return seg->seq == conn->rcv_nxt;
	} else if (seg->len == 0 && conn->rcv_wnd != 0) {
		return b_in;
	} else if (seg->len > 0 && conn->rcv_wnd == 0) {
		return false;
	} else {
		return b_in || e_in;
	}
}

/** Determine size that control bits occupy in sequence space. */
uint32_t seq_no_control_len(tcp_control_t ctrl)
{
	uint32_t len = 0;

	if ((ctrl & CTL_SYN) != 0)
		++len;

	if ((ctrl & CTL_FIN) != 0)
		++len;

	return len;
}

/**
 * @}
 */
