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

/** @addtogroup download
 * @{
 */

/** @file
 * Download a file from a HTTP server
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>
#include <task.h>
#include <macros.h>

#include <net/in.h>
#include <net/inet.h>
#include <net/socket.h>

#include <http/http.h>
#include <uri.h>

#define NAME "download"
#ifdef TIMESTAMP_UNIX
#define VERSION STRING(RELEASE) "-" STRING(TIMESTAMP_UNIX)
#else
#define VERSION STRING(RELEASE)
#endif
#define USER_AGENT "HelenOS-" NAME "/" VERSION

static void syntax_print(void)
{
	fprintf(stderr, "Usage: download <url>\n");
	fprintf(stderr, "  This will write the data to stdout, so you may want\n");
	fprintf(stderr, "  to redirect the output, e.g.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "    download http://helenos.org/ | to helenos.html\n\n");
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		syntax_print();
		return 2;
	}
	
	uri_t *uri = uri_parse(argv[1]);
	if (uri == NULL) {
		fprintf(stderr, "Failed parsing URI\n");
		return 2;
	}
	
	if (!uri_validate(uri)) {
		fprintf(stderr, "The URI is invalid\n");
		return 2;
	}
	
	/* TODO uri_normalize(uri) */
	
	if (str_cmp(uri->scheme, "http") != 0) {
		fprintf(stderr, "Only http scheme is supported at the moment\n");
		return 2;
	}
	
	if (uri->host == NULL) {
		fprintf(stderr, "host not set\n");
		return 2;
	}
	
	uint16_t port = 80;
	if (uri->port != NULL) {
		int rc = str_uint16_t(uri->port, NULL, 10, true, &port);
		if (rc != EOK) {
			fprintf(stderr, "Invalid port number: %s\n", uri->port);
			return 2;
		}
	}
	
	const char *path = uri->path;
	if (path == NULL || *path == 0)
		path = "/";
	char *server_path = NULL;
	if (uri->query == NULL) {
		server_path = str_dup(path);
		if (server_path == NULL) {
			fprintf(stderr, "Failed allocating path\n");
			uri_destroy(uri);
			return 3;
		}
	}
	else {
		int rc = asprintf(&server_path, "%s?%s", path, uri->query);
		if (rc < 0) {
			fprintf(stderr, "Failed allocating path\n");
			uri_destroy(uri);
			return 3;
		}
	}
	
	http_request_t *req = http_request_create("GET", server_path);
	free(server_path);
	if (req == NULL) {
		fprintf(stderr, "Failed creating request\n");
		uri_destroy(uri);
		return 3;
	}
	
	int rc = http_headers_append(&req->headers, "Host", uri->host);
	if (rc != EOK) {
		fprintf(stderr, "Failed setting Host header: %s\n", str_error(rc));
		uri_destroy(uri);
		return rc;
	}
	
	rc = http_headers_append(&req->headers, "User-Agent", USER_AGENT);
	if (rc != EOK) {
		fprintf(stderr, "Failed creating User-Agent header: %s\n", str_error(rc));
		uri_destroy(uri);
		return rc;
	}
	
	http_t *http = http_create(uri->host, port);
	if (http == NULL) {
		uri_destroy(uri);
		fprintf(stderr, "Failed creating HTTP object\n");
		return 3;
	}
	
	rc = http_connect(http);
	if (rc != EOK) {
		fprintf(stderr, "Failed connecting: %s\n", str_error(rc));
		uri_destroy(uri);
		return rc;
	}
	
	rc = http_send_request(http, req);
	if (rc != EOK) {
		fprintf(stderr, "Failed sending request: %s\n", str_error(rc));
		uri_destroy(uri);
		return rc;
	}
	
	http_response_t *response = NULL;
	rc = http_receive_response(&http->recv_buffer, &response, 16 * 1024,
	    100);
	if (rc != EOK) {
		fprintf(stderr, "Failed receiving response: %s\n", str_error(rc));
		uri_destroy(uri);
		return rc;
	}
	
	if (response->status != 200) {
		fprintf(stderr, "Server returned status %d %s\n", response->status,
		    response->message);
	}
	else {
		size_t buf_size = 4096;
		void *buf = malloc(buf_size);
		if (buf == NULL) {
			fprintf(stderr, "Failed allocating buffer\n)");
			uri_destroy(uri);
			return ENOMEM;
		}
		
		int body_size;
		while ((body_size = recv_buffer(&http->recv_buffer, buf, buf_size)) > 0) {
			fwrite(buf, 1, body_size, stdout);
		}
		
		if (body_size != 0) {
			fprintf(stderr, "Failed receiving body: %s", str_error(body_size));
		}
	}
	
	uri_destroy(uri);
	return EOK;
}

/** @}
 */
