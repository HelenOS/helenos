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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>
#include <task.h>
#include <macros.h>

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
	fprintf(stderr, "Usage: download [-o <outfile>] <url>\n");
	fprintf(stderr, "  Without -o, data will be written to stdout, so you may want\n");
	fprintf(stderr, "  to redirect the output, e.g.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "    download http://helenos.org/ | to helenos.html\n\n");
}

int main(int argc, char *argv[])
{
	int i;
	char *ofname = NULL;
	FILE *ofile = NULL;
	size_t buf_size = 4096;
	void *buf = NULL;
	uri_t *uri = NULL;
	http_t *http = NULL;
	errno_t rc;
	int ret;

	if (argc < 2) {
		syntax_print();
		rc = EINVAL;
		goto error;
	}

	i = 1;

	if (str_cmp(argv[i], "-o") == 0) {
		++i;
		if (argc < i + 1) {
			syntax_print();
			rc = EINVAL;
			goto error;
		}

		ofname = argv[i++];
		ofile = fopen(ofname, "wb");
		if (ofile == NULL) {
			fprintf(stderr, "Error creating '%s'.\n", ofname);
			rc = EINVAL;
			goto error;
		}
	}

	if (argc != i + 1) {
		syntax_print();
		rc = EINVAL;
		goto error;
	}

	uri = uri_parse(argv[i]);
	if (uri == NULL) {
		fprintf(stderr, "Failed parsing URI\n");
		rc = EINVAL;
		goto error;
	}

	if (!uri_validate(uri)) {
		fprintf(stderr, "The URI is invalid\n");
		rc = EINVAL;
		goto error;
	}

	/* TODO uri_normalize(uri) */

	if (str_cmp(uri->scheme, "http") != 0) {
		fprintf(stderr, "Only http scheme is supported at the moment\n");
		rc = EINVAL;
		goto error;
	}

	if (uri->host == NULL) {
		fprintf(stderr, "host not set\n");
		rc = EINVAL;
		goto error;
	}

	uint16_t port = 80;
	if (uri->port != NULL) {
		rc = str_uint16_t(uri->port, NULL, 10, true, &port);
		if (rc != EOK) {
			fprintf(stderr, "Invalid port number: %s\n", uri->port);
			rc = EINVAL;
			goto error;
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
			rc = ENOMEM;
			goto error;
		}
	} else {
		ret = asprintf(&server_path, "%s?%s", path, uri->query);
		if (ret < 0) {
			fprintf(stderr, "Failed allocating path\n");
			rc = ENOMEM;
			goto error;
		}
	}

	http_request_t *req = http_request_create("GET", server_path);
	free(server_path);
	if (req == NULL) {
		fprintf(stderr, "Failed creating request\n");
		rc = ENOMEM;
		goto error;
	}

	rc = http_headers_append(&req->headers, "Host", uri->host);
	if (rc != EOK) {
		fprintf(stderr, "Failed setting Host header: %s\n", str_error(rc));
		goto error;
	}

	rc = http_headers_append(&req->headers, "User-Agent", USER_AGENT);
	if (rc != EOK) {
		fprintf(stderr, "Failed creating User-Agent header: %s\n", str_error(rc));
		goto error;
	}

	http = http_create(uri->host, port);
	if (http == NULL) {
		fprintf(stderr, "Failed creating HTTP object\n");
		rc = ENOMEM;
		goto error;
	}

	rc = http_connect(http);
	if (rc != EOK) {
		fprintf(stderr, "Failed connecting: %s\n", str_error(rc));
		rc = EIO;
		goto error;
	}

	rc = http_send_request(http, req);
	if (rc != EOK) {
		fprintf(stderr, "Failed sending request: %s\n", str_error(rc));
		rc = EIO;
		goto error;
	}

	http_response_t *response = NULL;
	rc = http_receive_response(&http->recv_buffer, &response, 16 * 1024,
	    100);
	if (rc != EOK) {
		fprintf(stderr, "Failed receiving response: %s\n", str_error(rc));
		rc = EIO;
		goto error;
	}

	if (response->status != 200) {
		fprintf(stderr, "Server returned status %d %s\n", response->status,
		    response->message);
	} else {
		buf = malloc(buf_size);
		if (buf == NULL) {
			fprintf(stderr, "Failed allocating buffer\n)");
			rc = ENOMEM;
			goto error;
		}

		size_t body_size;
		while ((rc = recv_buffer(&http->recv_buffer, buf, buf_size, &body_size)) == EOK && body_size > 0) {
			fwrite(buf, 1, body_size, ofile != NULL ? ofile : stdout);
		}

		if (rc != EOK) {
			fprintf(stderr, "Failed receiving body: %s", str_error(rc));
		}
	}

	free(buf);
	http_destroy(http);
	uri_destroy(uri);
	if (ofile != NULL && fclose(ofile) != 0) {
		printf("Error writing '%s'.\n", ofname);
		return EIO;
	}

	return EOK;
error:
	free(buf);
	if (http != NULL)
		http_destroy(http);
	if (uri != NULL)
		uri_destroy(uri);
	if (ofile != NULL)
		fclose(ofile);
	return rc;
}

/** @}
 */
