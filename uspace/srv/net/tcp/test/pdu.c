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
