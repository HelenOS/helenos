/*
 * Copyright (c) 2011 Jan Vesely
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
/** @addtogroup kbd_port
 * @ingroup  kbd
 * @{
 */
/** @file
 * @brief i8042 port driver.
 */

#ifndef BUFFER_H_
#define BUFFER_H_

#include <assert.h>
#include <fibril_synch.h>

typedef struct {
	uint8_t *buffer;
	uint8_t *buffer_end;
	fibril_mutex_t guard;
	fibril_condvar_t change;
	uint8_t *read_head;
	uint8_t *write_head;
} buffer_t;

static inline void buffer_init(buffer_t *buffer, uint8_t *data, size_t size)
{
	assert(buffer);
	fibril_mutex_initialize(&buffer->guard);
	fibril_condvar_initialize(&buffer->change);
	buffer->buffer = data;
	buffer->buffer_end = data + size;
	buffer->read_head = buffer->buffer;
	buffer->write_head = buffer->buffer;
	bzero(buffer->buffer, size);
}

static inline void buffer_write(buffer_t *buffer, uint8_t data)
{
	fibril_mutex_lock(&buffer->guard);

	/* Next position. */
	uint8_t *new_head = buffer->write_head + 1;
	if (new_head == buffer->buffer_end)
		new_head = buffer->buffer;

	/* Buffer full. */
	while (new_head == buffer->read_head)
		fibril_condvar_wait(&buffer->change, &buffer->guard);

	/* Write data. */
	*buffer->write_head = data;

	/* Buffer was empty. */
	if (buffer->write_head == buffer->read_head)
		fibril_condvar_broadcast(&buffer->change);

	/* Move head */
	buffer->write_head = new_head;
	fibril_mutex_unlock(&buffer->guard);
}

static inline uint8_t buffer_read(buffer_t *buffer)
{
	fibril_mutex_lock(&buffer->guard);
	/* Buffer is empty. */
	while (buffer->write_head == buffer->read_head)
		fibril_condvar_wait(&buffer->change, &buffer->guard);

	/* Next position. */
	uint8_t *new_head = buffer->read_head + 1;
	if (new_head == buffer->buffer_end)
		new_head = buffer->buffer;

	/* Read data. */
	const uint8_t data = *buffer->read_head;

	/* Buffer was full. */
	uint8_t *new_write_head = buffer->write_head + 1;
	if (new_write_head == buffer->buffer_end)
		new_write_head = buffer->buffer;
	if (new_write_head == buffer->read_head)
		fibril_condvar_broadcast(&buffer->change);

	/* Move head */
	buffer->read_head = new_head;

	fibril_mutex_unlock(&buffer->guard);
	return data;
}


#endif
/**
 * @}
 */
