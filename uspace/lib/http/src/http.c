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

#include <http/http.h>
#include <http/receive-buffer.h>

static ssize_t http_receive(void *client_data, void *buf, size_t buf_size)
{
	http_t *http = client_data;
	return recv(http->conn_sd, buf, buf_size, 0);
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
	int rc = recv_buffer_init(&http->recv_buffer, http->buffer_size,
	    http_receive, http);
	if (rc != EOK) {
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
	
	struct sockaddr *saddr;
	socklen_t saddrlen;
	
	rc = inet_addr_sockaddr(&http->addr, http->port, &saddr, &saddrlen);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		return ENOMEM;
	}
	
	http->conn_sd = socket(saddr->sa_family, SOCK_STREAM, 0);
	if (http->conn_sd < 0)
		return http->conn_sd;
	
	rc = connect(http->conn_sd, saddr, saddrlen);
	free(saddr);
	
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
	recv_buffer_fini(&http->recv_buffer);
	free(http);
}

/** @}
 */
