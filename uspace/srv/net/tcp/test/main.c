/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mem.h>
#include <pcut/pcut.h>

#include "main.h"
#include "../segment.h"
#include "../tcp_type.h"

/** Verify that two segments have the same content */
void test_seg_same(tcp_segment_t *a, tcp_segment_t *b)
{
	PCUT_ASSERT_INT_EQUALS(a->ctrl, b->ctrl);
	PCUT_ASSERT_INT_EQUALS(a->seq, b->seq);
	PCUT_ASSERT_INT_EQUALS(a->ack, b->ack);
	PCUT_ASSERT_INT_EQUALS(a->len, b->len);
	PCUT_ASSERT_INT_EQUALS(a->wnd, b->wnd);
	PCUT_ASSERT_INT_EQUALS(a->up, b->up);
	PCUT_ASSERT_INT_EQUALS(tcp_segment_text_size(a),
	    tcp_segment_text_size(b));
	if (tcp_segment_text_size(a) != 0)
		PCUT_ASSERT_NOT_NULL(a->data);
	if (tcp_segment_text_size(b) != 0)
		PCUT_ASSERT_NOT_NULL(b->data);
	if (tcp_segment_text_size(a) != 0) {
		PCUT_ASSERT_INT_EQUALS(0, memcmp(a->data, b->data,
		    tcp_segment_text_size(a)));
	}
}

PCUT_INIT;

PCUT_IMPORT(conn);
PCUT_IMPORT(iqueue);
PCUT_IMPORT(pdu);
PCUT_IMPORT(rqueue);
PCUT_IMPORT(segment);
PCUT_IMPORT(seq_no);
PCUT_IMPORT(tqueue);
PCUT_IMPORT(ucall);

PCUT_MAIN();
