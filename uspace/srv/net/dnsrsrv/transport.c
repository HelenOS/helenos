/*
 * Copyright (c) 2013 Jiri Svoboda
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

/** @addtogroup dnsres
 * @{
 */
/**
 * @file
 */

#include <errno.h>
#include <net/in.h>
#include <net/inet.h>
#include <net/socket.h>
#include <stdlib.h>

#include "dns_msg.h"
#include "dns_type.h"
#include "transport.h"

#include <stdio.h>

#define RECV_BUF_SIZE 4096

static uint8_t recv_buf[RECV_BUF_SIZE];

int dns_request(dns_message_t *req, dns_message_t **rresp)
{
	dns_message_t *resp;
	int rc;
	void *req_data;
	size_t req_size;
	struct sockaddr_in addr;
	struct sockaddr_in laddr;
	struct sockaddr_in src_addr;
	socklen_t src_addr_size;
	size_t recv_size;
	int fd;
	int i;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(53);
	addr.sin_addr.s_addr = htonl((10 << 24) | (0 << 16) | (0 << 8) | 138);

	laddr.sin_family = AF_INET;
	laddr.sin_port = htons(12345);
	laddr.sin_addr.s_addr = INADDR_ANY;

	req_data = NULL;
	fd = -1;

	rc = dns_message_encode(req, &req_data, &req_size);
	if (rc != EOK)
		goto error;

	fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0) {
		rc = EIO;
		goto error;
	}

	rc = bind(fd, (struct sockaddr *)&laddr, sizeof(laddr));
	if (rc != EOK)
		goto error;

	printf("fd=%d req_data=%p, req_size=%zu\n", fd, req_data, req_size);
	rc = sendto(fd, req_data, req_size, 0, (struct sockaddr *)&addr,
	    sizeof(addr));
	if (rc != EOK)
		goto error;

	src_addr_size = sizeof(src_addr);
	rc = recvfrom(fd, recv_buf, RECV_BUF_SIZE, 0,
	    (struct sockaddr *)&src_addr, &src_addr_size);
	if (rc < 0) {
		printf("recvfrom returns error - %d\n", rc);
		goto error;
	}

	recv_size = (size_t)rc;

	printf("received %d bytes\n", (int)recv_size);
	for (i = 0; i < (int)recv_size; i++) {
		if (recv_buf[i] >= 32 && recv_buf[i] < 127)
			printf("%c", recv_buf[i]);
		else
			printf("?");
	}
	printf("\n");

	printf("close socket\n");
	closesocket(fd);
	printf("free req_data\n");
	free(req_data);

	rc = dns_message_decode(recv_buf, recv_size, &resp);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	*rresp = resp;
	return EOK;

error:
	if (req_data != NULL)
		free(req_data);
	if (fd >= 0)
		closesocket(fd);
	return rc;
}

/** @}
 */
