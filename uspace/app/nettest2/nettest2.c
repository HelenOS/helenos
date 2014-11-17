/*
 * Copyright (c) 2009 Lukas Mejdrech
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
 * Networking test 2 application - transfer.
 */

#include "nettest.h"
#include "print_error.h"

#include <assert.h>
#include <malloc.h>
#include <stdio.h>
#include <unistd.h>
#include <str.h>
#include <task.h>
#include <time.h>
#include <arg_parse.h>
#include <stdbool.h>

#include <inet/dnsr.h>
#include <net/in.h>
#include <net/in6.h>
#include <net/inet.h>
#include <net/socket.h>
#include <net/socket_parse.h>

/** Echo module name. */
#define NAME  "nettest2"

/** Packet data pattern. */
#define NETTEST2_TEXT  "Networking test 2 - transfer"

static uint16_t family = AF_NONE;
static size_t size = 28;
static bool verbose = false;
static sock_type_t type = SOCK_DGRAM;
static int sockets = 10;
static int messages = 10;
static uint16_t port = 7;

static void nettest2_print_help(void)
{
	printf(
	    "Network Networking test 2 aplication - UDP transfer\n"
	    "Usage: nettest2 [options] host\n"
	    "Where options are:\n"
	    "-f protocol_family | --family=protocol_family\n"
	    "\tThe listenning socket protocol family. Only the PF_INET and "
	    "PF_INET6 are supported.\n"
	    "\n"
	    "-h | --help\n"
	    "\tShow this application help.\n"
	    "\n"
	    "-m count | --messages=count\n"
	    "\tThe number of messages to send and receive per socket. The "
	    "default is 10.\n"
	    "\n"
	    "-n sockets | --sockets=count\n"
	    "\tThe number of sockets to use. The default is 10.\n"
	    "\n"
	    "-p port_number | --port=port_number\n"
	    "\tThe port number the application should send messages to. The "
	    "default is 7.\n"
	    "\n"
	    "-s packet_size | --size=packet_size\n"
	    "\tThe packet data size the application sends. The default is 29 "
	    "bytes.\n"
	    "\n"
	    "-v | --verbose\n"
	    "\tShow all output messages.\n");
}

/** Fill buffer with the NETTEST2_TEXT pattern.
 *
 * @param buffer	Data buffer.
 * @param size		Buffer size in bytes.
 */
static void nettest2_fill_buffer(char *buffer, size_t size)
{
	size_t length = 0;
	while (size > length + sizeof(NETTEST2_TEXT) - 1) {
		memcpy(buffer + length, NETTEST2_TEXT,
		    sizeof(NETTEST2_TEXT) - 1);
		length += sizeof(NETTEST2_TEXT) - 1;
	}
	
	memcpy(buffer + length, NETTEST2_TEXT, size - length);
	buffer[size] = '\0';
}

/** Parse one command-line option.
 *
 * @param argc		Number of all command-line arguments.
 * @param argv		All command-line arguments.
 * @param index		Current argument index (in, out).
 */
static int nettest2_parse_opt(int argc, char *argv[], int *index)
{
	int value;
	int rc;
	
	switch (argv[*index][1]) {
	/*
	 * Short options with only one letter
	 */
	case 'f':
		rc = arg_parse_name_int(argc, argv, index, &value, 0,
		    socket_parse_protocol_family);
		if (rc != EOK)
			return rc;
		
		family = (uint16_t) value;
		break;
	case 'h':
		nettest2_print_help();
		return EOK;
	case 'm':
		rc = arg_parse_int(argc, argv, index, &messages, 0);
		if (rc != EOK)
			return rc;
		
		break;
	case 'n':
		rc = arg_parse_int(argc, argv, index, &sockets, 0);
		if (rc != EOK)
			return rc;
		
		break;
	case 'p':
		rc = arg_parse_int(argc, argv, index, &value, 0);
		if (rc != EOK)
			return rc;
		
		port = (uint16_t) value;
		break;
	case 's':
		rc = arg_parse_int(argc, argv, index, &value, 0);
		if (rc != EOK)
			return rc;
		
		size = (value >= 0) ? (size_t) value : 0;
		break;
	case 't':
		rc = arg_parse_name_int(argc, argv, index, &value, 0,
		    socket_parse_socket_type);
		if (rc != EOK)
			return rc;
		
		type = (sock_type_t) value;
		break;
	case 'v':
		verbose = true;
		break;
	
	/*
	 * Long options with double dash ('-')
	 */
	case '-':
		if (str_lcmp(argv[*index] + 2, "family=", 7) == 0) {
			rc = arg_parse_name_int(argc, argv, index, &value, 9,
			    socket_parse_protocol_family);
			if (rc != EOK)
				return rc;
			
			family = (uint16_t) value;
		} else if (str_lcmp(argv[*index] + 2, "help", 5) == 0) {
			nettest2_print_help();
			return EOK;
		} else if (str_lcmp(argv[*index] + 2, "messages=", 6) == 0) {
			rc = arg_parse_int(argc, argv, index, &messages, 8);
			if (rc != EOK)
				return rc;
		} else if (str_lcmp(argv[*index] + 2, "sockets=", 6) == 0) {
			rc = arg_parse_int(argc, argv, index, &sockets, 8);
			if (rc != EOK)
				return rc;
		} else if (str_lcmp(argv[*index] + 2, "port=", 5) == 0) {
			rc = arg_parse_int(argc, argv, index, &value, 7);
			if (rc != EOK)
				return rc;
			
			port = (uint16_t) value;
		} else if (str_lcmp(argv[*index] + 2, "type=", 5) == 0) {
			rc = arg_parse_name_int(argc, argv, index, &value, 7,
			    socket_parse_socket_type);
			if (rc != EOK)
				return rc;
			
			type = (sock_type_t) value;
		} else if (str_lcmp(argv[*index] + 2, "verbose", 8) == 0) {
			verbose = 1;
		} else {
			nettest2_print_help();
			return EINVAL;
		}
		break;
	default:
		nettest2_print_help();
		return EINVAL;
	}
	
	return EOK;
}

int main(int argc, char *argv[])
{
	int index;
	int rc;
	
	/*
	 * Parse the command line arguments.
	 *
	 * Stop before the last argument if it does not start with dash ('-')
	 */
	for (index = 1; (index < argc - 1) || ((index == argc - 1) &&
	    (argv[index][0] == '-')); ++index) {
		/* Options should start with dash ('-') */
		if (argv[index][0] == '-') {
			rc = nettest2_parse_opt(argc, argv, &index);
			if (rc != EOK)
				return rc;
		} else {
			nettest2_print_help();
			return EINVAL;
		}
	}
	
	/* The last argument containing the host */
	if (index >= argc) {
		printf("Host name missing.\n");
		nettest2_print_help();
		return EINVAL;
	}
	
	char *addr_s = argv[argc - 1];
	
	/* Interpret as address */
	inet_addr_t addr_addr;
	rc = inet_addr_parse(addr_s, &addr_addr);
	
	if (rc != EOK) {
		/* Interpret as a host name */
		dnsr_hostinfo_t *hinfo = NULL;
		rc = dnsr_name2host(addr_s, &hinfo, ipver_from_af(family));
		
		if (rc != EOK) {
			printf("Error resolving host '%s'.\n", addr_s);
			return EINVAL;
		}
		
		addr_addr = hinfo->addr;
	}
	
	struct sockaddr *address;
	socklen_t addrlen;
	rc = inet_addr_sockaddr(&addr_addr, port, &address, &addrlen);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		return ENOMEM;
	}
	
	if (family == AF_NONE)
		family = address->sa_family;
	
	if (address->sa_family != family) {
		printf("Address family does not match explicitly set family.\n");
		return EINVAL;
	}
	
	/* Check data buffer size. */
	if (size <= 0) {
		fprintf(stderr, "Data buffer size too small (%zu). Using 1024 "
		    "bytes instead.\n", size);
		size = 1024;
	}
	
	/*
	 * Prepare the buffer. Allocate size bytes plus one for terminating
	 * null character.
	 */
	char *data = (char *) malloc(size + 1);
	if (!data) {
		fprintf(stderr, "Failed to allocate data buffer.\n");
		return ENOMEM;
	}
	
	/* Fill buffer with a pattern. */
	nettest2_fill_buffer(data, size);
	
	/* Check socket count. */
	if (sockets <= 0) {
		fprintf(stderr, "Socket count too small (%d). Using "
		    "2 instead.\n", sockets);
		sockets = 2;
	}
	
	/*
	 * Prepare the socket buffer.
	 * Allocate count entries plus the terminating null (\0)
	 */
	int *socket_ids = (int *) malloc(sizeof(int) * (sockets + 1));
	if (!socket_ids) {
		fprintf(stderr, "Failed to allocate receive buffer.\n");
		return ENOMEM;
	}
	
	socket_ids[sockets] = 0;
	
	if (verbose)
		printf("Starting tests\n");
	
	rc = sockets_create(verbose, socket_ids, sockets, family, type);
	if (rc != EOK)
		return rc;
	
	if (type == SOCK_STREAM) {
		rc = sockets_connect(verbose, socket_ids, sockets,
		    address, addrlen);
		if (rc != EOK)
			return rc;
	}
	
	if (verbose)
		printf("\n");
	
	struct timeval time_before;
	gettimeofday(&time_before, NULL);
	
	rc = sockets_sendto_recvfrom(verbose, socket_ids, sockets, address,
	    &addrlen, data, size, messages, type);
	if (rc != EOK)
		return rc;
	
	struct timeval time_after;
	gettimeofday(&time_after, NULL);
	
	if (verbose)
		printf("\tOK\n");
	
	printf("sendto + recvfrom tested in %ld microseconds\n",
	    tv_sub(&time_after, &time_before));
	
	gettimeofday(&time_before, NULL);
	
	rc = sockets_sendto(verbose, socket_ids, sockets, address, addrlen,
	    data, size, messages, type);
	if (rc != EOK)
		return rc;
	
	rc = sockets_recvfrom(verbose, socket_ids, sockets, address, &addrlen,
	    data, size, messages);
	if (rc != EOK)
		return rc;
	
	gettimeofday(&time_after, NULL);
	
	if (verbose)
		printf("\tOK\n");
	
	printf("sendto, recvfrom tested in %ld microseconds\n",
	    tv_sub(&time_after, &time_before));
	
	rc = sockets_close(verbose, socket_ids, sockets);
	if (rc != EOK)
		return rc;
	
	free(address);
	
	if (verbose)
		printf("\nExiting\n");
	
	return EOK;
}

/** @}
 */
