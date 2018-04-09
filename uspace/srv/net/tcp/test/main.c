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
