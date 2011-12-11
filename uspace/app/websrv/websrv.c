/*
 * Copyright (c) 2011 Jiri Svoboda
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

/** @addtogroup websrv
 * @{
 */
/**
 * @file Skeletal web server.
 */

#include <bool.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>

#include <net/in.h>
#include <net/inet.h>
#include <net/socket.h>

#include <str.h>

#define PORT_NUMBER 8080

#define WEB_ROOT "/data/web"

/** Buffer for receiving the request. */
#define BUFFER_SIZE 1024
static char rbuf[BUFFER_SIZE];
static size_t rbuf_out, rbuf_in;

static char lbuf[BUFFER_SIZE + 1];
static size_t lbuf_used;

static char fbuf[BUFFER_SIZE];

/** Response to send to client. */
static const char *ok_msg =
    "HTTP/1.0 200 OK\r\n"
    "\r\n";

/** Receive one character (with buffering) */
static int recv_char(int fd, char *c)
{
	ssize_t rc;

	if (rbuf_out == rbuf_in) {
		rbuf_out = 0;
		rbuf_in = 0;

		rc = recv(fd, rbuf, BUFFER_SIZE, 0);
		if (rc <= 0) {
			printf("recv() failed (%zd)\n", rc);
			return rc;
		}

		rbuf_in = rc;
	}

	*c = rbuf[rbuf_out++];
	return EOK;
}

/** Receive one line with length limit. */
static int recv_line(int fd)
{
	char c, prev;
	int rc;
	char *bp;

	bp = lbuf; c = '\0';
	while (bp < lbuf + BUFFER_SIZE) {
		prev = c;
		rc = recv_char(fd, &c);
		if (rc != EOK)
			return rc;

		*bp++ = c;
		if (prev == '\r' && c == '\n')
			break;
	}

	lbuf_used = bp - lbuf;
	*bp = '\0';

	if (bp == lbuf + BUFFER_SIZE)
		return ELIMIT;

	return EOK;
}

static bool uri_is_valid(char *uri)
{
	char *cp;
	char c;

	if (uri[0] != '/')
		return false;
	if (uri[1] == '.')
		return false;

	cp = uri + 1;
	while (*cp != '\0') {
		c = *cp++;
		if (c == '/')
			return false;
	}

	return true;
}

static int send_response(int conn_sd, const char *msg)
{
	size_t response_size;
	ssize_t rc;

	response_size = str_size(msg);

	/* Send a canned response. */
        printf("Send response...\n");
	rc = send(conn_sd, (void *) msg, response_size, 0);
	if (rc < 0) {
		printf("send() failed.\n");
		return rc;
	}

	return EOK;
}

static int uri_get(const char *uri, int conn_sd)
{
	int rc;
	char *fname;
	int fd;
	ssize_t nr;

	if (str_cmp(uri, "/") == 0)
		uri = "/index.htm";

	rc = asprintf(&fname, "%s%s", WEB_ROOT, uri);
	if (rc < 0)
		return ENOMEM;

	fd = open(fname, O_RDONLY);
	if (fd < 0) {
		printf("File '%s' not found.\n", fname);
		free(fname);
		return ENOENT;
	}

	free(fname);

	rc = send_response(conn_sd, ok_msg);
	if (rc != EOK)
		return rc;

	while (true) {
		nr = read(fd, fbuf, BUFFER_SIZE);
		if (nr == 0)
			break;

		if (nr < 0) {
			close(fd);
			return EIO;
		}

		rc = send(conn_sd, fbuf, nr, 0);
		if (rc < 0) {
			printf("send() failed\n");
			close(fd);
			return rc;
		}
	}

	close(fd);

	return EOK;
}

static int req_process(int conn_sd)
{
	int rc;
	char *uri, *end_uri;

	rc = recv_line(conn_sd);
	if (rc != EOK) {
		printf("recv_line() failed\n");
		return rc;
	}

	printf("%s", lbuf);

	if (str_lcmp(lbuf, "GET ", 4) != 0) {
		printf("Invalid HTTP method.\n");
		return EINVAL;
	}

	uri = lbuf + 4;
	end_uri = str_chr(uri, ' ');
	if (end_uri == NULL) {
		end_uri = lbuf + lbuf_used - 2;
		assert(*end_uri == '\r');
	}

	*end_uri = '\0';
	printf("Requested URI '%s'.\n", uri);

	if (!uri_is_valid(uri)) {
		printf("Invalid request URI.\n");
		return EINVAL;
	}

	return uri_get(uri, conn_sd);
}

int main(int argc, char *argv[])
{
	struct sockaddr_in addr;
	struct sockaddr_in raddr;

	socklen_t raddr_len;

	int listen_sd, conn_sd;
	int rc;


	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT_NUMBER);

	rc = inet_pton(AF_INET, "127.0.0.1", (void *) &addr.sin_addr.s_addr);
	if (rc != EOK) {
		printf("Error parsing network address.\n");
		return 1;
	}

	printf("Creating socket.\n");

	listen_sd = socket(PF_INET, SOCK_STREAM, 0);
	if (listen_sd < 0) {
		printf("Error creating listening socket.\n");
		return 1;
	}

	rc = bind(listen_sd, (struct sockaddr *) &addr, sizeof(addr));
	if (rc != EOK) {
		printf("Error binding socket.\n");
		return 1;
	}

	rc = listen(listen_sd, 1);
	if (rc != EOK) {
		printf("Error calling listen() (%d).\n", rc);
		return 1;
	}

	printf("Listening for connections at port number %u.\n", PORT_NUMBER);
	while (true) {
		raddr_len = sizeof(raddr);
		conn_sd = accept(listen_sd, (struct sockaddr *) &raddr,
		    &raddr_len);

		if (conn_sd < 0) {
			printf("accept() failed.\n");
			continue;
		}

		printf("Accepted connection, sd=%d.\n", conn_sd);

		printf("Wait for client request\n");
		rbuf_out = rbuf_in = 0;

		rc = req_process(conn_sd);
		if (rc != EOK) 
			printf("Error processing request.\n");

		rc = closesocket(conn_sd);
		if (rc != EOK) {
			printf("Error closing connection socket: %d\n", rc);
			return 1;
		}

		printf("Closed connection.\n");
	}

	/* Not reached. */
	return 0;
}

/** @}
 */
