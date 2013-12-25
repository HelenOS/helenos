/*
 * Copyright (c) 2013 Martin Sucha
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

/** @addtogroup netspeed
 * @{
 */

/** @file
 * Network speed measurement (iperf counterpart)
 *
 */

#include <assert.h>
#include <inet/dnsr.h>
#include <net/in.h>
#include <net/inet.h>
#include <net/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>
#include <task.h>

#define NAME "netspeed"

static int server(sock_type_t sock_type, unsigned port, void *buf, size_t bufsize)
{
	struct sockaddr_in addr;
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	
	int rc = inet_pton(AF_INET, "127.0.0.1", (void *)
	    &addr.sin_addr.s_addr);
	if (rc != EOK) {
		fprintf(stderr, "inet_pton failed: %s\n", str_error(rc));
		return rc;
	}

	int listen_sd = socket(PF_INET, sock_type, 0);
	if (listen_sd < 0) {
		fprintf(stderr, "socket failed: %s\n", str_error(rc));
		return rc;
	}
	
	rc = bind(listen_sd, (struct sockaddr *) &addr, sizeof(addr));
	if (rc != EOK) {
		fprintf(stderr, "bind failed: %s\n", str_error(rc));
		closesocket(listen_sd);
		return rc;
	}
	
	rc = listen(listen_sd, 2);
	if (rc != EOK) {
		fprintf(stderr, "listen failed: %s\n", str_error(rc));
		closesocket(listen_sd);
		return rc;
	}
	
	int conn_sd;
	struct sockaddr_in raddr;
	socklen_t raddr_len = sizeof(raddr);
	if (sock_type == SOCK_STREAM) {
		conn_sd = accept(listen_sd, (struct sockaddr *) &raddr,
		    &raddr_len);
		if (conn_sd < 0) {
			fprintf(stderr, "accept failed: %s\n", str_error(conn_sd));
			closesocket(listen_sd);
			return conn_sd;
		}
	} else {
		conn_sd = listen_sd;
	}
	
	while (true) {
		rc = recvfrom(conn_sd, buf, bufsize, 0, (struct sockaddr *) &raddr,
		    &raddr_len);
		if (rc <= 0) {
			if (rc < 0)
				fprintf(stderr, "recvfrom failed: %s\n", str_error(rc));
			break;
		}
	}
	
	if (sock_type == SOCK_STREAM)
		closesocket(conn_sd);
	closesocket(listen_sd);
	
	return rc;
}

static int client(sock_type_t sock_type, const char *host, unsigned port,
    unsigned long count, char *buf, size_t bufsize)
{
	inet_addr_t iaddr;
	struct sockaddr *saddr;
	socklen_t saddrlen;
	
	int rc = inet_addr_parse(host, &iaddr);
	if (rc != EOK) {
		dnsr_hostinfo_t *hinfo = NULL;
		rc = dnsr_name2host(host, &hinfo, ip_any);
		if (rc != EOK) {
			fprintf(stderr, "Error resolving host '%s'.\n", host);
			return ENOENT;
		}
		
		iaddr = hinfo->addr;
	}
	
	rc = inet_addr_sockaddr(&iaddr, port, &saddr, &saddrlen);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		fprintf(stderr, "Out of memory.\n");
		return ENOMEM;
	}
	
	int conn_sd = socket(saddr->sa_family, sock_type, 0);
	if (conn_sd < 0) {
		fprintf(stderr, "socket failed: %s\n", str_error(rc));
		return rc;
	}
	
	if (sock_type == SOCK_STREAM) {
		rc = connect(conn_sd, saddr, saddrlen);
		if (rc != EOK) {
			fprintf(stderr, "connect failed: %s\n", str_error(rc));
			closesocket(conn_sd);
			return rc;
		}
	}
	
	for (size_t pos = 0; pos < bufsize; pos++) {
		buf[pos] = '0' + (pos % 10);
	}
	
	for (unsigned long i = 0; i < count; i++) {
		if (sock_type == SOCK_STREAM) {
			rc = send(conn_sd, buf, bufsize, 0);
		} else {
			rc = sendto(conn_sd, buf, bufsize, 0, saddr, saddrlen);
		}
		if (rc != EOK) {
			fprintf(stderr, "send failed: %s\n", str_error(rc));
			break;
		}
	}
	
	closesocket(conn_sd);
	free(saddr);
	return rc;
}

static void syntax_print(void)
{
	fprintf(stderr, "Usage: netspeed <tcp|udp> server [port] <buffer size>\n");
	fprintf(stderr, "       netspeed <tcp|udp> client <host> <port> <count> <buffer size>\n");
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		syntax_print();
		return 2;
	}
	
	sock_type_t sock_type;
	if (str_cmp(argv[1], "tcp") == 0) {
		sock_type = SOCK_STREAM;
	} else if (str_cmp(argv[1], "udp") == 0) {
		sock_type = SOCK_DGRAM;
	} else {
		fprintf(stderr, "Invalid socket type\n");
		syntax_print();
		return 1;
	}
	
	char *endptr;
	int arg = 3;
	unsigned long port = 5001;
	const char *address;
	bool is_server;
	unsigned long count = 0;
	
	if (str_cmp(argv[2], "server") == 0) {
		is_server = true;
		if (argc < 4) {
			syntax_print();
			return 2;
		}
		if (argc > 4) {
			port = strtoul(argv[3], &endptr, 0);
			if (*endptr != 0) {
				fprintf(stderr, "Invalid port number\n");
				syntax_print();
				return 1;
			}
			arg = 4;
		}
	}
	else if (str_cmp(argv[2], "client") == 0) {
		is_server = false;
		if (argc < 6) {
			syntax_print();
			return 2;
		}
		address = argv[3];
		port = strtoul(argv[4], &endptr, 0);
		if (*endptr != 0) {
			fprintf(stderr, "Invalid port number\n");
			syntax_print();
			return 1;
		}
		
		count = strtoul(argv[5], &endptr, 0);
		if (*endptr != 0) {
			fprintf(stderr, "Invalid count\n");
			syntax_print();
			return 1;
		}
		
		arg = 6;
	}
	else {
		fprintf(stderr, "Invalid client/server mode\n");
		syntax_print();
		return 2;
	}
	
	if (argc < arg + 1) {
		syntax_print();
		return 2;
	}
	
	unsigned long bufsize = strtoul(argv[arg], &endptr, 0);
	if (*endptr != 0 || bufsize == 0) {
		fprintf(stderr, "Invalid buffer size\n");
		syntax_print();
		return 1;
	}
	
	void *buf = malloc(bufsize);
	if (buf == NULL) {
		fprintf(stderr, "Cannot allocate buffer\n");
		return ENOMEM;
	}
	
	int rc = EOK;
	if (is_server) {
		rc = server(sock_type, port, buf, bufsize);
		if (rc != EOK)
			fprintf(stderr, "Server failed: %s\n", str_error(rc));
	} else {
		rc = client(sock_type, address, port, count, buf, bufsize);
		if (rc != EOK)
			fprintf(stderr, "Client failed: %s\n", str_error(rc));
	}
	
	free(buf);

	return rc;
}

/** @}
 */
