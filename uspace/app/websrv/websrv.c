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

#include <errno.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <task.h>

#include <vfs/vfs.h>

#include <inet/addr.h>
#include <inet/endpoint.h>
#include <inet/tcp.h>

#include <arg_parse.h>
#include <macros.h>
#include <str.h>
#include <str_error.h>

#define NAME  "websrv"

#define DEFAULT_PORT  8080

#define WEB_ROOT  "/data/web"

/** Buffer for receiving the request. */
#define BUFFER_SIZE  1024

static void websrv_new_conn(tcp_listener_t *, tcp_conn_t *);

static tcp_listen_cb_t listen_cb = {
	.new_conn = websrv_new_conn
};

static tcp_cb_t conn_cb = {
	.connected = NULL
};

static uint16_t port = DEFAULT_PORT;

typedef struct {
	tcp_conn_t *conn;

	char rbuf[BUFFER_SIZE];
	size_t rbuf_out;
	size_t rbuf_in;

	char lbuf[BUFFER_SIZE + 1];
	size_t lbuf_used;
} recv_t;

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


static errno_t recv_create(tcp_conn_t *conn, recv_t **rrecv)
{
	recv_t *recv;

	recv = calloc(1, sizeof(recv_t));
	if (recv == NULL)
		return ENOMEM;

	recv->conn = conn;
	recv->rbuf_out = 0;
	recv->rbuf_in = 0;
	recv->lbuf_used = 0;

	*rrecv = recv;
	return EOK;
}

static void recv_destroy(recv_t *recv)
{
	free(recv);
}

/** Receive one character (with buffering) */
static errno_t recv_char(recv_t *recv, char *c)
{
	size_t nrecv;
	errno_t rc;

	if (recv->rbuf_out == recv->rbuf_in) {
		recv->rbuf_out = 0;
		recv->rbuf_in = 0;

		rc = tcp_conn_recv_wait(recv->conn, recv->rbuf, BUFFER_SIZE, &nrecv);
		if (rc != EOK) {
			fprintf(stderr, "tcp_conn_recv() failed: %s\n", str_error(rc));
			return rc;
		}

		recv->rbuf_in = nrecv;
	}

	*c = recv->rbuf[recv->rbuf_out++];
	return EOK;
}

/** Receive one line with length limit */
static errno_t recv_line(recv_t *recv, char **rbuf)
{
	char *bp = recv->lbuf;
	char c = '\0';

	while (bp < recv->lbuf + BUFFER_SIZE) {
		char prev = c;
		errno_t rc = recv_char(recv, &c);

		if (rc != EOK)
			return rc;

		*bp++ = c;
		if ((prev == '\r') && (c == '\n'))
			break;
	}

	recv->lbuf_used = bp - recv->lbuf;
	*bp = '\0';

	if (bp == recv->lbuf + BUFFER_SIZE)
		return ELIMIT;

	*rbuf = recv->lbuf;
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

static errno_t send_response(tcp_conn_t *conn, const char *msg)
{
	size_t response_size = str_size(msg);

	if (verbose)
	    fprintf(stderr, "Sending response\n");

	errno_t rc = tcp_conn_send(conn, (void *) msg, response_size);
	if (rc != EOK) {
		fprintf(stderr, "tcp_conn_send() failed\n");
		return rc;
	}

	return EOK;
}

static errno_t uri_get(const char *uri, tcp_conn_t *conn)
{
	char *fbuf = NULL;
	char *fname = NULL;
	errno_t rc;
	size_t nr;
	int fd = -1;

	fbuf = calloc(BUFFER_SIZE, 1);
	if (fbuf == NULL) {
		rc = ENOMEM;
		goto out;
	}

	if (str_cmp(uri, "/") == 0)
		uri = "/index.html";

	if (asprintf(&fname, "%s%s", WEB_ROOT, uri) < 0) {
		rc = ENOMEM;
		goto out;
	}

	rc = vfs_lookup_open(fname, WALK_REGULAR, MODE_READ, &fd);
	if (rc != EOK) {
		rc = send_response(conn, msg_not_found);
		goto out;
	}

	free(fname);
	fname = NULL;

	rc = send_response(conn, msg_ok);
	if (rc != EOK)
		goto out;

	aoff64_t pos = 0;
	while (true) {
		rc = vfs_read(fd, &pos, fbuf, BUFFER_SIZE, &nr);
		if (rc != EOK)
			goto out;

		if (nr == 0)
			break;

		rc = tcp_conn_send(conn, fbuf, nr);
		if (rc != EOK) {
			fprintf(stderr, "tcp_conn_send() failed\n");
			goto out;
		}
	}

	rc = EOK;
out:
	if (fd >= 0)
		vfs_put(fd);
	free(fname);
	free(fbuf);
	return rc;
}

static errno_t req_process(tcp_conn_t *conn, recv_t *recv)
{
	char *reqline = NULL;

	errno_t rc = recv_line(recv, &reqline);
	if (rc != EOK) {
		fprintf(stderr, "recv_line() failed\n");
		return rc;
	}

	if (verbose)
		fprintf(stderr, "Request: %s", reqline);

	if (str_lcmp(reqline, "GET ", 4) != 0) {
		rc = send_response(conn, msg_not_implemented);
		return rc;
	}

	char *uri = reqline + 4;
	char *end_uri = str_chr(uri, ' ');
	if (end_uri == NULL) {
		end_uri = reqline + str_size(reqline) - 2;
		assert(*end_uri == '\r');
	}

	*end_uri = '\0';
	if (verbose)
		fprintf(stderr, "Requested URI: %s\n", uri);

	if (!uri_is_valid(uri)) {
		rc = send_response(conn, msg_bad_request);
		return rc;
	}

	return uri_get(uri, conn);
}

static void usage(void)
{
	printf("Simple web server\n"
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

static errno_t parse_option(int argc, char *argv[], int *index)
{
	int value;
	errno_t rc;

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

static void websrv_new_conn(tcp_listener_t *lst, tcp_conn_t *conn)
{
	errno_t rc;
	recv_t *recv = NULL;

	if (verbose)
		fprintf(stderr, "New connection, waiting for request\n");

	rc = recv_create(conn, &recv);
	if (rc != EOK) {
		fprintf(stderr, "Out of memory.\n");
		goto error;
	}

	rc = req_process(conn, recv);
	if (rc != EOK) {
		fprintf(stderr, "Error processing request (%s)\n",
		    str_error(rc));
		goto error;
	}

	rc = tcp_conn_send_fin(conn);
	if (rc != EOK) {
		fprintf(stderr, "Error sending FIN.\n");
		goto error;
	}

	recv_destroy(recv);
	return;
error:
	rc = tcp_conn_reset(conn);
	if (rc != EOK)
		fprintf(stderr, "Error resetting connection.\n");

	recv_destroy(recv);
}

int main(int argc, char *argv[])
{
	inet_ep_t ep;
	tcp_listener_t *lst;
	tcp_t *tcp;
	errno_t rc;

	/* Parse command line arguments */
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			rc = parse_option(argc, argv, &i);
			if (rc != EOK)
				return rc;
		} else {
			usage();
			return EINVAL;
		}
	}

	printf("%s: HelenOS web server\n", NAME);

	if (verbose)
		fprintf(stderr, "Creating listener\n");

	inet_ep_init(&ep);
	ep.port = port;

	rc = tcp_create(&tcp);
	if (rc != EOK) {
		fprintf(stderr, "Error initializing TCP.\n");
		return 1;
	}

	rc = tcp_listener_create(tcp, &ep, &listen_cb, NULL, &conn_cb, NULL,
	    &lst);
	if (rc != EOK) {
		fprintf(stderr, "Error creating listener.\n");
		return 2;
	}

	fprintf(stderr, "%s: Listening for connections at port %" PRIu16 "\n",
	    NAME, port);

	task_retval(0);
	async_manager();

	/* Not reached */
	return 0;
}

/** @}
 */
