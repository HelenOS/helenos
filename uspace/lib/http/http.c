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

#include <net/socket.h>
#include <inet/dnsr.h>

#include "http.h"

#define HTTP_METHOD_LINE "%s %s HTTP/1.1\r\n"
#define HTTP_HEADER_LINE "%s: %s\r\n"
#define HTTP_REQUEST_LINE "\r\n"

static char *cut_str(const char *start, const char *end)
{
	size_t size = end - start;
	return str_ndup(start, size);
}

static void recv_reset(http_t *http)
{
	http->recv_buffer_in = 0;
	http->recv_buffer_out = 0;
}

/** Receive one character (with buffering) */
static int recv_char(http_t *http, char *c, bool consume)
{
	if (http->recv_buffer_out == http->recv_buffer_in) {
		recv_reset(http);
		
		ssize_t rc = recv(http->conn_sd, http->recv_buffer, http->buffer_size, 0);
		if (rc <= 0)
			return rc;
		
		http->recv_buffer_in = rc;
	}
	
	*c = http->recv_buffer[http->recv_buffer_out];
	if (consume)
		http->recv_buffer_out++;
	return EOK;
}

static ssize_t recv_buffer(http_t *http, char *buf, size_t buf_size)
{
	/* Flush any buffered data*/
	if (http->recv_buffer_out != http->recv_buffer_in) {
		size_t size = min(http->recv_buffer_in - http->recv_buffer_out, buf_size);
		memcpy(buf, http->recv_buffer + http->recv_buffer_out, size);
		http->recv_buffer_out += size;
		return size;
	}
	
	return recv(http->conn_sd, buf, buf_size, 0);
}

/** Receive a character and if it is c, discard it from input buffer */
static int recv_discard(http_t *http, char discard)
{
	char c = 0;
	int rc = recv_char(http, &c, false);
	if (rc != EOK)
		return rc;
	if (c != discard)
		return EOK;
	return recv_char(http, &c, true);
}

/* Receive a single line */
static ssize_t recv_line(http_t *http, char *line, size_t size)
{
	size_t written = 0;
	
	while (written < size) {
		char c = 0;
		int rc = recv_char(http, &c, true);
		if (rc != EOK)
			return rc;
		if (c == '\n') {
			recv_discard(http, '\r');
			line[written++] = 0;
			return written;
		}
		else if (c == '\r') {
			recv_discard(http, '\n');
			line[written++] = 0;
			return written;
		}
		line[written++] = c;
	}
	
	return ELIMIT;
}

http_t *http_create(const char *host, uint16_t port)
{
	http_t *http = malloc(sizeof(http_t));
	if (http == NULL)
		return NULL;
	
	http->host = str_dup(host);
	if (http->host == NULL) {
		free(http);
		return NULL;
	}
	http->port = port;
	
	http->buffer_size = 4096;
	http->recv_buffer = malloc(http->buffer_size);
	if (http->recv_buffer == NULL) {
		free(http->host);
		free(http);
		return NULL;
	}
	
	return http;
}

int http_connect(http_t *http)
{
	if (http->connected)
		return EBUSY;
	
	/* Interpret as address */
	int rc = inet_addr_parse(http->host, &http->addr);
	
	if (rc != EOK) {
		/* Interpret as a host name */
		dnsr_hostinfo_t *hinfo = NULL;
		rc = dnsr_name2host(http->host, &hinfo, ip_any);
		
		if (rc != EOK)
			return rc;
		
		http->addr = hinfo->addr;
		dnsr_hostinfo_destroy(hinfo);
	}
	
	struct sockaddr_in addr;
	struct sockaddr_in6 addr6;
	uint16_t af = inet_addr_sockaddr_in(&http->addr, &addr, &addr6);
	
	http->conn_sd = socket(PF_INET, SOCK_STREAM, 0);
	if (http->conn_sd < 0)
		return http->conn_sd;
	
	switch (af) {
	case AF_INET:
		addr.sin_port = htons(http->port);
		rc = connect(http->conn_sd, (struct sockaddr *) &addr, sizeof(addr));
		break;
	case AF_INET6:
		addr6.sin6_port = htons(http->port);
		rc = connect(http->conn_sd, (struct sockaddr *) &addr6, sizeof(addr6));
		break;
	default:
		return ENOTSUP;
	}
	
	return rc;
}

http_header_t *http_header_create(const char *name, const char *value)
{
	char *dname = str_dup(name);
	if (dname == NULL)
		return NULL;
	
	char *dvalue = str_dup(value);
	if (dvalue == NULL) {
		free(dname);
		return NULL;
	}

	return http_header_create_no_copy(dname, dvalue);
}

http_header_t *http_header_create_no_copy(char *name, char *value)
{
	http_header_t *header = malloc(sizeof(http_header_t));
	if (header == NULL)
		return NULL;
	
	link_initialize(&header->link);
	header->name = name;
	header->value = value;
	
	return header;
}

void http_header_destroy(http_header_t *header)
{
	free(header->name);
	free(header->value);
	free(header);
}

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
	
	list_initialize(&req->headers);
	
	return req;
}

void http_request_destroy(http_request_t *req)
{
	free(req->method);
	free(req->path);
	link_t *link = req->headers.head.next;
	while (link != &req->headers.head) {
		link_t *next = link->next;
		http_header_t *header = list_get_instance(link, http_header_t, link);
		http_header_destroy(header);
		link = next;
	}
	free(req);
}

static ssize_t http_encode_method(char *buf, size_t buf_size,
    const char *method, const char *path)
{
	if (buf == NULL) {
		return printf_size(HTTP_METHOD_LINE, method, path);
	}
	else {
		return snprintf(buf, buf_size, HTTP_METHOD_LINE, method, path);
	}
}

static ssize_t http_encode_header(char *buf, size_t buf_size,
    http_header_t *header)
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

int http_request_format(http_request_t *req, char **out_buf,
    size_t *out_buf_size)
{
	/* Compute the size of the request */
	ssize_t meth_size = http_encode_method(NULL, 0, req->method, req->path);
	if (meth_size < 0)
		return meth_size;
	size_t size = meth_size;
	
	list_foreach(req->headers, link, http_header_t, header) {
		ssize_t header_size = http_encode_header(NULL, 0, header);
		if (header_size < 0)
			return header_size;
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
		return written;
	}
	pos += written;
	pos_size -= written;
	
	list_foreach(req->headers, link, http_header_t, header) {
		written = http_encode_header(pos, pos_size, header);
		if (written < 0) {
			free(buf);
			return written;
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

int http_send_request(http_t *http, http_request_t *req)
{
	char *buf;
	size_t buf_size;
	
	int rc = http_request_format(req, &buf, &buf_size);
	if (rc != EOK)
		return rc;
	
	rc = send(http->conn_sd, buf, buf_size, 0);
	free(buf);
	
	return rc;
}

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

int http_parse_header(const char *line, char **out_name, char **out_value)
{
	const char *pos = line;
	while (*pos != 0 && *pos != ':') pos++;
	if (*pos != ':')
		return EINVAL;
	
	char *name = cut_str(line, pos);
	if (name == NULL)
		return ENOMEM;
	
	pos++;
	
	while (*pos == ' ') pos++;
	
	char *value = str_dup(pos);
	if (value == NULL) {
		free(name);
		return ENOMEM;
	}
	
	*out_name = name;
	*out_value = value;
	
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
	
	int rc = recv_line(http, line, http->buffer_size);
	if (rc < 0)
		goto error;
	
	rc = http_parse_status(line, &resp->version, &resp->status,
	    &resp->message);
	if (rc != EOK)
		goto error;
	
	while (true) {
		rc = recv_line(http, line, http->buffer_size);
		if (rc < 0)
			goto error;
		if (*line == 0)
			break;
		
		char *name = NULL;
		char *value = NULL;
		rc = http_parse_header(line, &name, &value);
		if (rc != EOK)
			goto error;
		
		http_header_t *header = http_header_create_no_copy(name, value);
		if (header == NULL) {
			free(name);
			free(value);
			rc = ENOMEM;
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
	return recv_buffer(http, buf, buf_size);
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

int http_close(http_t *http)
{
	if (!http->connected)
		return EINVAL;
	
	return closesocket(http->conn_sd);
}

void http_destroy(http_t *http)
{
	free(http->recv_buffer);
	free(http);
}

/** @}
 */
