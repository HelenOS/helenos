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
#include <errno.h>
#include <fibril.h>
#include <inet/endpoint.h>
#include <inet/hostport.h>
#include <inet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <str_error.h>

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
	errno_t rc;
	size_t nrecv;

	while (true) {
		rc = tcp_conn_recv(conn, recv_buf, RECV_BUF_SIZE, &nrecv);
		if (rc != EOK) {
			printf("\n[Receive error: %s]\n", str_error_name(rc));
			break;
		}

		nterm_received(recv_buf, nrecv);

		if (nrecv != RECV_BUF_SIZE)
			break;
	}
}

errno_t conn_open(const char *hostport)
{
	inet_ep2_t epp;
	const char *errmsg;
	errno_t rc;

	inet_ep2_init(&epp);
	rc = inet_hostport_plookup_one(hostport, ip_any, &epp.remote, NULL,
	    &errmsg);
	if (rc != EOK) {
		printf("Error: %s (host:port %s).\n", errmsg, hostport);
		goto error;
	}

	printf("Connecting to %s\n", hostport);
	char *s;
	rc = inet_addr_format(&epp.remote.addr, &s);
	if (rc != EOK)
		goto error;

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

errno_t conn_send(void *data, size_t size)
{
	errno_t rc = tcp_conn_send(conn, data, size);
	if (rc != EOK)
		return EIO;

	return EOK;
}

/** @}
 */
