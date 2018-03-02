/*
 * Copyright (c) 2010 Lenka Trochtova
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

/** @addtogroup ns8250
 * @{
 */
/** @file
 */

#ifndef CYCLIC_BUFFER_H_
#define CYCLIC_BUFFER_H_

#define BUF_LEN 4096

typedef struct cyclic_buffer {
	uint8_t buf[BUF_LEN];
	int start;
	int cnt;
}  cyclic_buffer_t;

/*
 * @return		False if the buffer is full.
 */
static inline bool buf_push_back(cyclic_buffer_t *buf, uint8_t item)
{
	if (buf->cnt >= BUF_LEN)
		return false;
	int pos = (buf->start + buf->cnt) % BUF_LEN;
	buf->buf[pos] = item;
	buf->cnt++;
	return true;
}

static inline bool buf_is_empty(cyclic_buffer_t *buf)
{
	return buf->cnt == 0;
}

static inline uint8_t buf_pop_front(cyclic_buffer_t *buf)
{
	assert(!buf_is_empty(buf));

	uint8_t res = buf->buf[buf->start];
	buf->start = (buf->start + 1) % BUF_LEN;
	buf->cnt--;
	return res;
}

static inline void buf_clear(cyclic_buffer_t *buf)
{
	buf->cnt = 0;
}

#endif

/**
 * @}
 */
