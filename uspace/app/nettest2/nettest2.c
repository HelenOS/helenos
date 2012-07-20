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

#include <malloc.h>
#include <stdio.h>
#include <unistd.h>
#include <str.h>
#include <task.h>
#include <time.h>
#include <arg_parse.h>
#include <bool.h>

#include <net/in.h>
#include <net/in6.h>
#include <net/inet.h>
#include <net/socket.h>
#include <net/socket_parse.h>

/** Echo module name. */
#define NAME	"Nettest2"

/** Packet data pattern. */
#define NETTEST2_TEXT	"Networking test 2 - transfer"

static size_t size;
static bool verbose;
static sock_type_t type;
static int sockets;
static int messages;
static int family;
static uint16_t port;

static void nettest2_print_help(void)
{
	printf(
	    "Network Networking test 2 aplication - UDP transfer\n"
	    "Usage: echo [options] address\n"
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
	size_t length;

	length = 0;
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
		rc = arg_parse_name_int(argc, argv, index, &family, 0,
		    socket_parse_protocol_family);
		if (rc != EOK)
			return rc;
		break;
	case 'h':
		nettest2_print_help();
		return EOK;
		break;
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
			rc = arg_parse_name_int(argc, argv, index, &family, 9,
			    socket_parse_protocol_family);
			if (rc != EOK)
				return rc;
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
	struct sockaddr *address;
	struct sockaddr_in address_in;
	struct sockaddr_in6 address_in6;
	socklen_t addrlen;
	uint8_t *address_start;

	int *socket_ids;
	char *data;
	int index;
	struct timeval time_before;
	struct timeval time_after;

	int rc;

	size = 28;
	verbose = false;
	type = SOCK_DGRAM;
	sockets = 10;
	messages = 10;
	family = PF_INET;
	port = 7;

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

	/* If not before the last argument containing the address */
	if (index >= argc) {
		printf("Command line error: missing address\n");
		nettest2_print_help();
		return EINVAL;
	}

	/* Prepare the address buffer */

	switch (family) {
	case PF_INET:
		address_in.sin_family = AF_INET;
		address_in.sin_port = htons(port);
		address = (struct sockaddr *) &address_in;
		addrlen = sizeof(address_in);
		address_start = (uint8_t *) &address_in.sin_addr.s_addr;
		break;
	case PF_INET6:
		address_in6.sin6_family = AF_INET6;
		address_in6.sin6_port = htons(port);
		address = (struct sockaddr *) &address_in6;
		addrlen = sizeof(address_in6);
		address_start = (uint8_t *) &address_in6.sin6_addr.s6_addr;
		break;
	default:
		fprintf(stderr, "Address family is not supported\n");
		return EAFNOSUPPORT;
	}

	/* Parse the last argument which should contain the address. */
	rc = inet_pton(family, argv[argc - 1], address_start);
	if (rc != EOK) {
		fprintf(stderr, "Address parse error %d\n", rc);
		return rc;
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
	data = (char *) malloc(size + 1);
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
	socket_ids = (int *) malloc(sizeof(int) * (sockets + 1));
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

	rc = gettimeofday(&time_before, NULL);
	if (rc != EOK) {
		fprintf(stderr, "Get time of day error %d\n", rc);
		return rc;
	}

	rc = sockets_sendto_recvfrom(verbose, socket_ids, sockets, address,
	    &addrlen, data, size, messages);
	if (rc != EOK)
		return rc;

	rc = gettimeofday(&time_after, NULL);
	if (rc != EOK) {
		fprintf(stderr, "Get time of day error %d\n", rc);
		return rc;
	}

	if (verbose)
		printf("\tOK\n");

	printf("sendto + recvfrom tested in %ld microseconds\n",
	    tv_sub(&time_after, &time_before));

	rc = gettimeofday(&time_before, NULL);
	if (rc != EOK) {
		fprintf(stderr, "Get time of day error %d\n", rc);
		return rc;
	}

	rc = sockets_sendto(verbose, socket_ids, sockets, address, addrlen,
	    data, size, messages);
	if (rc != EOK)
		return rc;

	rc = sockets_recvfrom(verbose, socket_ids, sockets, address, &addrlen,
	    data, size, messages);
	if (rc != EOK)
		return rc;

	rc = gettimeofday(&time_after, NULL);
	if (rc != EOK) {
		fprintf(stderr, "Get time of day error %d\n", rc);
		return rc;
	}

	if (verbose)
		printf("\tOK\n");

	printf("sendto, recvfrom tested in %ld microseconds\n",
	    tv_sub(&time_after, &time_before));

	rc = sockets_close(verbose, socket_ids, sockets);
	if (rc != EOK)
		return rc;

	if (verbose)
		printf("\nExiting\n");

	return EOK;
}

/** @}
 */
