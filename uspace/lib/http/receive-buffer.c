/*
 * Copyright (c) 2012 Jiri Svoboda
 * Copyright (c) 2013 Martin Sucha
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

/** @addtogroup http
 * @{
 */
/**
 * @file
 */

#include <stdlib.h>
#include <str.h>
#include <errno.h>
#include <macros.h>

#include "receive-buffer.h"

int recv_buffer_init(receive_buffer_t *rb, size_t buffer_size,
    receive_func_t receive, void *client_data)
{
	rb->receive = receive;
	rb->client_data = client_data;
	
	rb->in = 0;
	rb->out = 0;
	rb->size = buffer_size;
	rb->buffer = malloc(buffer_size);
	if (rb->buffer == NULL)
		return ENOMEM;
	return EOK;
}

void recv_buffer_fini(receive_buffer_t *rb)
{
	free(rb->buffer);
}

void recv_reset(receive_buffer_t *rb)
{
	rb->in = 0;
	rb->out = 0;
}

/** Receive one character (with buffering) */
int recv_char(receive_buffer_t *rb, char *c, bool consume)
{
	if (rb->out == rb->in) {
		recv_reset(rb);
		
		ssize_t rc = rb->receive(rb->client_data, rb->buffer, rb->size);
		if (rc <= 0)
			return rc;
		
		rb->in = rc;
	}
	
	*c = rb->buffer[rb->out];
	if (consume)
		rb->out++;
	return EOK;
}

ssize_t recv_buffer(receive_buffer_t *rb, char *buf, size_t buf_size)
{
	/* Flush any buffered data*/
	if (rb->out != rb->in) {
		size_t size = min(rb->in - rb->out, buf_size);
		memcpy(buf, rb->buffer + rb->out, size);
		rb->out += size;
		return size;
	}
	
	return rb->receive(rb->client_data, buf, buf_size);
}

/** Receive a character and if it is c, discard it from input buffer */
int recv_discard(receive_buffer_t *rb, char discard)
{
	char c = 0;
	int rc = recv_char(rb, &c, false);
	if (rc != EOK)
		return rc;
	if (c != discard)
		return EOK;
	return recv_char(rb, &c, true);
}

/* Receive a single line */
ssize_t recv_line(receive_buffer_t *rb, char *line, size_t size)
{
	size_t written = 0;
	
	while (written < size) {
		char c = 0;
		int rc = recv_char(rb, &c, true);
		if (rc != EOK)
			return rc;
		if (c == '\n') {
			recv_discard(rb, '\r');
			line[written++] = 0;
			return written;
		}
		else if (c == '\r') {
			recv_discard(rb, '\n');
			line[written++] = 0;
			return written;
		}
		line[written++] = c;
	}
	
	return ELIMIT;
}

/** @}
 */
