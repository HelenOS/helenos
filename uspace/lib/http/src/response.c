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

#include <http/http.h>

int http_parse_status(const char *line, http_version_t *out_version,
    uint16_t *out_status, char **out_message)
{
	http_version_t version;
	uint16_t status;
	char *message = NULL;
	
	if (!str_test_prefix(line, "HTTP/"))
		return EINVAL;
	
	const char *pos_version = line + 5;
	const char *pos = pos_version;
	
	int rc = str_uint8_t(pos_version, &pos, 10, false, &version.major);
	if (rc != EOK)
		return rc;
	if (*pos != '.')
		return EINVAL;
	pos++;
	
	pos_version = pos;
	rc = str_uint8_t(pos_version, &pos, 10, false, &version.minor);
	if (rc != EOK)
		return rc;
	if (*pos != ' ')
		return EINVAL;
	pos++;
	
	const char *pos_status = pos;
	rc = str_uint16_t(pos_status, &pos, 10, false, &status);
	if (rc != EOK)
		return rc;
	if (*pos != ' ')
		return EINVAL;
	pos++;
	
	if (out_message) {
		message = str_dup(pos);
		if (message == NULL)
			return ENOMEM;
	}
	
	if (out_version)
		*out_version = version;
	if (out_status)
		*out_status = status;
	if (out_message)
		*out_message = message;
	return EOK;
}

int http_receive_response(http_t *http, http_response_t **out_response)
{
	http_response_t *resp = malloc(sizeof(http_response_t));
	if (resp == NULL)
		return ENOMEM;
	memset(resp, 0, sizeof(http_response_t));
	list_initialize(&resp->headers);
	
	char *line = malloc(http->buffer_size);
	if (line == NULL) {
		free(resp);
		return ENOMEM;
	}
	
	int rc = recv_line(&http->recv_buffer, line, http->buffer_size);
	if (rc < 0)
		goto error;
	
	rc = http_parse_status(line, &resp->version, &resp->status,
	    &resp->message);
	if (rc != EOK)
		goto error;
	
	while (true) {
		rc = recv_eol(&http->recv_buffer);
		if (rc < 0)
			goto error;
		
		/* Empty line ends header part */
		if (rc > 0)
			break;
		
		http_header_t *header = malloc(sizeof(http_header_t));
		if (header == NULL) {
			rc = ENOMEM;
			goto error;
		}
		http_header_init(header);
		
		rc = http_header_receive(&http->recv_buffer, header);
		if (rc != EOK) {
			free(header);
			goto error;
		}
		
		list_append(&header->link, &resp->headers);
	}
	
	*out_response = resp;
	
	return EOK;
error:
	free(line);
	http_response_destroy(resp);
	return rc;
}

int http_receive_body(http_t *http, void *buf, size_t buf_size)
{
	return recv_buffer(&http->recv_buffer, buf, buf_size);
}

void http_response_destroy(http_response_t *resp)
{
	free(resp->message);
	link_t *link = resp->headers.head.next;
	while (link != &resp->headers.head) {
		link_t *next = link->next;
		http_header_t *header = list_get_instance(link, http_header_t, link);
		http_header_destroy(header);
		link = next;
	}
	free(resp);
}

/** @}
 */
