/*
 * Copyright (c) 2009 Lukas Mejdrech
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

/** @addtogroup libc
 * @{
 */

/** @file
 * Dynamic first in first out positive integer queue implementation.
 */

#include <adt/dynamic_fifo.h>

#include <errno.h>
#include <malloc.h>
#include <mem.h>

/** Internal magic value for a consistency check. */
#define DYN_FIFO_MAGIC_VALUE	0x58627659

/** Returns the next queue index.
 *
 * The queue field is circular.
 *
 * @param[in] fifo	The dynamic queue.
 * @param[in] index	The actual index to be shifted.
 */
#define NEXT_INDEX(fifo, index)	(((index) + 1) % ((fifo)->size + 1))

/** Checks if the queue is valid.
 *
 * @param[in] fifo	The dynamic queue.
 * @return		TRUE if the queue is valid.
 * @return		FALSE otherwise.
 */
static int dyn_fifo_is_valid(dyn_fifo_t *fifo)
{
	return fifo && (fifo->magic_value == DYN_FIFO_MAGIC_VALUE);
}

/** Initializes the dynamic queue.
 *
 * @param[in,out] fifo	The dynamic queue.
 * @param[in] size	The initial queue size.
 * @return		EOK on success.
 * @return		EINVAL if the queue is not valid.
 * @return		EBADMEM if the fifo parameter is NULL.
 * @return		ENOMEM if there is not enough memory left.
 */
int dyn_fifo_initialize(dyn_fifo_t *fifo, int size)
{
	if (!fifo)
		return EBADMEM;

	if (size <= 0)
		return EINVAL;

	fifo->items = (int *) malloc(sizeof(int) * size + 1);
	if (!fifo->items)
		return ENOMEM;

	fifo->size = size;
	fifo->head = 0;
	fifo->tail = 0;
	fifo->magic_value = DYN_FIFO_MAGIC_VALUE;

	return EOK;
}

/** Appends a new item to the queue end.
 *
 * @param[in,out] fifo	The dynamic queue.
 * @param[in] value	The new item value. Should be positive.
 * @param[in] max_size	The maximum queue size. The queue is not resized beyound
 *			this limit. May be zero or negative to indicate no
 *			limit.
 * @return		EOK on success.
 * @return		EINVAL if the queue is not valid.
 * @return		ENOMEM if there is not enough memory left.
 */
int dyn_fifo_push(dyn_fifo_t *fifo, int value, int max_size)
{
	int *new_items;

	if (!dyn_fifo_is_valid(fifo))
		return EINVAL;

	if (NEXT_INDEX(fifo, fifo->tail) == fifo->head) {
		if ((max_size > 0) && ((fifo->size * 2) > max_size)) {
			if(fifo->size >= max_size)
				return ENOMEM;
		} else {
			max_size = fifo->size * 2;
		}

		new_items = realloc(fifo->items, sizeof(int) * max_size + 1);
		if (!new_items)
			return ENOMEM;

		fifo->items = new_items;
		if (fifo->tail < fifo->head) {
			if (fifo->tail < max_size - fifo->size) {
				memcpy(fifo->items + fifo->size + 1,
				    fifo->items, fifo->tail * sizeof(int));
				fifo->tail += fifo->size + 1;
			} else {
				memcpy(fifo->items + fifo->size + 1,
				    fifo->items,
				    (max_size - fifo->size) * sizeof(int));
				memcpy(fifo->items,
				    fifo->items + max_size - fifo->size,
				    fifo->tail - max_size + fifo->size);
				fifo->tail -= max_size - fifo->size;
			}
		}
		fifo->size = max_size;
	}

	fifo->items[fifo->tail] = value;
	fifo->tail = NEXT_INDEX(fifo, fifo->tail);
	return EOK;
}

/** Returns and excludes the first item in the queue.
 *
 * @param[in,out] fifo	The dynamic queue.
 * @return		Value of the first item in the queue.
 * @return		EINVAL if the queue is not valid.
 * @return		ENOENT if the queue is empty.
 */
int dyn_fifo_pop(dyn_fifo_t *fifo)
{
	int value;

	if (!dyn_fifo_is_valid(fifo))
		return EINVAL;

	if (fifo->head == fifo->tail)
		return ENOENT;

	value = fifo->items[fifo->head];
	fifo->head = NEXT_INDEX(fifo, fifo->head);
	return value;
}

/** Returns and keeps the first item in the queue.
 *
 * @param[in,out] fifo	The dynamic queue.
 * @return		Value of the first item in the queue.
 * @return		EINVAL if the queue is not valid.
 * @return		ENOENT if the queue is empty.
 */
int dyn_fifo_value(dyn_fifo_t *fifo)
{
	if (!dyn_fifo_is_valid(fifo))
		return EINVAL;

	if (fifo->head == fifo->tail)
		return ENOENT;

	return fifo->items[fifo->head];
}

/** Clears and destroys the queue.
 *
 * @param[in,out] fifo	The dynamic queue.
 * @return		EOK on success.
 * @return		EINVAL if the queue is not valid.
 */
int dyn_fifo_destroy(dyn_fifo_t *fifo)
{
	if (!dyn_fifo_is_valid(fifo))
		return EINVAL;

	free(fifo->items);
	fifo->magic_value = 0;
	return EOK;
}

/** @}
 */
