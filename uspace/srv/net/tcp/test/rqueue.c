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

#include <adt/prodcons.h>
#include <inet/endpoint.h>
#include <io/log.h>
#include <pcut/pcut.h>

#include "../rqueue.h"
#include "../segment.h"

PCUT_INIT;

PCUT_TEST_SUITE(rqueue);

enum {
	test_seg_max = 10
};

static void test_seg_received(inet_ep2_t *, tcp_segment_t *);

static tcp_rqueue_cb_t rcb = {
	.seg_received = test_seg_received
};

static int seg_cnt;
static tcp_segment_t *recv_seg[test_seg_max];

static void test_seg_received(inet_ep2_t *epp, tcp_segment_t *seg)
{
	recv_seg[seg_cnt++] = seg;
}

PCUT_TEST_BEFORE
{
	errno_t rc;

	/* We will be calling functions that perform logging */
	rc = log_init("test-tcp");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test empty queue */
PCUT_TEST(init_fini)
{
	tcp_rqueue_init(&rcb);
	tcp_rqueue_fibril_start();
	tcp_rqueue_fini();
}

/** Test one segment */
PCUT_TEST(one_segment)
{
	tcp_segment_t *seg;
	inet_ep2_t epp;

	tcp_rqueue_init(&rcb);
	seg_cnt = 0;

	seg = tcp_segment_make_ctrl(CTL_SYN);
	PCUT_ASSERT_NOT_NULL(seg);
	inet_ep2_init(&epp);

	tcp_rqueue_insert_seg(&epp, seg);
	tcp_rqueue_fibril_start();
	tcp_rqueue_fini();

	PCUT_ASSERT_INT_EQUALS(1, seg_cnt);
	PCUT_ASSERT_EQUALS(seg, recv_seg[0]);

	tcp_segment_delete(seg);
}

/** Test multiple segments */
PCUT_TEST(multiple_segments)
{
	tcp_segment_t *seg[test_seg_max];
	inet_ep2_t epp;
	int i;

	tcp_rqueue_init(&rcb);
	seg_cnt = 0;

	inet_ep2_init(&epp);

	tcp_rqueue_fibril_start();

	for (i = 0; i < test_seg_max; i++) {
		seg[i] = tcp_segment_make_ctrl(CTL_ACK);
		PCUT_ASSERT_NOT_NULL(seg[i]);
		tcp_rqueue_insert_seg(&epp, seg[i]);
	}

	tcp_rqueue_fini();

	PCUT_ASSERT_INT_EQUALS(test_seg_max, seg_cnt);
	for (i = 0; i < test_seg_max; i++) {
		PCUT_ASSERT_EQUALS(seg[i], recv_seg[i]);
		tcp_segment_delete(seg[i]);
	}

}

PCUT_EXPORT(rqueue);
