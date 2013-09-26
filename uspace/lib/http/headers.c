/*
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

#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <macros.h>

#include "http.h"
#include "http-ctype.h"

#define HTTP_HEADER_LINE "%s: %s\r\n"

void http_header_init(http_header_t *header)
{
	link_initialize(&header->link);
	header->name = NULL;
	header->value = NULL;
}

http_header_t *http_header_create(const char *name, const char *value)
{
	http_header_t *header = malloc(sizeof(http_header_t));
	if (header == NULL)
		return NULL;
	http_header_init(header);

	header->name = str_dup(name);
	if (header->name == NULL) {
		free(header);
		return NULL;
	}
	
	header->value = str_dup(value);
	if (header->value == NULL) {
		free(header->name);
		free(header);
		return NULL;
	}

	return header;
}

void http_header_destroy(http_header_t *header)
{
	free(header->name);
	free(header->value);
	free(header);
}

ssize_t http_header_encode(http_header_t *header, char *buf, size_t buf_size)
{
	/* TODO properly split long header values */
	if (buf == NULL) {
		return printf_size(HTTP_HEADER_LINE, header->name, header->value);
	}
	else {
		return snprintf(buf, buf_size,
		    HTTP_HEADER_LINE, header->name, header->value);
	}
}

int http_header_receive_name(receive_buffer_t *rb, char **out_name)
{
	receive_buffer_mark_t name_start;
	receive_buffer_mark_t name_end;
	
	recv_mark(rb, &name_start);
	recv_mark(rb, &name_end);
	
	char c = 0;
	do {
		recv_mark_update(rb, &name_end);
		
		int rc = recv_char(rb, &c, true);
		if (rc != EOK) {
			recv_unmark(rb, &name_start);
			recv_unmark(rb, &name_end);
			return rc;
		}
	} while (is_token(c));
	
	if (c != ':') {
		recv_unmark(rb, &name_start);
		recv_unmark(rb, &name_end);
		return EINVAL;
	}
	
	char *name = NULL;
	int rc = recv_cut_str(rb, &name_start, &name_end, &name);
	recv_unmark(rb, &name_start);
	recv_unmark(rb, &name_end);
	if (rc != EOK)
		return rc;
	
	*out_name = name;
	return EOK;
}

int http_header_receive_value(receive_buffer_t *rb, char **out_value)
{
	int rc = EOK;
	char c = 0;
	
	receive_buffer_mark_t value_start;
	recv_mark(rb, &value_start);
	
	/* Ignore any inline LWS */
	while (true) {
		recv_mark_update(rb, &value_start);
		rc = recv_char(rb, &c, false);
		if (rc != EOK)
			goto error;
		
		if (c != ' ' && c != '\t')
			break;
		
		rc = recv_char(rb, &c, true);
		if (rc != EOK)
			goto error;
	}
	
	receive_buffer_mark_t value_end;
	recv_mark(rb, &value_end);
	
	while (true) {
		recv_mark_update(rb, &value_end);
		
		rc = recv_char(rb, &c, true);
		if (rc != EOK)
			goto error_end;
		
		if (c != '\r' && c != '\n')
			continue;
		
		rc = recv_discard(rb, (c == '\r' ? '\n' : '\r'));
		if (rc < 0)
			goto error_end;
		
		rc = recv_char(rb, &c, false);
		if (rc != EOK)
			goto error_end;
		
		if (c != ' ' && c != '\t')
			break;
		
		/* Ignore the char */
		rc = recv_char(rb, &c, true);
		if (rc != EOK)
			goto error_end;
	}
	
	char *value = NULL;
	rc = recv_cut_str(rb, &value_start, &value_end, &value);
	recv_unmark(rb, &value_start);
	recv_unmark(rb, &value_end);
	if (rc != EOK)
		return rc;
	
	*out_value = value;
	return EOK;
error_end:
	recv_unmark(rb, &value_end);
error:
	recv_unmark(rb, &value_start);
	return rc;
}

int http_header_receive(receive_buffer_t *rb, http_header_t *header)
{
	char *name = NULL;
	int rc = http_header_receive_name(rb, &name);
	if (rc != EOK) {
		printf("Failed receiving header name\n");
		return rc;
	}
	
	char *value = NULL;
	rc = http_header_receive_value(rb, &value);
	if (rc != EOK) {
		free(name);
		return rc;
	}
	
	header->name = name;
	header->value = value;
	return EOK;
}

/** Normalize HTTP header value
 *
 * @see RFC2616 section 4.2
 */
void http_header_normalize_value(char *value)
{
	size_t read_index = 0;
	size_t write_index = 0;
	
	while (is_lws(value[read_index])) read_index++;
	
	while (value[read_index] != 0) {
		if (is_lws(value[read_index])) {
			while (is_lws(value[read_index])) read_index++;
			
			if (value[read_index] != 0)
				value[write_index++] = ' ';
			
			continue;
		}
		
		value[write_index++] = value[read_index++];
	}
	
	value[write_index] = 0;
}

/** @}
 */
