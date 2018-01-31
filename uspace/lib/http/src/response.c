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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <macros.h>

#include <http/http.h>

static bool is_digit(char c)
{
	return (c >= '0' && c <= '9');
}

static errno_t receive_number(receive_buffer_t *rb, char **str)
{
	receive_buffer_mark_t start;
	receive_buffer_mark_t end;
	
	recv_mark(rb, &start);
	errno_t rc = recv_while(rb, is_digit);
	if (rc != EOK) {
		recv_unmark(rb, &start);
		return rc;
	}
	recv_mark(rb, &end);
	
	rc = recv_cut_str(rb, &start, &end, str);
	recv_unmark(rb, &start);
	recv_unmark(rb, &end);
	return rc;
}

static errno_t receive_uint8_t(receive_buffer_t *rb, uint8_t *out_value)
{
	char *str = NULL;
	errno_t rc = receive_number(rb, &str);
	
	if (rc != EOK)
		return rc;
	
	rc = str_uint8_t(str, NULL, 10, true, out_value);
	free(str);
	
	return rc;
}

static errno_t receive_uint16_t(receive_buffer_t *rb, uint16_t *out_value)
{
	char *str = NULL;
	errno_t rc = receive_number(rb, &str);
	
	if (rc != EOK)
		return rc;
	
	rc = str_uint16_t(str, NULL, 10, true, out_value);
	free(str);
	
	return rc;
}

static errno_t expect(receive_buffer_t *rb, const char *expect)
{
	size_t ndisc;
	errno_t rc = recv_discard_str(rb, expect, &ndisc);
	if (rc != EOK)
		return rc;
	if (ndisc < str_length(expect))
		return HTTP_EPARSE;
	return EOK;
}

static bool is_not_newline(char c)
{
	return (c != '\n' && c != '\r');
}

errno_t http_receive_status(receive_buffer_t *rb, http_version_t *out_version,
    uint16_t *out_status, char **out_message)
{
	http_version_t version;
	uint16_t status;
	char *message = NULL;
	
	errno_t rc = expect(rb, "HTTP/");
	if (rc != EOK)
		return rc;
	
	rc = receive_uint8_t(rb, &version.major);
	if (rc != EOK)
		return rc;
	
	rc = expect(rb, ".");
	if (rc != EOK)
		return rc;
	
	rc = receive_uint8_t(rb, &version.minor);
	if (rc != EOK)
		return rc;
	
	rc = expect(rb, " ");
	if (rc != EOK)
		return rc;
	
	rc = receive_uint16_t(rb, &status);
	if (rc != EOK)
		return rc;
	
	rc = expect(rb, " ");
	if (rc != EOK)
		return rc;
	
	receive_buffer_mark_t msg_start;
	recv_mark(rb, &msg_start);
	
	rc = recv_while(rb, is_not_newline);
	if (rc != EOK) {
		recv_unmark(rb, &msg_start);
		return rc;
	}
	
	receive_buffer_mark_t msg_end;
	recv_mark(rb, &msg_end);
	
	if (out_message) {
		rc = recv_cut_str(rb, &msg_start, &msg_end, &message);
		if (rc != EOK) {
			recv_unmark(rb, &msg_start);
			recv_unmark(rb, &msg_end);
			return rc;
		}
	}
	
	recv_unmark(rb, &msg_start);
	recv_unmark(rb, &msg_end);
	
	size_t nrecv;
	rc = recv_eol(rb, &nrecv);
	if (rc == EOK && nrecv == 0)
		rc = HTTP_EPARSE;
	if (rc != EOK) {
		free(message);
		return rc;
	}
	
	if (out_version)
		*out_version = version;
	if (out_status)
		*out_status = status;
	if (out_message)
		*out_message = message;
	return EOK;
}

errno_t http_receive_response(receive_buffer_t *rb, http_response_t **out_response,
    size_t max_headers_size, unsigned max_headers_count)
{
	http_response_t *resp = malloc(sizeof(http_response_t));
	if (resp == NULL)
		return ENOMEM;
	memset(resp, 0, sizeof(http_response_t));
	http_headers_init(&resp->headers);
	
	errno_t rc = http_receive_status(rb, &resp->version, &resp->status,
	    &resp->message);
	if (rc != EOK)
		goto error;
	
	rc = http_headers_receive(rb, &resp->headers, max_headers_size,
	    max_headers_count);
	if (rc != EOK)
		goto error;
	
	size_t nrecv;
	rc = recv_eol(rb, &nrecv);
	if (rc == EOK && nrecv == 0)
		rc = HTTP_EPARSE;
	if (rc != EOK)
		goto error;
	
	*out_response = resp;
	
	return EOK;
error:
	http_response_destroy(resp);
	return rc;
}

void http_response_destroy(http_response_t *resp)
{
	free(resp->message);
	http_headers_clear(&resp->headers);
	free(resp);
}

/** @}
 */
