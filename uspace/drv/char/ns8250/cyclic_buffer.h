/*
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
