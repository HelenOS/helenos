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
 * @file Segment processing
 */

#include <io/log.h>
#include <mem.h>
#include <stdlib.h>
#include "segment.h"
#include "seq_no.h"
#include "tcp_type.h"

/** Alocate new segment structure. */
static tcp_segment_t *tcp_segment_new(void)
{
	return calloc(1, sizeof(tcp_segment_t));
}

/** Delete segment. */
void tcp_segment_delete(tcp_segment_t *seg)
{
	free(seg->dfptr);
	free(seg);
}

/** Create duplicate of segment.
 *
 * @param seg	Segment
 * @return 	Duplicate segment
 */
tcp_segment_t *tcp_segment_dup(tcp_segment_t *seg)
{
	tcp_segment_t *scopy;
	size_t tsize;

	scopy = tcp_segment_new();
	if (scopy == NULL)
		return NULL;

	scopy->ctrl = seg->ctrl;
	scopy->seq = seg->seq;
	scopy->ack = seg->ack;
	scopy->len = seg->len;
	scopy->wnd = seg->wnd;
	scopy->up = seg->up;

	tsize = tcp_segment_text_size(seg);
	scopy->data = calloc(tsize, 1);
	if (scopy->data == NULL) {
		free(scopy);
		return NULL;
	}

	memcpy(scopy->data, seg->data, tsize);
	scopy->dfptr = scopy->data;

	return scopy;
}

/** Create a control-only segment.
 *
 * @return	Segment
 */
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

/** Create an RST segment.
 *
 * @param seg	Segment we are replying to
 * @return	RST segment
 */
tcp_segment_t *tcp_segment_make_rst(tcp_segment_t *seg)
{
	tcp_segment_t *rseg;

	rseg = tcp_segment_new();
	if (rseg == NULL)
		return NULL;

	if ((seg->ctrl & CTL_ACK) == 0) {
		rseg->ctrl = CTL_RST | CTL_ACK;
		rseg->seq = 0;
		rseg->ack = seg->seq + seg->len;
	} else {
		rseg->ctrl = CTL_RST;
		rseg->seq = seg->ack;
	}

	return rseg;
}

/** Create a control segment.
 *
 * @return	Segment
 */
tcp_segment_t *tcp_segment_make_data(tcp_control_t ctrl, void *data,
    size_t size)
{
	tcp_segment_t *seg;

	seg = tcp_segment_new();
	if (seg == NULL)
		return NULL;

	seg->ctrl = ctrl;
	seg->len = seq_no_control_len(ctrl) + size;

	seg->dfptr = seg->data = malloc(size);
	if (seg->dfptr == NULL) {
		free(seg);
		return NULL;
	}

	memcpy(seg->data, data, size);

	return seg;
}

/** Trim segment from left and right by the specified amount.
 *
 * Trim any text or control to remove the specified amount of sequence
 * numbers from the left (lower sequence numbers) and right side
 * (higher sequence numbers) of the segment.
 *
 * @param seg		Segment, will be modified in place
 * @param left		Amount of sequence numbers to trim from the left
 * @param right		Amount of sequence numbers to trim from the right
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
 * Data is copied from the beginning of the segment text up to @a size bytes.
 * @a size must not be greater than the size of the segment text, but
 * it can be less.
 *
 * @param seg	Segment
 * @param buf	Destination buffer
 * @param size	Size of destination buffer
 */
void tcp_segment_text_copy(tcp_segment_t *seg, void *buf, size_t size)
{
	assert(size <= tcp_segment_text_size(seg));
	memcpy(buf, seg->data, size);
}

/** Return number of bytes in segment text.
 *
 * @param seg	Segment
 * @return	Number of bytes in segment text
 */
size_t tcp_segment_text_size(tcp_segment_t *seg)
{
	return seg->len - seq_no_control_len(seg->ctrl);
}

/** Dump segment contents to log.
 *
 * @param seg	Segment
 */
void tcp_segment_dump(tcp_segment_t *seg)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG2, "Segment dump:");
	log_msg(LOG_DEFAULT, LVL_DEBUG2, " - ctrl = %u", (unsigned)seg->ctrl);
	log_msg(LOG_DEFAULT, LVL_DEBUG2, " - seq = %" PRIu32, seg->seq);
	log_msg(LOG_DEFAULT, LVL_DEBUG2, " - ack = %" PRIu32, seg->ack);
	log_msg(LOG_DEFAULT, LVL_DEBUG2, " - len = %" PRIu32, seg->len);
	log_msg(LOG_DEFAULT, LVL_DEBUG2, " - wnd = %" PRIu32, seg->wnd);
	log_msg(LOG_DEFAULT, LVL_DEBUG2, " - up = %" PRIu32, seg->up);
}

/**
 * @}
 */
