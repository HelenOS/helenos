/*
 * Copyright (c) 2012 Jiri Svoboda
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

#include <stdbool.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <task.h>

#include <net/in.h>
#include <net/inet.h>
#include <net/socket.h>

#include <arg_parse.h>
#include <macros.h>
#include <str.h>
#include <str_error.h>

#define NAME  "websrv"

#define DEFAULT_PORT  8080
#define BACKLOG_SIZE  3

#define WEB_ROOT  "/data/web"

/** Buffer for receiving the request. */
#define BUFFER_SIZE  1024

static uint16_t port = DEFAULT_PORT;

static char rbuf[BUFFER_SIZE];
static size_t rbuf_out;
static size_t rbuf_in;

static char lbuf[BUFFER_SIZE + 1];
static size_t lbuf_used;

static char fbuf[BUFFER_SIZE];

static bool verbose = false;

/** Responses to send to client. */

static const char *msg_ok =
    "HTTP/1.0 200 OK\r\n"
    "\r\n";

static const char *msg_bad_request =
    "HTTP/1.0 400 Bad Request\r\n"
    "\r\n"
    "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
    "<html><head>\r\n"
    "<title>400 Bad Request</title>\r\n"
    "</head>\r\n"
    "<body>\r\n"
    "<h1>Bad Request</h1>\r\n"
    "<p>The requested URL has bad syntax.</p>\r\n"
    "</body>\r\n"
    "</html>\r\n";

static const char *msg_not_found =
    "HTTP/1.0 404 Not Found\r\n"
    "\r\n"
    "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
    "<html><head>\r\n"
    "<title>404 Not Found</title>\r\n"
    "</head>\r\n"
    "<body>\r\n"
    "<h1>Not Found</h1>\r\n"
    "<p>The requested URL was not found on this server.</p>\r\n"
    "</body>\r\n"
    "</html>\r\n";

static const char *msg_not_implemented =
    "HTTP/1.0 501 Not Implemented\r\n"
    "\r\n"
    "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
    "<html><head>\r\n"
    "<title>501 Not Implemented</title>\r\n"
    "</head>\r\n"
    "<body>\r\n"
    "<h1>Not Implemented</h1>\r\n"
    "<p>The requested method is not implemented on this server.</p>\r\n"
    "</body>\r\n"
    "</html>\r\n";

/** Receive one character (with buffering) */
static int recv_char(int fd, char *c)
{
	if (rbuf_out == rbuf_in) {
		rbuf_out = 0;
		rbuf_in = 0;
		
		ssize_t rc = recv(fd, rbuf, BUFFER_SIZE, 0);
		if (rc <= 0) {
			fprintf(stderr, "recv() failed (%zd)\n", rc);
			return rc;
		}
		
		rbuf_in = rc;
	}
	
	*c = rbuf[rbuf_out++];
	return EOK;
}

/** Receive one line with length limit */
static int recv_line(int fd)
{
	char *bp = lbuf;
	char c = '\0';
	
	while (bp < lbuf + BUFFER_SIZE) {
		char prev = c;
		int rc = recv_char(fd, &c);
		
		if (rc != EOK)
			return rc;
		
		*bp++ = c;
		if ((prev == '\r') && (c == '\n'))
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
	if (uri[0] != '/')
		return false;
	
	if (uri[1] == '.')
		return false;
	
	char *cp = uri + 1;
	
	while (*cp != '\0') {
		char c = *cp++;
		if (c == '/')
			return false;
	}
	
	return true;
}

static int send_response(int conn_sd, const char *msg)
{
	size_t response_size = str_size(msg);
	
	if (verbose)
	    fprintf(stderr, "Sending response\n");
	
	ssize_t rc = send(conn_sd, (void *) msg, response_size, 0);
	if (rc < 0) {
		fprintf(stderr, "send() failed\n");
		return rc;
	}
	
	return EOK;
}

static int uri_get(const char *uri, int conn_sd)
{
	if (str_cmp(uri, "/") == 0)
		uri = "/index.html";
	
	char *fname;
	int rc = asprintf(&fname, "%s%s", WEB_ROOT, uri);
	if (rc < 0)
		return ENOMEM;
	
	int fd = open(fname, O_RDONLY);
	if (fd < 0) {
		rc = send_response(conn_sd, msg_not_found);
		free(fname);
		return rc;
	}
	
	free(fname);
	
	rc = send_response(conn_sd, msg_ok);
	if (rc != EOK)
		return rc;
	
	while (true) {
		ssize_t nr = read(fd, fbuf, BUFFER_SIZE);
		if (nr == 0)
			break;
		
		if (nr < 0) {
			close(fd);
			return EIO;
		}
		
		rc = send(conn_sd, fbuf, nr, 0);
		if (rc < 0) {
			fprintf(stderr, "send() failed\n");
			close(fd);
			return rc;
		}
	}
	
	close(fd);
	
	return EOK;
}

static int req_process(int conn_sd)
{
	int rc = recv_line(conn_sd);
	if (rc != EOK) {
		fprintf(stderr, "recv_line() failed\n");
		return rc;
	}
	
	if (verbose)
		fprintf(stderr, "Request: %s", lbuf);
	
	if (str_lcmp(lbuf, "GET ", 4) != 0) {
		rc = send_response(conn_sd, msg_not_implemented);
		return rc;
	}
	
	char *uri = lbuf + 4;
	char *end_uri = str_chr(uri, ' ');
	if (end_uri == NULL) {
		end_uri = lbuf + lbuf_used - 2;
		assert(*end_uri == '\r');
	}
	
	*end_uri = '\0';
	if (verbose)
		fprintf(stderr, "Requested URI: %s\n", uri);
	
	if (!uri_is_valid(uri)) {
		rc = send_response(conn_sd, msg_bad_request);
		return rc;
	}
	
	return uri_get(uri, conn_sd);
}

static void usage(void)
{
	printf("Skeletal server\n"
	    "\n"
	    "Usage: " NAME " [options]\n"
	    "\n"
	    "Where options are:\n"
	    "-p port_number | --port=port_number\n"
	    "\tListening port (default " STRING(DEFAULT_PORT) ").\n"
	    "\n"
	    "-h | --help\n"
	    "\tShow this application help.\n"
	    "-v | --verbose\n"
	    "\tVerbose mode\n");
}

static int parse_option(int argc, char *argv[], int *index)
{
	int value;
	int rc;
	
	switch (argv[*index][1]) {
	case 'h':
		usage();
		exit(0);
		break;
	case 'p':
		rc = arg_parse_int(argc, argv, index, &value, 0);
		if (rc != EOK)
			return rc;
		
		port = (uint16_t) value;
		break;
	case 'v':
		verbose = true;
		break;
	/* Long options with double dash */
	case '-':
		if (str_lcmp(argv[*index] + 2, "help", 5) == 0) {
			usage();
			exit(0);
		} else if (str_lcmp(argv[*index] + 2, "port=", 5) == 0) {
			rc = arg_parse_int(argc, argv, index, &value, 7);
			if (rc != EOK)
				return rc;
			
			port = (uint16_t) value;
		} else if (str_cmp(argv[*index] +2, "verbose") == 0) {
			verbose = true;
		} else {
			usage();
			return EINVAL;
		}
		break;
	default:
		usage();
		return EINVAL;
	}
	
	return EOK;
}

int main(int argc, char *argv[])
{
	/* Parse command line arguments */
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			int rc = parse_option(argc, argv, &i);
			if (rc != EOK)
				return rc;
		} else {
			usage();
			return EINVAL;
		}
	}
	
	struct sockaddr_in addr;
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	
	int rc = inet_pton(AF_INET, "127.0.0.1", (void *)
	    &addr.sin_addr.s_addr);
	if (rc != EOK) {
		fprintf(stderr, "Error parsing network address (%s)\n",
		    str_error(rc));
		return 1;
	}
	
	printf("%s: HelenOS web server\n", NAME);

	if (verbose)
		fprintf(stderr, "Creating socket\n");
	
	int listen_sd = socket(PF_INET, SOCK_STREAM, 0);
	if (listen_sd < 0) {
		fprintf(stderr, "Error creating listening socket (%s)\n",
		    str_error(listen_sd));
		return 2;
	}
	
	rc = bind(listen_sd, (struct sockaddr *) &addr, sizeof(addr));
	if (rc != EOK) {
		fprintf(stderr, "Error binding socket (%s)\n",
		    str_error(rc));
		return 3;
	}
	
	rc = listen(listen_sd, BACKLOG_SIZE);
	if (rc != EOK) {
		fprintf(stderr, "listen() failed (%s)\n", str_error(rc));
		return 4;
	}
	
	fprintf(stderr, "%s: Listening for connections at port %" PRIu16 "\n",
	    NAME, port);

	task_retval(0);

	while (true) {
		struct sockaddr_in raddr;
		socklen_t raddr_len = sizeof(raddr);
		int conn_sd = accept(listen_sd, (struct sockaddr *) &raddr,
		    &raddr_len);
		
		if (conn_sd < 0) {
			fprintf(stderr, "accept() failed (%s)\n", str_error(rc));
			continue;
		}
		
		if (verbose) {
			fprintf(stderr, "Connection accepted (sd=%d), "
			    "waiting for request\n", conn_sd);
		}
		
		rbuf_out = 0;
		rbuf_in = 0;
		
		rc = req_process(conn_sd);
		if (rc != EOK)
			fprintf(stderr, "Error processing request (%s)\n",
			    str_error(rc));
		
		rc = closesocket(conn_sd);
		if (rc != EOK) {
			fprintf(stderr, "Error closing connection socket (%s)\n",
			    str_error(rc));
			closesocket(listen_sd);
			return 5;
		}
		
		if (verbose)
			fprintf(stderr, "Connection closed\n");
	}
	
	/* Not reached */
	return 0;
}

/** @}
 */
