/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

//#include <inet/endpoint.h>
#include <io/log.h>
#include <pcut/pcut.h>

#include "../conn.h"
#include "../segment.h"
#include "../tqueue.h"

PCUT_INIT;

PCUT_TEST_SUITE(tqueue);

enum {
	test_seg_max = 10
};

static int seg_cnt;
static tcp_segment_t *trans_seg[test_seg_max];

static void tqueue_test_transmit_seg(inet_ep2_t *, tcp_segment_t *);

static tcp_tqueue_cb_t tqueue_test_cb = {
	.transmit_seg = tqueue_test_transmit_seg
};

PCUT_TEST_BEFORE
{
	errno_t rc;

	/* We will be calling functions that perform logging */
	rc = log_init("test-tcp");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = tcp_conns_init();
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

PCUT_TEST_AFTER
{
	tcp_conns_fini();
}

/** Test  */
PCUT_TEST(init_fini)
{
	tcp_conn_t *conn;
	inet_ep2_t epp;

	/* XXX tqueue can only be created via tcp_conn_new */
	inet_ep2_init(&epp);
	conn = tcp_conn_new(&epp);
	PCUT_ASSERT_NOT_NULL(conn);

	/* Redirect segment transmission */
	conn->retransmit.cb = &tqueue_test_cb;
	seg_cnt = 0;

	tcp_conn_lock(conn);
	tcp_conn_reset(conn);
	tcp_conn_unlock(conn);
	tcp_conn_delete(conn);
	PCUT_ASSERT_EQUALS(0, seg_cnt);
}

/** Test sending control segment and tearing down a non-empty queue */
PCUT_TEST(ctrl_seg_teardown)
{
	tcp_conn_t *conn;
	inet_ep2_t epp;

	/* XXX tqueue can only be created via tcp_conn_new */
	inet_ep2_init(&epp);
	conn = tcp_conn_new(&epp);
	PCUT_ASSERT_NOT_NULL(conn);

	conn->snd_nxt = 10;

	/* Redirect segment transmission */
	conn->retransmit.cb = &tqueue_test_cb;
	seg_cnt = 0;

	tcp_conn_lock(conn);
	tcp_tqueue_ctrl_seg(conn, CTL_SYN);
	tcp_conn_reset(conn);
	tcp_conn_unlock(conn);
	PCUT_ASSERT_EQUALS(11, conn->snd_nxt);

	tcp_conn_delete(conn);
	PCUT_ASSERT_EQUALS(1, seg_cnt);
	PCUT_ASSERT_EQUALS(CTL_SYN, trans_seg[0]->ctrl);
	PCUT_ASSERT_EQUALS(10, trans_seg[0]->seq);
	tcp_segment_delete(trans_seg[0]);
}

/** Test sending data and FIN */
PCUT_TEST(new_data_fin)
{
	tcp_conn_t *conn;
	inet_ep2_t epp;
	int i;

	/* XXX tqueue can only be created via tcp_conn_new */
	inet_ep2_init(&epp);
	conn = tcp_conn_new(&epp);
	PCUT_ASSERT_NOT_NULL(conn);

	conn->cstate = st_established;
	conn->snd_una = 10;
	conn->snd_nxt = 10;
	conn->snd_wnd = 1024;
	conn->snd_buf_used = 20;
	conn->snd_buf_fin = true;
	for (i = 0; i < 20; i++)
		conn->snd_buf[i] = i;

	/* Redirect segment transmission */
	conn->retransmit.cb = &tqueue_test_cb;
	seg_cnt = 0;

	tcp_conn_lock(conn);
	tcp_tqueue_new_data(conn);
	tcp_conn_reset(conn);
	tcp_conn_unlock(conn);
	PCUT_ASSERT_EQUALS(31, conn->snd_nxt);
	PCUT_ASSERT_EQUALS(0, conn->snd_buf_used);
	PCUT_ASSERT_FALSE(conn->snd_buf_fin);

	tcp_conn_delete(conn);
	PCUT_ASSERT_EQUALS(1, seg_cnt);
	PCUT_ASSERT_EQUALS(CTL_FIN | CTL_ACK, trans_seg[0]->ctrl);
	PCUT_ASSERT_EQUALS(10, trans_seg[0]->seq);
	tcp_segment_delete(trans_seg[0]);
}

/** Test sending data when send window is smaller */
PCUT_TEST(new_data_small_win)
{
	tcp_conn_t *conn;
	inet_ep2_t epp;
	int i;

	/* XXX tqueue can only be created via tcp_conn_new */
	inet_ep2_init(&epp);
	conn = tcp_conn_new(&epp);
	PCUT_ASSERT_NOT_NULL(conn);

	conn->cstate = st_established;
	conn->snd_una = 10;
	conn->snd_nxt = 10;
	conn->snd_wnd = 5;
	conn->snd_buf_used = 30;
	conn->snd_buf_fin = false;
	for (i = 0; i < 30; i++)
		conn->snd_buf[i] = i;

	/* Redirect segment transmission */
	conn->retransmit.cb = &tqueue_test_cb;
	seg_cnt = 0;

	tcp_conn_lock(conn);
	tcp_tqueue_new_data(conn);
	tcp_conn_reset(conn);
	tcp_conn_unlock(conn);

	PCUT_ASSERT_EQUALS(15, conn->snd_nxt);
	PCUT_ASSERT_EQUALS(25, conn->snd_buf_used);
	PCUT_ASSERT_FALSE(conn->snd_buf_fin);
	for (i = 0; i < 25; i++)
		PCUT_ASSERT_INT_EQUALS(5 + i, conn->snd_buf[i]);

	tcp_conn_delete(conn);
	PCUT_ASSERT_EQUALS(1, seg_cnt);
	PCUT_ASSERT_EQUALS(CTL_ACK, trans_seg[0]->ctrl);
	PCUT_ASSERT_EQUALS(10, trans_seg[0]->seq);
	tcp_segment_delete(trans_seg[0]);
}

/** Test flushing tqueue due to receiving an ACK */
PCUT_TEST(ack_received)
{
	tcp_conn_t *conn;
	inet_ep2_t epp;
	int i;

	/* XXX tqueue can only be created via tcp_conn_new */
	inet_ep2_init(&epp);
	conn = tcp_conn_new(&epp);
	PCUT_ASSERT_NOT_NULL(conn);

	conn->cstate = st_established;
	conn->snd_una = 10;
	conn->snd_nxt = 10;
	conn->snd_wnd = 1024;

	/* Redirect segment transmission */
	conn->retransmit.cb = &tqueue_test_cb;
	seg_cnt = 0;

	tcp_conn_lock(conn);

	/* Queue first data segment */
	conn->snd_buf_used = 10;
	conn->snd_buf_fin = false;
	for (i = 0; i < 10; i++)
		conn->snd_buf[i] = i;
	tcp_tqueue_new_data(conn);

	PCUT_ASSERT_EQUALS(20, conn->snd_nxt);

	/* Queue second data segment */
	conn->snd_buf_used = 20;
	conn->snd_buf_fin = false;
	for (i = 0; i < 20; i++)
		conn->snd_buf[i] = i;
	tcp_tqueue_new_data(conn);

	PCUT_ASSERT_EQUALS(40, conn->snd_nxt);

	PCUT_ASSERT_INT_EQUALS(2, list_count(&conn->retransmit.list));

	/* One of the two segments is acked */
	conn->snd_una = 20;
	tcp_tqueue_ack_received(conn);

	PCUT_ASSERT_INT_EQUALS(1, list_count(&conn->retransmit.list));

	tcp_conn_reset(conn);
	tcp_conn_unlock(conn);
	tcp_conn_delete(conn);
}

static void tqueue_test_transmit_seg(inet_ep2_t *epp, tcp_segment_t *seg)
{
	trans_seg[seg_cnt++] = tcp_segment_dup(seg);
}

PCUT_EXPORT(tqueue);
