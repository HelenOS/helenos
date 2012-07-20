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

#include <bool.h>
#include <errno.h>
#include <fibril.h>
#include <net/socket.h>
#include <stdio.h>
#include <str_error.h>
#include <sys/types.h>

#include "conn.h"
#include "nterm.h"

static int conn_fd;
static fid_t rcv_fid;

#define RECV_BUF_SIZE 1024
static uint8_t recv_buf[RECV_BUF_SIZE];

static int rcv_fibril(void *arg)
{
	ssize_t nr;

	while (true) {
		nr = recv(conn_fd, recv_buf, RECV_BUF_SIZE, 0);
		if (nr < 0)
			break;

		nterm_received(recv_buf, nr);
	}

	if (nr == ENOTCONN)
		printf("\n[Other side has closed the connection]\n");
	else
		printf("'\n[Receive errror (%s)]\n", str_error(nr));

	exit(0);
	return 0;
}

int conn_open(const char *addr_s, const char *port_s)
{
	struct sockaddr_in addr;
	int rc;
	char *endptr;

	addr.sin_family = AF_INET;

	rc = inet_pton(addr.sin_family, addr_s, (uint8_t *)&addr.sin_addr);
	if (rc != EOK) {
		printf("Invalid addres %s\n", addr_s);
		return EINVAL;
	}

	addr.sin_port = htons(strtol(port_s, &endptr, 10));
	if (*endptr != '\0') {
		printf("Invalid port number %s\n", port_s);
		return EINVAL;
	}

	conn_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (conn_fd < 0)
		goto error;

	printf("Connecting to address %s port %u\n", addr_s, ntohs(addr.sin_port));

	rc = connect(conn_fd, (struct sockaddr *)&addr, sizeof(addr));
	if (rc != EOK)
		goto error;

	rcv_fid = fibril_create(rcv_fibril, NULL);
	if (rcv_fid == 0)
		goto error;

	fibril_add_ready(rcv_fid);

	return EOK;

error:
	if (conn_fd >= 0) {
		closesocket(conn_fd);
		conn_fd = -1;
	}

	return EIO;
}

int conn_send(void *data, size_t size)
{
	int rc;

	rc = send(conn_fd, data, size, 0);
	if (rc != EOK)
		return EIO;

	return EOK;
}

/** @}
 */
