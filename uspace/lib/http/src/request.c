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

#include <inet/tcp.h>

#include <http/http.h>

#define HTTP_METHOD_LINE "%s %s HTTP/1.1\r\n"
#define HTTP_REQUEST_LINE "\r\n"

http_request_t *http_request_create(const char *method, const char *path)
{
	http_request_t *req = malloc(sizeof(http_request_t));
	if (req == NULL)
		return NULL;

	req->method = str_dup(method);
	if (req->method == NULL) {
		free(req);
		return NULL;
	}

	req->path = str_dup(path);
	if (req->path == NULL) {
		free(req->method);
		free(req);
		return NULL;
	}

	http_headers_init(&req->headers);

	return req;
}

void http_request_destroy(http_request_t *req)
{
	free(req->method);
	free(req->path);
	http_headers_clear(&req->headers);
	free(req);
}

static ssize_t http_encode_method(char *buf, size_t buf_size,
    const char *method, const char *path)
{
	return snprintf(buf, buf_size, HTTP_METHOD_LINE, method, path);
}

errno_t http_request_format(http_request_t *req, char **out_buf,
    size_t *out_buf_size)
{
	/* Compute the size of the request */
	ssize_t meth_size = http_encode_method(NULL, 0, req->method, req->path);
	if (meth_size < 0)
		return EINVAL;
	size_t size = meth_size;

	http_headers_foreach(req->headers, header) {
		ssize_t header_size = http_header_encode(header, NULL, 0);
		if (header_size < 0)
			return EINVAL;
		size += header_size;
	}
	size += str_length(HTTP_REQUEST_LINE);

	char *buf = malloc(size);
	if (buf == NULL)
		return ENOMEM;

	char *pos = buf;
	size_t pos_size = size;
	ssize_t written = http_encode_method(pos, pos_size, req->method, req->path);
	if (written < 0) {
		free(buf);
		return EINVAL;
	}
	pos += written;
	pos_size -= written;

	http_headers_foreach(req->headers, header) {
		written = http_header_encode(header, pos, pos_size);
		if (written < 0) {
			free(buf);
			return EINVAL;
		}
		pos += written;
		pos_size -= written;
	}

	size_t rlsize = str_size(HTTP_REQUEST_LINE);
	memcpy(pos, HTTP_REQUEST_LINE, rlsize);
	pos_size -= rlsize;
	assert(pos_size == 0);

	*out_buf = buf;
	*out_buf_size = size;
	return EOK;
}

errno_t http_send_request(http_t *http, http_request_t *req)
{
	char *buf = NULL;
	size_t buf_size = 0;

	errno_t rc = http_request_format(req, &buf, &buf_size);
	if (rc != EOK)
		return rc;

	rc = tcp_conn_send(http->conn, buf, buf_size);
	free(buf);

	return rc;
}

/** @}
 */
