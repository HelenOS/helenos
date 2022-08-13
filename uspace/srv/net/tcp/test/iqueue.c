/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <inet/endpoint.h>
#include <pcut/pcut.h>

#include "../conn.h"
#include "../iqueue.h"
#include "../segment.h"

PCUT_INIT;

PCUT_TEST_SUITE(iqueue);

/** Test empty queue */
PCUT_TEST(empty_queue)
{
	tcp_conn_t *conn;
	tcp_iqueue_t iqueue;
	inet_ep2_t epp;
	tcp_segment_t *rseg;
	errno_t rc;

	inet_ep2_init(&epp);
	conn = tcp_conn_new(&epp);
	PCUT_ASSERT_NOT_NULL(conn);

	conn->rcv_nxt = 10;
	conn->rcv_wnd = 20;

	tcp_iqueue_init(&iqueue, conn);
	rc = tcp_iqueue_get_ready_seg(&iqueue, &rseg);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	tcp_conn_delete(conn);
}

/** Test iqeue with one segment */
PCUT_TEST(one_segment)
{
	tcp_conn_t *conn;
	tcp_iqueue_t iqueue;
	inet_ep2_t epp;
	tcp_segment_t *rseg;
	tcp_segment_t *seg;
	void *data;
	size_t dsize;
	errno_t rc;

	inet_ep2_init(&epp);
	conn = tcp_conn_new(&epp);
	PCUT_ASSERT_NOT_NULL(conn);

	conn->rcv_nxt = 10;
	conn->rcv_wnd = 20;

	dsize = 15;
	data = calloc(dsize, 1);
	PCUT_ASSERT_NOT_NULL(data);

	seg = tcp_segment_make_data(0, data, dsize);
	PCUT_ASSERT_NOT_NULL(seg);

	tcp_iqueue_init(&iqueue, conn);
	rc = tcp_iqueue_get_ready_seg(&iqueue, &rseg);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	seg->seq = 10;
	tcp_iqueue_insert_seg(&iqueue, seg);
	rc = tcp_iqueue_get_ready_seg(&iqueue, &rseg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	seg->seq = 15;
	tcp_iqueue_insert_seg(&iqueue, seg);
	rc = tcp_iqueue_get_ready_seg(&iqueue, &rseg);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);
	tcp_iqueue_remove_seg(&iqueue, seg);

	tcp_segment_delete(seg);
	free(data);
	tcp_conn_delete(conn);
}

/** Test iqeue with two segments */
PCUT_TEST(two_segments)
{
	tcp_conn_t *conn;
	tcp_iqueue_t iqueue;
	inet_ep2_t epp;
	tcp_segment_t *rseg;
	tcp_segment_t *seg1, *seg2;
	void *data;
	size_t dsize;
	errno_t rc;

	inet_ep2_init(&epp);
	conn = tcp_conn_new(&epp);
	PCUT_ASSERT_NOT_NULL(conn);

	conn->rcv_nxt = 10;
	conn->rcv_wnd = 20;

	dsize = 15;
	data = calloc(dsize, 1);
	PCUT_ASSERT_NOT_NULL(data);

	seg1 = tcp_segment_make_data(0, data, dsize);
	PCUT_ASSERT_NOT_NULL(seg1);
	seg2 = tcp_segment_make_data(0, data, dsize);
	PCUT_ASSERT_NOT_NULL(seg2);

	tcp_iqueue_init(&iqueue, conn);
	rc = tcp_iqueue_get_ready_seg(&iqueue, &rseg);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	/* Test reception in ascending order */
	seg1->seq = 5;
	tcp_iqueue_insert_seg(&iqueue, seg1);
	seg2->seq = 10;
	tcp_iqueue_insert_seg(&iqueue, seg2);

	rc = tcp_iqueue_get_ready_seg(&iqueue, &rseg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(rseg == seg1);

	rc = tcp_iqueue_get_ready_seg(&iqueue, &rseg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(rseg == seg2);

	/* Test reception in descending order */
	seg1->seq = 10;
	tcp_iqueue_insert_seg(&iqueue, seg1);
	seg2->seq = 5;
	tcp_iqueue_insert_seg(&iqueue, seg2);

	rc = tcp_iqueue_get_ready_seg(&iqueue, &rseg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(seg2, rseg);

	rc = tcp_iqueue_get_ready_seg(&iqueue, &rseg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(seg1, rseg);

	rc = tcp_iqueue_get_ready_seg(&iqueue, &rseg);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	tcp_segment_delete(seg1);
	tcp_segment_delete(seg2);
	free(data);
	tcp_conn_delete(conn);
}

PCUT_EXPORT(iqueue);
