/*
 * Copyright (c) 2015 Jiri Svoboda
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

/** @addtogroup tcp
 * @{
 */

/**
 * @file Internal TCP test
 */

#include <async.h>
#include <errno.h>
#include <stdio.h>
#include <fibril.h>
#include <str.h>
#include "tcp_type.h"
#include "ucall.h"

#include "test.h"

#define RCV_BUF_SIZE 64

static errno_t test_srv(void *arg)
{
	tcp_conn_t *conn;
	inet_ep2_t epp;
	char rcv_buf[RCV_BUF_SIZE + 1];
	size_t rcvd;
	xflags_t xflags;

	printf("test_srv()\n");

	inet_ep2_init(&epp);

	inet_addr(&epp.local.addr, 127, 0, 0, 1);
	epp.local.port = 80;

	inet_addr(&epp.remote.addr, 127, 0, 0, 1);
	epp.remote.port = 1024;

	printf("S: User open...\n");
	tcp_uc_open(&epp, ap_passive, 0, &conn);
	conn->name = (char *) "S";

	while (true) {
		printf("S: User receive...\n");
		tcp_uc_receive(conn, rcv_buf, RCV_BUF_SIZE, &rcvd, &xflags);
		if (rcvd == 0) {
			printf("S: End of data reached.\n");
			break;
		}
		rcv_buf[rcvd] = '\0';
		printf("S: User received %zu bytes '%s'.\n", rcvd, rcv_buf);

		async_usleep(1000 * 1000 * 2);
	}

	async_usleep(/*10**/1000 * 1000);

	printf("S: User close...\n");
	tcp_uc_close(conn);

	printf("test_srv() terminating\n");
	return 0;
}

static errno_t test_cli(void *arg)
{
	tcp_conn_t *conn;
	inet_ep2_t epp;
	const char *msg = "Hello World!";

	printf("test_cli()\n");

	inet_ep2_init(&epp);

	inet_addr(&epp.local.addr, 127, 0, 0, 1);
	epp.local.port = 1024;

	inet_addr(&epp.remote.addr, 127, 0, 0, 1);
	epp.remote.port = 80;

	async_usleep(1000 * 1000 * 3);
	printf("C: User open...\n");
	tcp_uc_open(&epp, ap_active, 0, &conn);
	conn->name = (char *) "C";

	async_usleep(1000 * 1000 * 10);
	printf("C: User send...\n");
	tcp_uc_send(conn, (void *)msg, str_size(msg), 0);

	async_usleep(1000 * 1000 * 20/**20*2*/);
	printf("C: User close...\n");
	tcp_uc_close(conn);

	return 0;
}

void tcp_test(void)
{
	fid_t srv_fid;
	fid_t cli_fid;

	printf("tcp_test()\n");

	async_usleep(1000 * 1000);

	if (0) {
		srv_fid = fibril_create(test_srv, NULL);
		if (srv_fid == 0) {
			printf("Failed to create server fibril.\n");
			return;
		}

		fibril_add_ready(srv_fid);
	}

	if (0) {
		cli_fid = fibril_create(test_cli, NULL);
		if (cli_fid == 0) {
			printf("Failed to create client fibril.\n");
			return;
		}

		fibril_add_ready(cli_fid);
	}
}

/**
 * @}
 */
