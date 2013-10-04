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
#include <net/inet.h>
#include <net/socket.h>
#include <stdio.h>
#include <stdlib.h>
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

int conn_open(const char *host, const char *port_s)
{
	int conn_fd = -1;
	struct sockaddr *saddr = NULL;
	socklen_t saddrlen;
	
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
	
	rc = inet_addr_sockaddr(&iaddr, port, &saddr, &saddrlen);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		return ENOMEM;
	}
	
	printf("Connecting to host %s port %u\n", host, port);
	
	conn_fd = socket(saddr->sa_family, SOCK_STREAM, 0);
	if (conn_fd < 0)
		goto error;
	
	rc = connect(conn_fd, saddr, saddrlen);
	if (rc != EOK)
		goto error;
	
	rcv_fid = fibril_create(rcv_fibril, NULL);
	if (rcv_fid == 0)
		goto error;
	
	fibril_add_ready(rcv_fid);
	
	free(saddr);
	return EOK;
error:
	if (conn_fd >= 0) {
		closesocket(conn_fd);
		conn_fd = -1;
	}
	free(saddr);
	
	return EIO;
}

int conn_send(void *data, size_t size)
{
	int rc = send(conn_fd, data, size, 0);
	if (rc != EOK)
		return EIO;
	
	return EOK;
}

/** @}
 */
