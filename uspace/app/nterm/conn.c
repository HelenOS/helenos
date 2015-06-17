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

/** @addtogroup nterm
 * @{
 */
/** @file
 */

#include <byteorder.h>
#include <stdbool.h>
#include <errno.h>
#include <fibril.h>
#include <inet/dnsr.h>
#include <inet/endpoint.h>
#include <inet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>
#include <sys/types.h>

#include "conn.h"
#include "nterm.h"

static tcp_t *tcp;
static tcp_conn_t *conn;

#define RECV_BUF_SIZE 1024
static uint8_t recv_buf[RECV_BUF_SIZE];

static void conn_conn_reset(tcp_conn_t *);
static void conn_data_avail(tcp_conn_t *);

static tcp_cb_t conn_cb = {
	.conn_reset = conn_conn_reset,
	.data_avail = conn_data_avail
};

static void conn_conn_reset(tcp_conn_t *conn)
{
	printf("\n[Connection reset]\n");
}

static void conn_data_avail(tcp_conn_t *conn)
{
	int rc;
	size_t nrecv;

	while (true) {
		rc = tcp_conn_recv(conn, recv_buf, RECV_BUF_SIZE, &nrecv);
		if (rc != EOK) {
			printf("\n[Receive error %d]\n", rc);
			break;
		}

		nterm_received(recv_buf, nrecv);

		if (nrecv != RECV_BUF_SIZE)
			break;
	}
}

int conn_open(const char *host, const char *port_s)
{
	inet_ep2_t epp;

	/* Interpret as address */
	inet_addr_t iaddr;
	int rc = inet_addr_parse(host, &iaddr);

	if (rc != EOK) {
		/* Interpret as a host name */
		dnsr_hostinfo_t *hinfo = NULL;
		rc = dnsr_name2host(host, &hinfo, ip_any);

		if (rc != EOK) {
			printf("Error resolving host '%s'.\n", host);
			goto error;
		}

		iaddr = hinfo->addr;
	}

	char *endptr;
	uint16_t port = strtol(port_s, &endptr, 10);
	if (*endptr != '\0') {
		printf("Invalid port number %s\n", port_s);
		goto error;
	}

	inet_ep2_init(&epp);
	epp.remote.addr = iaddr;
	epp.remote.port = port;

	printf("Connecting to host %s port %u\n", host, port);

	rc = tcp_create(&tcp);
	if (rc != EOK)
		goto error;

	rc = tcp_conn_create(tcp, &epp, &conn_cb, NULL, &conn);
	if (rc != EOK)
		goto error;

	rc = tcp_conn_wait_connected(conn);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	tcp_conn_destroy(conn);
	tcp_destroy(tcp);

	return EIO;
}

int conn_send(void *data, size_t size)
{
	int rc = tcp_conn_send(conn, data, size);
	if (rc != EOK)
		return EIO;

	return EOK;
}

/** @}
 */
