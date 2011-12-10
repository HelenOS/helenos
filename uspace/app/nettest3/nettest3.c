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

/** @addtogroup nettest
 * @{
 */

/** @file
 * Networking test 3.
 */

#include <async.h>
#include <stdio.h>
#include <str.h>

#include <net/in.h>
#include <net/in6.h>
#include <net/inet.h>
#include <net/socket.h>

#define BUF_SIZE 32

static char *data;
static size_t size;

static char buf[BUF_SIZE];

static struct sockaddr_in addr;

static uint16_t port;

int main(int argc, char *argv[])
{
	int rc;
	int fd;
	char *endptr;

	port = 7;

	data = (char *)"Hello World!";
	size = str_size(data);

	/* Connect to local IP address by default */
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(0x7f000001);

	if (argc >= 2) {
		printf("parsing address '%s'\n", argv[1]);
		rc = inet_pton(AF_INET, argv[1], (uint8_t *)&addr.sin_addr.s_addr);
		if (rc != EOK) {
			fprintf(stderr, "Error parsing address\n");
			return 1;
		}
		printf("result: rc=%d, family=%d, addr=%x\n", rc,
		    addr.sin_family, addr.sin_addr.s_addr);
	}

	if (argc >= 3) {
		printf("parsing port '%s'\n", argv[2]);
		addr.sin_port = htons(strtoul(argv[2], &endptr, 10));
		if (*endptr != '\0') {
			fprintf(stderr, "Error parsing port\n");
			return 1;
		}
	}

	printf("socket()\n");
	fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	printf(" -> %d\n", fd);
	if (fd < 0)
		return 1;

	printf("connect()\n");
	rc = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
	printf(" -> %d\n", rc);
	if (rc != 0)
		return 1;

	printf("send()\n");
	rc = send(fd, data, size, 0);
	printf(" -> %d\n", rc);
	if (rc < 0)
		return 1;

	do {
		printf("recv()\n");
		rc = recv(fd, buf, BUF_SIZE, 0);
		printf(" -> %d\n", rc);
	} while (rc > 0);

	async_usleep(1000*1000);

	printf("closesocket()\n");
	rc = closesocket(fd);
	printf(" -> %d\n", rc);

	return 0;
}


/** @}
 */
