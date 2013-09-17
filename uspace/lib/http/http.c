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

void recv_reset(http_t *http)
{
	http->recv_buffer_in = 0;
	http->recv_buffer_out = 0;
}

/** Receive one character (with buffering) */
int recv_char(http_t *http, char *c, bool consume)
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

ssize_t recv_buffer(http_t *http, char *buf, size_t buf_size)
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
int recv_discard(http_t *http, char discard)
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
ssize_t recv_line(http_t *http, char *line, size_t size)
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
		rc = dnsr_name2host(http->host, &hinfo, AF_NONE);
		
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
