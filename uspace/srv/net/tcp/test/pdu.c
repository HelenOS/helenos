/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <inet/endpoint.h>
#include <mem.h>
#include <pcut/pcut.h>
#include <stdlib.h>

#include "main.h"
#include "../pdu.h"
#include "../segment.h"

PCUT_INIT;

PCUT_TEST_SUITE(pdu);

/** Test encode/decode round trip for control PDU */
PCUT_TEST(encdec_syn)
{
	tcp_segment_t *seg, *dseg;
	tcp_pdu_t *pdu;
	inet_ep2_t epp, depp;
	errno_t rc;

	inet_ep2_init(&epp);
	inet_addr(&epp.local.addr, 1, 2, 3, 4);
	inet_addr(&epp.remote.addr, 5, 6, 7, 8);

	seg = tcp_segment_make_ctrl(CTL_SYN);
	PCUT_ASSERT_NOT_NULL(seg);

	seg->seq = 20;
	seg->ack = 19;
	seg->wnd = 18;
	seg->up = 17;

	rc = tcp_pdu_encode(&epp, seg, &pdu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	rc = tcp_pdu_decode(pdu, &depp, &dseg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	test_seg_same(seg, dseg);
	tcp_segment_delete(seg);
}

/** Test encode/decode round trip for data PDU */
PCUT_TEST(encdec_data)
{
	tcp_segment_t *seg, *dseg;
	tcp_pdu_t *pdu;
	inet_ep2_t epp, depp;
	uint8_t *data;
	size_t i, dsize;
	errno_t rc;

	inet_ep2_init(&epp);
	inet_addr(&epp.local.addr, 1, 2, 3, 4);
	inet_addr(&epp.remote.addr, 5, 6, 7, 8);

	dsize = 15;
	data = malloc(dsize);
	PCUT_ASSERT_NOT_NULL(data);

	for (i = 0; i < dsize; i++)
		data[i] = (uint8_t) i;

	seg = tcp_segment_make_data(CTL_SYN, data, dsize);
	PCUT_ASSERT_NOT_NULL(seg);

	seg->seq = 20;
	seg->ack = 19;
	seg->wnd = 18;
	seg->up = 17;

	rc = tcp_pdu_encode(&epp, seg, &pdu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	rc = tcp_pdu_decode(pdu, &depp, &dseg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	test_seg_same(seg, dseg);
	tcp_segment_delete(seg);
	free(data);
}

PCUT_EXPORT(pdu);
