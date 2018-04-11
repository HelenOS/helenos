/*
 * Copyright (c) 2017 Jiri Svoboda
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

#include <errno.h>
#include <inet/endpoint.h>
#include <pcut/pcut.h>

#include "../conn.h"
#include "../segment.h"
#include "../seq_no.h"

PCUT_INIT;

PCUT_TEST_SUITE(seq_no);

/** Test seq_no_ack_acceptable() */
PCUT_TEST(ack_acceptable)
{
	tcp_conn_t *conn;
	inet_ep2_t epp;

	inet_ep2_init(&epp);
	conn = tcp_conn_new(&epp);
	PCUT_ASSERT_NOT_NULL(conn);

	/* ACK is acceptable iff SND.UNA < SEG.ACK <= SND.NXT */

	conn->snd_una = 10;
	conn->snd_nxt = 30;

	PCUT_ASSERT_FALSE(seq_no_ack_acceptable(conn, 9));
	PCUT_ASSERT_FALSE(seq_no_ack_acceptable(conn, 10));
	PCUT_ASSERT_TRUE(seq_no_ack_acceptable(conn, 11));
	PCUT_ASSERT_TRUE(seq_no_ack_acceptable(conn, 29));
	PCUT_ASSERT_TRUE(seq_no_ack_acceptable(conn, 30));
	PCUT_ASSERT_FALSE(seq_no_ack_acceptable(conn, 31));

	/* We also test whether seq_no_lt_le() wraps around properly */

	conn->snd_una = 30;
	conn->snd_nxt = 10;

	PCUT_ASSERT_FALSE(seq_no_ack_acceptable(conn, 29));
	PCUT_ASSERT_FALSE(seq_no_ack_acceptable(conn, 30));
	PCUT_ASSERT_TRUE(seq_no_ack_acceptable(conn, 31));
	PCUT_ASSERT_TRUE(seq_no_ack_acceptable(conn, 9));
	PCUT_ASSERT_TRUE(seq_no_ack_acceptable(conn, 10));
	PCUT_ASSERT_FALSE(seq_no_ack_acceptable(conn, 11));

	tcp_conn_delete(conn);
}

/** Test seq_no_ack_duplicate() */
PCUT_TEST(ack_duplicate)
{
	tcp_conn_t *conn;
	inet_ep2_t epp;

	inet_ep2_init(&epp);
	conn = tcp_conn_new(&epp);
	PCUT_ASSERT_NOT_NULL(conn);

	/* ACK is duplicate iff SEG.ACK <= SND.UNA */

	conn->snd_una = 10;

	PCUT_ASSERT_TRUE(seq_no_ack_duplicate(conn, 9));
	PCUT_ASSERT_TRUE(seq_no_ack_duplicate(conn, 10));
	PCUT_ASSERT_FALSE(seq_no_ack_duplicate(conn, 11));

	conn->snd_una = (uint32_t) -10;

	PCUT_ASSERT_TRUE(seq_no_ack_duplicate(conn, (uint32_t) -11));
	PCUT_ASSERT_TRUE(seq_no_ack_duplicate(conn, (uint32_t) -10));
	PCUT_ASSERT_FALSE(seq_no_ack_duplicate(conn, (uint32_t) -9));

	tcp_conn_delete(conn);
}

/** Test seq_no_in_rcv_wnd() */
PCUT_TEST(in_rcv_wnd)
{
	tcp_conn_t *conn;
	inet_ep2_t epp;

	inet_ep2_init(&epp);
	conn = tcp_conn_new(&epp);
	PCUT_ASSERT_NOT_NULL(conn);

	/* In receive window iff RCV.WND <= SEG.SEQ <= SND.UNA */

	conn->rcv_nxt = 10;
	conn->rcv_wnd = 20;

	PCUT_ASSERT_FALSE(seq_no_in_rcv_wnd(conn, 9));
	PCUT_ASSERT_TRUE(seq_no_in_rcv_wnd(conn, 10));
	PCUT_ASSERT_TRUE(seq_no_in_rcv_wnd(conn, 11));
	PCUT_ASSERT_TRUE(seq_no_in_rcv_wnd(conn, 29));
	PCUT_ASSERT_FALSE(seq_no_in_rcv_wnd(conn, 30));
	PCUT_ASSERT_FALSE(seq_no_in_rcv_wnd(conn, 31));

	/* We also test whether seq_no_le_lt() wraps around properly */

	conn->rcv_nxt = 20;
	conn->rcv_wnd = (uint32_t) -10;

	PCUT_ASSERT_FALSE(seq_no_in_rcv_wnd(conn, 19));
	PCUT_ASSERT_TRUE(seq_no_in_rcv_wnd(conn, 20));
	PCUT_ASSERT_TRUE(seq_no_in_rcv_wnd(conn, 21));
	PCUT_ASSERT_TRUE(seq_no_in_rcv_wnd(conn, 9));
	PCUT_ASSERT_FALSE(seq_no_in_rcv_wnd(conn, 10));
	PCUT_ASSERT_FALSE(seq_no_in_rcv_wnd(conn, 11));

	tcp_conn_delete(conn);
}

/** Test seq_no_new_wnd_update() */
PCUT_TEST(new_wnd_update)
{
	tcp_conn_t *conn;
	inet_ep2_t epp;
	tcp_segment_t *seg;

	inet_ep2_init(&epp);
	conn = tcp_conn_new(&epp);
	PCUT_ASSERT_NOT_NULL(conn);

	/*
	 * Segment must be acceptable. Segment has new window update iff
	 * either SND.WL1 < SEG.SEQ or
	 * (SND.WL1 = SEG.SEQ and SND.WL2 <= SEG.ACK)
	 */

	conn->rcv_nxt = 10;
	conn->rcv_wnd = 20;
	conn->snd_una = 30;
	conn->snd_wnd = 40;
	conn->snd_wl1 = 15;
	conn->snd_wl2 = 60;

	seg = tcp_segment_make_ctrl(CTL_ACK);
	PCUT_ASSERT_NOT_NULL(seg);

	seg->seq = 14;
	seg->ack = 80;
	PCUT_ASSERT_FALSE(seq_no_new_wnd_update(conn, seg));

	seg->seq = 15;
	seg->ack = 59;
	PCUT_ASSERT_FALSE(seq_no_new_wnd_update(conn, seg));

	seg->seq = 15;
	seg->ack = 60;
	PCUT_ASSERT_TRUE(seq_no_new_wnd_update(conn, seg));

	seg->seq = 16;
	seg->ack = 50;
	PCUT_ASSERT_TRUE(seq_no_new_wnd_update(conn, seg));

	tcp_segment_delete(seg);
	tcp_conn_delete(conn);
}

/** Test seq_no_segment_acked() */
PCUT_TEST(segment_acked)
{
	tcp_conn_t *conn;
	inet_ep2_t epp;
	tcp_segment_t *seg;
	uint8_t *data;
	size_t dsize;

	inet_ep2_init(&epp);
	conn = tcp_conn_new(&epp);
	PCUT_ASSERT_NOT_NULL(conn);

	dsize = 15;
	data = calloc(dsize, 1);
	PCUT_ASSERT_NOT_NULL(data);

	seg = tcp_segment_make_data(0, data, dsize);
	PCUT_ASSERT_NOT_NULL(seg);

	/* Segment is acked iff SEG.SEQ + SEG.LEN <= SND.UNA */

	seg->seq = 10;
	PCUT_ASSERT_INT_EQUALS(dsize, seg->len);

	PCUT_ASSERT_FALSE(seq_no_segment_acked(conn, seg, 24));
	PCUT_ASSERT_TRUE(seq_no_segment_acked(conn, seg, 25));

	tcp_segment_delete(seg);
	tcp_conn_delete(conn);
	free(data);
}

/** Test seq_no_syn_acked() */
PCUT_TEST(syn_acked)
{
	tcp_conn_t *conn;
	inet_ep2_t epp;

	inet_ep2_init(&epp);
	conn = tcp_conn_new(&epp);
	PCUT_ASSERT_NOT_NULL(conn);

	conn->iss = 1;
	conn->snd_una = 1;
	conn->snd_nxt = 2;

	PCUT_ASSERT_FALSE(seq_no_syn_acked(conn));

	conn->snd_una = 2;
	PCUT_ASSERT_TRUE(seq_no_syn_acked(conn));

	tcp_conn_delete(conn);
}

/** Test seq_no_segment_ready() */
PCUT_TEST(segment_ready)
{
	tcp_conn_t *conn;
	inet_ep2_t epp;
	tcp_segment_t *seg;
	uint8_t *data;
	size_t dsize;

	inet_ep2_init(&epp);
	conn = tcp_conn_new(&epp);
	PCUT_ASSERT_NOT_NULL(conn);

	dsize = 15;
	data = calloc(dsize, 1);
	PCUT_ASSERT_NOT_NULL(data);

	seg = tcp_segment_make_data(0, data, dsize);
	PCUT_ASSERT_NOT_NULL(seg);

	/* Segment must be acceptable. Ready iff intersects RCV.NXT */

	conn->rcv_nxt = 30;
	conn->rcv_wnd = 20;

	PCUT_ASSERT_INT_EQUALS(dsize, seg->len);

	seg->seq = 16;
	PCUT_ASSERT_TRUE(seq_no_segment_ready(conn, seg));

	seg->seq = 17;
	PCUT_ASSERT_TRUE(seq_no_segment_ready(conn, seg));

	seg->seq = 29;
	PCUT_ASSERT_TRUE(seq_no_segment_ready(conn, seg));

	seg->seq = 30;
	PCUT_ASSERT_TRUE(seq_no_segment_ready(conn, seg));

	seg->seq = 31;
	PCUT_ASSERT_FALSE(seq_no_segment_ready(conn, seg));

	tcp_segment_delete(seg);
	tcp_conn_delete(conn);
	free(data);
}

/** Test seq_no_segment_acceptable() */
PCUT_TEST(segment_acceptable)
{
	tcp_conn_t *conn;
	inet_ep2_t epp;
	tcp_segment_t *seg;
	uint8_t *data;
	size_t dsize;

	inet_ep2_init(&epp);
	conn = tcp_conn_new(&epp);
	PCUT_ASSERT_NOT_NULL(conn);

	dsize = 15;
	data = calloc(dsize, 1);
	PCUT_ASSERT_NOT_NULL(data);

	seg = tcp_segment_make_data(0, data, dsize);
	PCUT_ASSERT_NOT_NULL(seg);

	/* Segment acceptable iff overlaps receive window */

	/* Segment shorter than receive window */
	conn->rcv_nxt = 30;
	conn->rcv_wnd = 20;

	PCUT_ASSERT_INT_EQUALS(dsize, seg->len);

	seg->seq = 10;
	PCUT_ASSERT_FALSE(seq_no_segment_acceptable(conn, seg));

	seg->seq = 15;
	PCUT_ASSERT_FALSE(seq_no_segment_acceptable(conn, seg));

	seg->seq = 16;
	PCUT_ASSERT_TRUE(seq_no_segment_acceptable(conn, seg));

	seg->seq = 49;
	PCUT_ASSERT_TRUE(seq_no_segment_acceptable(conn, seg));

	seg->seq = 50;
	PCUT_ASSERT_FALSE(seq_no_segment_acceptable(conn, seg));

	/* Segment longer than receive window */
	conn->rcv_nxt = 30;
	conn->rcv_wnd = 10;

	PCUT_ASSERT_INT_EQUALS(dsize, seg->len);

	seg->seq = 10;
	PCUT_ASSERT_FALSE(seq_no_segment_acceptable(conn, seg));

	seg->seq = 15;
	PCUT_ASSERT_FALSE(seq_no_segment_acceptable(conn, seg));

	seg->seq = 16;
	PCUT_ASSERT_TRUE(seq_no_segment_acceptable(conn, seg));

	seg->seq = 39;
	PCUT_ASSERT_TRUE(seq_no_segment_acceptable(conn, seg));

	seg->seq = 40;
	PCUT_ASSERT_FALSE(seq_no_segment_acceptable(conn, seg));

	tcp_segment_delete(seg);
	tcp_conn_delete(conn);
	free(data);
}

/** Test seq_no_seg_trim_calc() */
PCUT_TEST(seg_trim_calc)
{
	tcp_conn_t *conn;
	inet_ep2_t epp;
	tcp_segment_t *seg;
	uint8_t *data;
	size_t dsize;
	uint32_t ltrim, rtrim;

	inet_ep2_init(&epp);
	conn = tcp_conn_new(&epp);
	PCUT_ASSERT_NOT_NULL(conn);

	dsize = 15;
	data = calloc(dsize, 1);
	PCUT_ASSERT_NOT_NULL(data);

	seg = tcp_segment_make_data(0, data, dsize);
	PCUT_ASSERT_NOT_NULL(seg);

	/* Segment must be acceptable, amount of trim needed */

	/* Segment shorter than receive window */
	conn->rcv_nxt = 30;
	conn->rcv_wnd = 20;

	PCUT_ASSERT_INT_EQUALS(dsize, seg->len);

	seg->seq = 16;
	seq_no_seg_trim_calc(conn, seg, &ltrim, &rtrim);
	PCUT_ASSERT_INT_EQUALS(14, ltrim);
	PCUT_ASSERT_INT_EQUALS(0, rtrim);

	seg->seq = 17;
	seq_no_seg_trim_calc(conn, seg, &ltrim, &rtrim);
	PCUT_ASSERT_INT_EQUALS(13, ltrim);
	PCUT_ASSERT_INT_EQUALS(0, rtrim);

	seg->seq = 29;
	seq_no_seg_trim_calc(conn, seg, &ltrim, &rtrim);
	PCUT_ASSERT_INT_EQUALS(1, ltrim);
	PCUT_ASSERT_INT_EQUALS(0, rtrim);

	seg->seq = 30;
	seq_no_seg_trim_calc(conn, seg, &ltrim, &rtrim);
	PCUT_ASSERT_INT_EQUALS(0, ltrim);
	PCUT_ASSERT_INT_EQUALS(0, rtrim);

	seg->seq = 31;
	seq_no_seg_trim_calc(conn, seg, &ltrim, &rtrim);
	PCUT_ASSERT_INT_EQUALS(0, ltrim);
	PCUT_ASSERT_INT_EQUALS(0, rtrim);

	seg->seq = 35;
	seq_no_seg_trim_calc(conn, seg, &ltrim, &rtrim);
	PCUT_ASSERT_INT_EQUALS(0, ltrim);
	PCUT_ASSERT_INT_EQUALS(0, rtrim);

	seg->seq = 36;
	seq_no_seg_trim_calc(conn, seg, &ltrim, &rtrim);
	PCUT_ASSERT_INT_EQUALS(0, ltrim);
	PCUT_ASSERT_INT_EQUALS(1, rtrim);

	seg->seq = 37;
	seq_no_seg_trim_calc(conn, seg, &ltrim, &rtrim);
	PCUT_ASSERT_INT_EQUALS(0, ltrim);
	PCUT_ASSERT_INT_EQUALS(2, rtrim);

	seg->seq = 48;
	seq_no_seg_trim_calc(conn, seg, &ltrim, &rtrim);
	PCUT_ASSERT_INT_EQUALS(0, ltrim);
	PCUT_ASSERT_INT_EQUALS(13, rtrim);

	seg->seq = 49;
	seq_no_seg_trim_calc(conn, seg, &ltrim, &rtrim);
	PCUT_ASSERT_INT_EQUALS(0, ltrim);
	PCUT_ASSERT_INT_EQUALS(14, rtrim);

	/* Segment longer than receive window */
	conn->rcv_nxt = 30;
	conn->rcv_wnd = 10;

	PCUT_ASSERT_INT_EQUALS(dsize, seg->len);

	seg->seq = 16;
	seq_no_seg_trim_calc(conn, seg, &ltrim, &rtrim);
	PCUT_ASSERT_INT_EQUALS(14, ltrim);
	PCUT_ASSERT_INT_EQUALS(0, rtrim);

	seg->seq = 17;
	seq_no_seg_trim_calc(conn, seg, &ltrim, &rtrim);
	PCUT_ASSERT_INT_EQUALS(13, ltrim);
	PCUT_ASSERT_INT_EQUALS(0, rtrim);

	seg->seq = 24;
	seq_no_seg_trim_calc(conn, seg, &ltrim, &rtrim);
	PCUT_ASSERT_INT_EQUALS(6, ltrim);
	PCUT_ASSERT_INT_EQUALS(0, rtrim);

	seg->seq = 25;
	seq_no_seg_trim_calc(conn, seg, &ltrim, &rtrim);
	PCUT_ASSERT_INT_EQUALS(5, ltrim);
	PCUT_ASSERT_INT_EQUALS(0, rtrim);

	seg->seq = 26;
	seq_no_seg_trim_calc(conn, seg, &ltrim, &rtrim);
	PCUT_ASSERT_INT_EQUALS(4, ltrim);
	PCUT_ASSERT_INT_EQUALS(1, rtrim);

	tcp_segment_delete(seg);
	tcp_conn_delete(conn);
	free(data);
}

/** Test seq_no_seg_cmp() */
PCUT_TEST(seg_cmp)
{
	tcp_conn_t *conn;
	inet_ep2_t epp;
	tcp_segment_t *seg1, *seg2;
	uint8_t *data;
	size_t dsize;

	inet_ep2_init(&epp);
	conn = tcp_conn_new(&epp);
	PCUT_ASSERT_NOT_NULL(conn);

	dsize = 15;
	data = calloc(dsize, 1);
	PCUT_ASSERT_NOT_NULL(data);

	seg1 = tcp_segment_make_data(0, data, dsize);
	PCUT_ASSERT_NOT_NULL(seg1);
	seg2 = tcp_segment_make_data(0, data, dsize);
	PCUT_ASSERT_NOT_NULL(seg2);

	/* Both segments must be acceptable */

	conn->rcv_nxt = 10;
	conn->rcv_wnd = 20;

	PCUT_ASSERT_INT_EQUALS(dsize, seg1->len);
	PCUT_ASSERT_INT_EQUALS(dsize, seg2->len);

	seg1->seq = 5;
	seg2->seq = 6;
	PCUT_ASSERT_INT_EQUALS(-1, seq_no_seg_cmp(conn, seg1, seg2));

	seg1->seq = 6;
	seg2->seq = 6;
	PCUT_ASSERT_INT_EQUALS(0, seq_no_seg_cmp(conn, seg1, seg2));

	seg1->seq = 6;
	seg2->seq = 5;
	PCUT_ASSERT_INT_EQUALS(1, seq_no_seg_cmp(conn, seg1, seg2));

	tcp_segment_delete(seg1);
	tcp_segment_delete(seg2);
	tcp_conn_delete(conn);
	free(data);
}

/** Test seq_no_control_len() */
PCUT_TEST(control_len)
{
	PCUT_ASSERT_INT_EQUALS(0, seq_no_control_len(0));
	PCUT_ASSERT_INT_EQUALS(0, seq_no_control_len(CTL_ACK));
	PCUT_ASSERT_INT_EQUALS(0, seq_no_control_len(CTL_RST));
	PCUT_ASSERT_INT_EQUALS(0, seq_no_control_len(CTL_ACK | CTL_RST));
	PCUT_ASSERT_INT_EQUALS(1, seq_no_control_len(CTL_SYN));
	PCUT_ASSERT_INT_EQUALS(1, seq_no_control_len(CTL_FIN));
	PCUT_ASSERT_INT_EQUALS(1, seq_no_control_len(CTL_SYN | CTL_ACK));
	PCUT_ASSERT_INT_EQUALS(1, seq_no_control_len(CTL_FIN | CTL_ACK));
	PCUT_ASSERT_INT_EQUALS(1, seq_no_control_len(CTL_SYN | CTL_RST));
	PCUT_ASSERT_INT_EQUALS(1, seq_no_control_len(CTL_FIN | CTL_RST));
	PCUT_ASSERT_INT_EQUALS(1, seq_no_control_len(CTL_SYN | CTL_ACK |
	    CTL_RST));
	PCUT_ASSERT_INT_EQUALS(1, seq_no_control_len(CTL_FIN | CTL_ACK |
	    CTL_RST));
	PCUT_ASSERT_INT_EQUALS(2, seq_no_control_len(CTL_SYN | CTL_FIN));
	PCUT_ASSERT_INT_EQUALS(2, seq_no_control_len(CTL_SYN | CTL_FIN |
	    CTL_ACK));
	PCUT_ASSERT_INT_EQUALS(2, seq_no_control_len(CTL_SYN | CTL_FIN |
	    CTL_RST));
	PCUT_ASSERT_INT_EQUALS(2, seq_no_control_len(CTL_SYN | CTL_FIN |
	    CTL_ACK | CTL_RST));
}

PCUT_EXPORT(seq_no);
