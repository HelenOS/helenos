/*
 * SPDX-FileCopyrightText: 2013 Martin Sucha
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
