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
#include <adt/list.h>

#include <http/receive-buffer.h>

int recv_buffer_init(receive_buffer_t *rb, size_t buffer_size,
    receive_func_t receive, void *client_data)
{
	rb->receive = receive;
	rb->client_data = client_data;
	
	rb->in = 0;
	rb->out = 0;
	rb->size = buffer_size;
	
	list_initialize(&rb->marks);
	
	rb->buffer = malloc(buffer_size);
	if (rb->buffer == NULL)
		return ENOMEM;
	return EOK;
}

static ssize_t dummy_receive(void *unused, void *buf, size_t buf_size)
{
	return 0;
}

int recv_buffer_init_const(receive_buffer_t *rb, void *buf, size_t size)
{
	int rc = recv_buffer_init(rb, size, dummy_receive, NULL);
	if (rc != EOK)
		return rc;
	
	memcpy(rb->buffer, buf, size);
	rb->in = size;
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

void recv_mark(receive_buffer_t *rb, receive_buffer_mark_t *mark)
{
	link_initialize(&mark->link);
	list_append(&mark->link, &rb->marks);
	recv_mark_update(rb, mark);
}

void recv_unmark(receive_buffer_t *rb, receive_buffer_mark_t *mark)
{
	list_remove(&mark->link);
}

void recv_mark_update(receive_buffer_t *rb, receive_buffer_mark_t *mark)
{
	mark->offset = rb->out;
}

int recv_cut(receive_buffer_t *rb, receive_buffer_mark_t *a, receive_buffer_mark_t *b, void **out_buf, size_t *out_size)
{
	if (a->offset > b->offset)
		return EINVAL;
	
	size_t size = b->offset - a->offset;
	void *buf = malloc(size);
	if (buf == NULL)
		return ENOMEM;
	
	memcpy(buf, rb->buffer + a->offset, size);
	*out_buf = buf;
	*out_size = size;
	return EOK;
}

int recv_cut_str(receive_buffer_t *rb, receive_buffer_mark_t *a, receive_buffer_mark_t *b, char **out_buf)
{
	if (a->offset > b->offset)
		return EINVAL;
	
	size_t size = b->offset - a->offset;
	char *buf = malloc(size + 1);
	if (buf == NULL)
		return ENOMEM;
	
	memcpy(buf, rb->buffer + a->offset, size);
	buf[size] = 0;
	for (size_t i = 0; i < size; i++) {
		if (buf[i] == 0) {
			free(buf);
			return EIO;
		}
	}
	*out_buf = buf;
	return EOK;
}


/** Receive one character (with buffering) */
int recv_char(receive_buffer_t *rb, char *c, bool consume)
{
	if (rb->out == rb->in) {
		size_t free = rb->size - rb->in;
		if (free == 0) {
			size_t min_mark = rb->size;
			list_foreach(rb->marks, link, receive_buffer_mark_t, mark) {
				min_mark = min(min_mark, mark->offset);
			}
			
			if (min_mark == 0)
				return ELIMIT;
			
			size_t new_in = rb->in - min_mark;
			memmove(rb->buffer, rb->buffer + min_mark, new_in);
			rb->out = rb->in = new_in;
			free = rb->size - rb->in;
			list_foreach(rb->marks, link, receive_buffer_mark_t, mark) {
				mark->offset -= min_mark;
			}
		}
		
		ssize_t rc = rb->receive(rb->client_data, rb->buffer + rb->in, free);
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

/** Receive a character and if it is c, discard it from input buffer
 * @return number of characters discarded (0 or 1) or negative error code
 */
ssize_t recv_discard(receive_buffer_t *rb, char discard)
{
	char c = 0;
	int rc = recv_char(rb, &c, false);
	if (rc != EOK)
		return rc;
	if (c != discard)
		return 0;
	rc = recv_char(rb, &c, true);
	if (rc != EOK)
		return rc;
	return 1;
}

/** Receive a prefix of constant string discard and return number of bytes read
 * @return number of characters discarded or negative error code
 */
ssize_t recv_discard_str(receive_buffer_t *rb, const char *discard)
{
	size_t discarded = 0;
	while (*discard) {
		ssize_t rc = recv_discard(rb, *discard);
		if (rc < 0)
			return rc;
		if (rc == 0)
			break;
		discarded++;
		discard++;
	}
	return discarded;
}

ssize_t recv_while(receive_buffer_t *rb, char_class_func_t class)
{
	size_t received = 0;
	
	while (true) {
		char c = 0;
		int rc = recv_char(rb, &c, false);
		if (rc != EOK)
			return rc;
		
		if (!class(c))
			break;
		
		rc = recv_char(rb, &c, true);
		if (rc != EOK)
			return rc;
		
		received++;
	}
	
	return received;
}

/** Receive an end of line, either CR, LF, CRLF or LFCR
 *
 * @return number of bytes read (0 if no newline is present in the stream)
 *         or negative error code
 */
ssize_t recv_eol(receive_buffer_t *rb)
{
	char c = 0;
	int rc = recv_char(rb, &c, false);
	if (rc != EOK)
		return rc;
	
	if (c != '\r' && c != '\n')
		return 0;
	
	rc = recv_char(rb, &c, true);
	if (rc != EOK)
		return rc;
	
	ssize_t rc2 = recv_discard(rb, (c == '\r' ? '\n' : '\r'));
	if (rc2 < 0)
		return rc2;
	
	return 1 + rc2;
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
