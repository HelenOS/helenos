/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

		fibril_usleep(1000 * 1000 * 2);
	}

	fibril_usleep(1000 * 1000);

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

	fibril_usleep(1000 * 1000 * 3);
	printf("C: User open...\n");
	tcp_uc_open(&epp, ap_active, 0, &conn);
	conn->name = (char *) "C";

	fibril_usleep(1000 * 1000 * 10);
	printf("C: User send...\n");
	tcp_uc_send(conn, (void *)msg, str_size(msg), 0);

	fibril_usleep(1000 * 1000 * 20);
	printf("C: User close...\n");
	tcp_uc_close(conn);

	return 0;
}

void tcp_test(void)
{
	fid_t srv_fid;
	fid_t cli_fid;

	printf("tcp_test()\n");

	fibril_usleep(1000 * 1000);

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
