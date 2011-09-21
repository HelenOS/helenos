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

#include <mem.h>
#include <stdlib.h>
#include "segment.h"
#include "seq_no.h"
#include "tcp_type.h"

tcp_segment_t *tcp_segment_new(void)
{
	return calloc(1, sizeof(tcp_segment_t));
}

void tcp_segment_delete(tcp_segment_t *seg)
{
	free(seg);
}

/** Create a control segment. */
tcp_segment_t *tcp_segment_make_ctrl(tcp_control_t ctrl)
{
	tcp_segment_t *seg;

	seg = tcp_segment_new();
	if (seg == NULL)
		return NULL;

	seg->ctrl = ctrl;
	seg->len = seq_no_control_len(ctrl);

	return seg;
}

tcp_segment_t *tcp_segment_make_rst(tcp_segment_t *seg)
{
	tcp_segment_t *rseg;

	rseg = tcp_segment_new();
	if (rseg == NULL)
		return NULL;

	rseg->ctrl = CTL_RST;
	rseg->seq = seg->ack;

	return rseg;
}

/** Trim segment to the specified interval.
 *
 * Trim any text or control whose sequence number is outside of [lo, hi)
 * interval.
 */
void tcp_segment_trim(tcp_segment_t *seg, uint32_t left, uint32_t right)
{
	uint32_t t_size;

	assert(left + right <= seg->len);

	/* Special case, entire segment trimmed from left */
	if (left == seg->len) {
		seg->seq = seg->seq + seg->len;
		seg->len = 0;
		return;
	}

	/* Special case, entire segment trimmed from right */
	if (right == seg->len) {
		seg->len = 0;
		return;
	}

	/* General case */

	t_size = tcp_segment_text_size(seg);

	if (left > 0 && (seg->ctrl & CTL_SYN) != 0) {
		/* Trim SYN */
		seg->ctrl &= ~CTL_SYN;
		seg->seq++;
		seg->len--;
		left--;
	}

	if (right > 0 && (seg->ctrl & CTL_FIN) != 0) {
		/* Trim FIN */
		seg->ctrl &= ~CTL_FIN;
		seg->len--;
		right--;
	}

	if (left > 0 || right > 0) {
		/* Trim segment text */
		assert(left + right <= t_size);

		seg->data += left;
		seg->len -= left + right;
	}
}

/** Copy out text data from segment.
 *
 */
void tcp_segment_text_copy(tcp_segment_t *seg, void *buf, size_t size)
{
	assert(size <= tcp_segment_text_size(seg));
	memcpy(buf, seg->data, size);
}

/** Return number of bytes in segment text. */
size_t tcp_segment_text_size(tcp_segment_t *seg)
{
	return seg->len - seq_no_control_len(seg->ctrl);
}

/**
 * @}
 */
