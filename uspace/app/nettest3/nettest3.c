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
#include <stdlib.h>
#include <str.h>

#include <inet/dnsr.h>
#include <net/in.h>
#include <net/in6.h>
#include <net/inet.h>
#include <net/socket.h>

#define BUF_SIZE 32

static char *data;
static size_t size;

static char buf[BUF_SIZE];

static struct sockaddr *address;
static socklen_t addrlen;

static uint16_t port;

int main(int argc, char *argv[])
{
	int rc;
	int fd;
	char *endptr;
	dnsr_hostinfo_t *hinfo;
	inet_addr_t addr;
	char *addr_s;

	port = 7;

	data = (char *) "Hello World!";
	size = str_size(data);

	/* Connect to local IP address by default */
	inet_addr(&addr, 127, 0, 0, 1);

	if (argc >= 2) {
		printf("parsing address '%s'\n", argv[1]);
		rc = inet_addr_parse(argv[1], &addr);
		if (rc != EOK) {
			/* Try interpreting as a host name */
			rc = dnsr_name2host(argv[1], &hinfo, ip_v4);
			if (rc != EOK) {
				printf("Error resolving host '%s'.\n", argv[1]);
				return rc;
			}

			addr = hinfo->addr;
		}
		rc = inet_addr_format(&addr, &addr_s);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			printf("Out of memory.\n");
			return rc;
		}
		printf("result: rc=%d, ver=%d, addr=%s\n", rc,
		    addr.version, addr_s);
		free(addr_s);
	}

	if (argc >= 3) {
		printf("parsing port '%s'\n", argv[2]);
		port = htons(strtoul(argv[2], &endptr, 10));
		if (*endptr != '\0') {
			fprintf(stderr, "Error parsing port\n");
			return 1;
		}
	}

	rc = inet_addr_sockaddr(&hinfo->addr, port, &address, &addrlen);
	if (rc != EOK) {
		printf("Out of memory.\n");
		return rc;
	}

	printf("socket()\n");
	fd = socket(address->sa_family, SOCK_STREAM, IPPROTO_TCP);
	printf(" -> %d\n", fd);
	if (fd < 0)
		return 1;

	printf("connect()\n");
	rc = connect(fd, address, addrlen);
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

	async_usleep(1000 * 1000);

	printf("closesocket()\n");
	rc = closesocket(fd);
	printf(" -> %d\n", rc);

	free(address);

	return 0;
}

/** @}
 */
