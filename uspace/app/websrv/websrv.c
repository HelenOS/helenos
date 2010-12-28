/*
 * Copyright (c) 2010 Jiri Svoboda
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
 * @file (Less-than-skeleton) web server.
 */

#include <stdio.h>

#include <net/in.h>
#include <net/inet.h>
#include <net/socket.h>

#include <str.h>

#define PORT_NUMBER 8080

/** Buffer for receiving the request. */
#define BUFFER_SIZE 1024
static char buf[BUFFER_SIZE];

/** Response to send to client. */
static const char *response_msg =
    "HTTP/1.0 200 OK\r\n"
    "\r\n"
    "<h1>Hello from HelenOS!</h1>\r\n";

int main(int argc, char *argv[])
{
	struct sockaddr_in addr;
	struct sockaddr_in raddr;

	socklen_t raddr_len;

	int listen_sd, conn_sd;
	int rc;

	size_t response_size;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT_NUMBER);

	rc = inet_pton(AF_INET, "127.0.0.1", (void *) &addr.sin_addr.s_addr);
	if (rc != EOK) {
		printf("Error parsing network address.\n");
		return 1;
	}

	printf("Creating socket.\n");

	listen_sd = socket(PF_INET, SOCK_STREAM, 0);
	if (listen_sd < 0) {
		printf("Error creating listening socket.\n");
		return 1;
	}

	rc = bind(listen_sd, (struct sockaddr *) &addr, sizeof(addr));
	if (rc != EOK) {
		printf("Error binding socket.\n");
		return 1;
	}

	rc = listen(listen_sd, 1);
	if (rc != EOK) {
		printf("Error calling listen() (%d).\n", rc);
		return 1;
	}

	response_size = str_size(response_msg);

	printf("Listening for connections at port number %u.\n", PORT_NUMBER);
	while (true) {
		raddr_len = sizeof(raddr);
		conn_sd = accept(listen_sd, (struct sockaddr *) &raddr,
		    &raddr_len);

		if (conn_sd < 0) {
			printf("accept() failed.\n");
			return 1;
		}

		printf("Accepted connection, sd=%d.\n", conn_sd);

		printf("Wait for client request\n");

		/* Really we should wait for a blank line. */
		rc = recv(conn_sd, buf, BUFFER_SIZE, 0);
		if (rc < 0) {
			printf("recv() failed\n");
			return 1;
		}

		/* Send a canned response. */
                printf("Send response...\n");
		rc = send(conn_sd, (void *) response_msg, response_size, 0);
		if (rc < 0) {
			printf("send() failed.\n");
			return 1;
		}

		rc = closesocket(conn_sd);
		if (rc != EOK) {
			printf("Error closing connection socket: %d\n", rc);
			return 1;
		}

		printf("Closed connection.\n");
	}

	/* Not reached. */
	return 0;
}

/** @}
 */
