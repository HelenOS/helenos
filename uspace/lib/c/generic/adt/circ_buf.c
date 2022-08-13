/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */

/** @file Circular buffer
 */

#include <adt/circ_buf.h>
#include <errno.h>
#include <mem.h>
#include <stddef.h>

/** Initialize circular buffer.
 *
 * @param cbuf Circular buffer
 * @param buf Buffer for storing data
 * @param nmemb Number of entries in @a buf
 * @param size Size of individual buffer entry
 */
void circ_buf_init(circ_buf_t *cbuf, void *buf, size_t nmemb, size_t size)
{
	cbuf->buf = buf;
	cbuf->nmemb = nmemb;
	cbuf->size = size;
	cbuf->rp = 0;
	cbuf->wp = 0;
	cbuf->nused = 0;
}

/** Return number of free buffer entries.
 *
 * @param cbuf Circular buffer
 * @return Number of free buffer entries
 */
size_t circ_buf_nfree(circ_buf_t *cbuf)
{
	return cbuf->nmemb - cbuf->nused;
}

/** Return number of used buffer entries.
 *
 * @param cbuf Circular buffer
 * @return Number of used buffer entries
 */
size_t circ_buf_nused(circ_buf_t *cbuf)
{
	return cbuf->nused;
}

/** Push new entry into circular buffer.
 *
 * @param cbuf Circular buffer
 * @param data Pointer to entry data
 * @return EOK on success, EAGAIN if circular buffer is full
 */
errno_t circ_buf_push(circ_buf_t *cbuf, const void *data)
{
	if (circ_buf_nfree(cbuf) == 0)
		return EAGAIN;

	memcpy(cbuf->buf + cbuf->size * cbuf->wp, data, cbuf->size);
	cbuf->wp = (cbuf->wp + 1) % cbuf->nmemb;
	cbuf->nused++;
	return EOK;
}

/** Pop entry from circular buffer.
 *
 * @param cbuf Circular buffer
 * @param datab Pointer to data buffer for storing entry
 * @return EOK on success, EAGAIN if circular buffer is full
 */
errno_t circ_buf_pop(circ_buf_t *cbuf, void *datab)
{
	if (cbuf->nused == 0)
		return EAGAIN;

	memcpy(datab, cbuf->buf + cbuf->size * cbuf->rp, cbuf->size);
	cbuf->rp = (cbuf->rp + 1) % cbuf->nmemb;
	cbuf->nused--;
	return EOK;
}

/** @}
 */
