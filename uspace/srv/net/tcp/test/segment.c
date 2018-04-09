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

#include <pcut/pcut.h>

#include "main.h"
#include "../segment.h"

PCUT_INIT;

PCUT_TEST_SUITE(segment);

/** Test create/destroy control segment */
PCUT_TEST(ctrl_seg_mkdel)
{
	tcp_segment_t *seg;

	seg = tcp_segment_make_ctrl(CTL_SYN);
	PCUT_ASSERT_NOT_NULL(seg);
	tcp_segment_delete(seg);
}

/** Test create/destroy data segment */
PCUT_TEST(data_seg_mkdel)
{
	tcp_segment_t *seg;
	uint8_t *data;
	size_t i, dsize;

	dsize = 15;
	data = malloc(dsize);
	PCUT_ASSERT_NOT_NULL(data);

	for (i = 0; i < dsize; i++)
		data[i] = (uint8_t) i;

	seg = tcp_segment_make_data(CTL_SYN, data, dsize);
	PCUT_ASSERT_NOT_NULL(seg);

	tcp_segment_delete(seg);
	free(data);
}

/** Test create/duplicate/destroy control segment */
PCUT_TEST(ctrl_seg_dup)
{
	tcp_segment_t *seg, *dup;

	seg = tcp_segment_make_ctrl(CTL_SYN);
	PCUT_ASSERT_NOT_NULL(seg);

	seg->seq = 20;
	seg->ack = 19;
	seg->wnd = 18;
	seg->up = 17;

	dup = tcp_segment_dup(seg);
	test_seg_same(seg, dup);

	tcp_segment_delete(seg);
	tcp_segment_delete(dup);
}

/** Test create/duplicate/destroy data segment */
PCUT_TEST(data_seg_dup)
{
	tcp_segment_t *seg, *dup;
	uint8_t *data;
	size_t i, dsize;

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

	dup = tcp_segment_dup(seg);
	test_seg_same(seg, dup);

	tcp_segment_delete(seg);
	tcp_segment_delete(dup);
	free(data);
}

/** Test reset segment for segment with ACK not set */
PCUT_TEST(noack_seg_rst)
{
	tcp_segment_t *seg, *rst;

	seg = tcp_segment_make_ctrl(CTL_SYN);
	PCUT_ASSERT_NOT_NULL(seg);

	seg->seq = 20;
	seg->ack = 19;
	seg->wnd = 18;
	seg->up = 17;

	rst = tcp_segment_make_rst(seg);
	PCUT_ASSERT_NOT_NULL(seg);

	tcp_segment_delete(seg);
	tcp_segment_delete(rst);
}

/** Test reset segment for segment with ACK set */
PCUT_TEST(ack_seg_rst)
{
	tcp_segment_t *seg, *rst;

	seg = tcp_segment_make_ctrl(CTL_SYN | CTL_ACK);
	PCUT_ASSERT_NOT_NULL(seg);

	seg->seq = 20;
	seg->ack = 19;
	seg->wnd = 18;
	seg->up = 17;

	rst = tcp_segment_make_rst(seg);
	PCUT_ASSERT_NOT_NULL(seg);

	tcp_segment_delete(seg);
	tcp_segment_delete(rst);
}

/** Test copying out data segment text */
PCUT_TEST(data_seg_text)
{
	tcp_segment_t *seg;
	uint8_t *data;
	uint8_t *cdata;
	size_t i, dsize;

	dsize = 15;
	data = malloc(dsize);
	PCUT_ASSERT_NOT_NULL(data);
	cdata = malloc(dsize);
	PCUT_ASSERT_NOT_NULL(cdata);

	for (i = 0; i < dsize; i++)
		data[i] = (uint8_t) i;

	seg = tcp_segment_make_data(CTL_SYN, data, dsize);
	PCUT_ASSERT_NOT_NULL(seg);

	PCUT_ASSERT_INT_EQUALS(dsize, tcp_segment_text_size(seg));
	tcp_segment_text_copy(seg, cdata, dsize);

	for (i = 0; i < dsize; i++)
		PCUT_ASSERT_INT_EQUALS(data[i], cdata[i]);

	tcp_segment_delete(seg);
	free(data);
	free(cdata);
}

/** Test trimming data segment text */
PCUT_TEST(data_seg_trim)
{
	tcp_segment_t *seg;
	uint8_t *data;
	uint8_t *cdata;
	size_t i, dsize;

	dsize = 15;
	data = malloc(dsize);
	PCUT_ASSERT_NOT_NULL(data);
	cdata = malloc(dsize);
	PCUT_ASSERT_NOT_NULL(cdata);

	for (i = 0; i < dsize; i++)
		data[i] = (uint8_t) i;

	seg = tcp_segment_make_data(CTL_SYN | CTL_FIN, data, dsize);
	PCUT_ASSERT_NOT_NULL(seg);

	PCUT_ASSERT_INT_EQUALS(dsize, tcp_segment_text_size(seg));
	tcp_segment_text_copy(seg, cdata, dsize);

	for (i = 0; i < dsize; i++)
		PCUT_ASSERT_INT_EQUALS(data[i], cdata[i]);

	PCUT_ASSERT_INT_EQUALS(CTL_SYN | CTL_FIN, seg->ctrl);

	tcp_segment_trim(seg, 1, 0);
	PCUT_ASSERT_INT_EQUALS(CTL_FIN, seg->ctrl);
	PCUT_ASSERT_INT_EQUALS(dsize, tcp_segment_text_size(seg));

	tcp_segment_trim(seg, 0, 1);
	PCUT_ASSERT_INT_EQUALS(0, seg->ctrl);
	PCUT_ASSERT_INT_EQUALS(dsize, tcp_segment_text_size(seg));

	tcp_segment_trim(seg, 1, 0);
	PCUT_ASSERT_INT_EQUALS(0, seg->ctrl);
	PCUT_ASSERT_INT_EQUALS(dsize - 1, tcp_segment_text_size(seg));

	tcp_segment_text_copy(seg, cdata, dsize - 1);
	for (i = 0; i < dsize - 1; i++)
		PCUT_ASSERT_INT_EQUALS(data[i + 1], cdata[i]);

	tcp_segment_trim(seg, 0, 1);
	PCUT_ASSERT_INT_EQUALS(0, seg->ctrl);
	PCUT_ASSERT_INT_EQUALS(dsize - 2, tcp_segment_text_size(seg));

	tcp_segment_text_copy(seg, cdata, dsize - 2);
	for (i = 0; i < dsize - 2; i++)
		PCUT_ASSERT_INT_EQUALS(data[i + 1], cdata[i]);

	tcp_segment_delete(seg);
	free(data);
	free(cdata);
}

PCUT_EXPORT(segment);
