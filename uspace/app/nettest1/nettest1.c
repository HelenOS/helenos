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
 *  @{
 */

/** @file
 *  Networking test 1 application - sockets.
 */

#include <malloc.h>
#include <stdio.h>
#include <str.h>
#include <task.h>
#include <time.h>

#include <in.h>
#include <in6.h>
#include <inet.h>
#include <socket.h>
#include <net_err.h>

#include "nettest.h"
#include "parse.h"
#include "print_error.h"

/** Echo module name.
 */
#define NAME	"Nettest1"

/** Packet data pattern.
 */
#define NETTEST1_TEXT	"Networking test 1 - sockets"

/** Module entry point.
 *  Starts testing.
 *  @param[in] argc The number of command line parameters.
 *  @param[in] argv The command line parameters.
 *  @returns EOK on success.
 */
int main(int argc, char * argv[]);

/** Prints the application help.
 */
void nettest1_print_help(void);

/** Refreshes the data.
 *  Fills the data block with the NETTEST1_TEXT pattern.
 *  @param[out] data The data block.
 *  @param[in] size The data block size in bytes.
 */
void nettest1_refresh_data(char * data, size_t size);

int main(int argc, char * argv[]){
	ERROR_DECLARE;

	size_t size			= 27;
	int verbose			= 0;
	sock_type_t type	= SOCK_DGRAM;
	int sockets			= 10;
	int messages		= 10;
	int family			= PF_INET;
	uint16_t port		= 7;

	socklen_t max_length				= sizeof(struct sockaddr_in6);
	uint8_t address_data[max_length];
	struct sockaddr * address			= (struct sockaddr *) address_data;
	struct sockaddr_in * address_in		= (struct sockaddr_in *) address;
	struct sockaddr_in6 * address_in6	= (struct sockaddr_in6 *) address;
	socklen_t addrlen;
//	char address_string[INET6_ADDRSTRLEN];
	uint8_t * address_start;

	int * socket_ids;
	char * data;
	int value;
	int index;
	struct timeval time_before;
	struct timeval time_after;

	// print the program label
	printf("Task %d - ", task_get_id());
	printf("%s\n", NAME);

	// parse the command line arguments
	// stop before the last argument if it does not start with the minus sign ('-')
	for(index = 1; (index < argc - 1) || ((index == argc - 1) && (argv[index][0] == '-')); ++ index){
		// options should start with the minus sign ('-')
		if(argv[index][0] == '-'){
			switch(argv[index][1]){
				// short options with only one letter
				case 'f':
					ERROR_PROPAGATE(parse_parameter_name_int(argc, argv, &index, &family, "protocol family", 0, parse_protocol_family));
					break;
				case 'h':
					nettest1_print_help();
					return EOK;
					break;
				case 'm':
					ERROR_PROPAGATE(parse_parameter_int(argc, argv, &index, &messages, "message count", 0));
					break;
				case 'n':
					ERROR_PROPAGATE(parse_parameter_int(argc, argv, &index, &sockets, "socket count", 0));
					break;
				case 'p':
					ERROR_PROPAGATE(parse_parameter_int(argc, argv, &index, &value, "port number", 0));
					port = (uint16_t) value;
					break;
				case 's':
					ERROR_PROPAGATE(parse_parameter_int(argc, argv, &index, &value, "packet size", 0));
					size = (value >= 0) ? (size_t) value : 0;
					break;
				case 't':
					ERROR_PROPAGATE(parse_parameter_name_int(argc, argv, &index, &value, "socket type", 0, parse_socket_type));
					type = (sock_type_t) value;
					break;
				case 'v':
					verbose = 1;
					break;
				// long options with the double minus sign ('-')
				case '-':
					if(str_lcmp(argv[index] + 2, "family=", 7) == 0){
						ERROR_PROPAGATE(parse_parameter_name_int(argc, argv, &index, &family, "protocol family", 9, parse_protocol_family));
					}else if(str_lcmp(argv[index] + 2, "help", 5) == 0){
						nettest1_print_help();
						return EOK;
					}else if(str_lcmp(argv[index] + 2, "messages=", 6) == 0){
						ERROR_PROPAGATE(parse_parameter_int(argc, argv, &index, &messages, "message count", 8));
					}else if(str_lcmp(argv[index] + 2, "sockets=", 6) == 0){
						ERROR_PROPAGATE(parse_parameter_int(argc, argv, &index, &sockets, "socket count", 8));
					}else if(str_lcmp(argv[index] + 2, "port=", 5) == 0){
						ERROR_PROPAGATE(parse_parameter_int(argc, argv, &index, &value, "port number", 7));
						port = (uint16_t) value;
					}else if(str_lcmp(argv[index] + 2, "type=", 5) == 0){
						ERROR_PROPAGATE(parse_parameter_name_int(argc, argv, &index, &value, "socket type", 7, parse_socket_type));
						type = (sock_type_t) value;
					}else if(str_lcmp(argv[index] + 2, "verbose", 8) == 0){
						verbose = 1;
					}else{
						print_unrecognized(index, argv[index] + 2);
						nettest1_print_help();
						return EINVAL;
					}
					break;
				default:
					print_unrecognized(index, argv[index] + 1);
					nettest1_print_help();
					return EINVAL;
			}
		}else{
			print_unrecognized(index, argv[index]);
			nettest1_print_help();
			return EINVAL;
		}
	}

	// if not before the last argument containing the address
	if(index >= argc){
		printf("Command line error: missing address\n");
		nettest1_print_help();
		return EINVAL;
	}

	// prepare the address buffer
	bzero(address_data, max_length);
	switch(family){
		case PF_INET:
			address_in->sin_family = AF_INET;
			address_in->sin_port = htons(port);
			address_start = (uint8_t *) &address_in->sin_addr.s_addr;
			addrlen = sizeof(struct sockaddr_in);
			break;
		case PF_INET6:
			address_in6->sin6_family = AF_INET6;
			address_in6->sin6_port = htons(port);
			address_start = (uint8_t *) &address_in6->sin6_addr.s6_addr;
			addrlen = sizeof(struct sockaddr_in6);
			break;
		default:
			fprintf(stderr, "Address family is not supported\n");
			return EAFNOSUPPORT;
	}

	// parse the last argument which should contain the address
	if(ERROR_OCCURRED(inet_pton(family, argv[argc - 1], address_start))){
		fprintf(stderr, "Address parse error %d\n", ERROR_CODE);
		return ERROR_CODE;
	}

	// check the buffer size
	if(size <= 0){
		fprintf(stderr, "Data buffer size too small (%d). Using 1024 bytes instead.\n", size);
		size = 1024;
	}

	// prepare the buffer
	// size plus the terminating null (\0)
	data = (char *) malloc(size + 1);
	if(! data){
		fprintf(stderr, "Failed to allocate data buffer.\n");
		return ENOMEM;
	}
	nettest1_refresh_data(data, size);

	// check the socket count
	if(sockets <= 0){
		fprintf(stderr, "Socket count too small (%d). Using 2 instead.\n", sockets);
		sockets = 2;
	}

	// prepare the socket buffer
	// count plus the terminating null (\0)
	socket_ids = (int *) malloc(sizeof(int) * (sockets + 1));
	if(! socket_ids){
		fprintf(stderr, "Failed to allocate receive buffer.\n");
		return ENOMEM;
	}
	socket_ids[sockets] = NULL;

	if(verbose){
		printf("Starting tests\n");
	}

	if(verbose){
		printf("1 socket, 1 message\n");
	}

	if(ERROR_OCCURRED(gettimeofday(&time_before, NULL))){
		fprintf(stderr, "Get time of day error %d\n", ERROR_CODE);
		return ERROR_CODE;
	}

	ERROR_PROPAGATE(sockets_create(verbose, socket_ids, 1, family, type));
	ERROR_PROPAGATE(sockets_close(verbose, socket_ids, 1));
	if(verbose){
		printf("\tOK\n");
	}

	ERROR_PROPAGATE(sockets_create(verbose, socket_ids, 1, family, type));
	if(type == SOCK_STREAM){
		ERROR_PROPAGATE(sockets_connect(verbose, socket_ids, 1, address, addrlen));
	}
	ERROR_PROPAGATE(sockets_sendto_recvfrom(verbose, socket_ids, 1, address, &addrlen, data, size, 1));
	ERROR_PROPAGATE(sockets_close(verbose, socket_ids, 1));
	if(verbose){
		printf("\tOK\n");
	}

	ERROR_PROPAGATE(sockets_create(verbose, socket_ids, 1, family, type));
	if(type == SOCK_STREAM){
		ERROR_PROPAGATE(sockets_connect(verbose, socket_ids, 1, address, addrlen));
	}
	ERROR_PROPAGATE(sockets_sendto(verbose, socket_ids, 1, address, addrlen, data, size, 1));
	ERROR_PROPAGATE(sockets_recvfrom(verbose, socket_ids, 1, address, &addrlen, data, size, 1));
	ERROR_PROPAGATE(sockets_close(verbose, socket_ids, 1));
	if(verbose){
		printf("\tOK\n");
	}

	if(verbose){
		printf("1 socket, %d messages\n", messages);
	}

	ERROR_PROPAGATE(sockets_create(verbose, socket_ids, 1, family, type));
	if(type == SOCK_STREAM){
		ERROR_PROPAGATE(sockets_connect(verbose, socket_ids, 1, address, addrlen));
	}
	ERROR_PROPAGATE(sockets_sendto_recvfrom(verbose, socket_ids, 1, address, &addrlen, data, size, messages));
	ERROR_PROPAGATE(sockets_close(verbose, socket_ids, 1));
	if(verbose){
		printf("\tOK\n");
	}

	ERROR_PROPAGATE(sockets_create(verbose, socket_ids, 1, family, type));
	if(type == SOCK_STREAM){
		ERROR_PROPAGATE(sockets_connect(verbose, socket_ids, 1, address, addrlen));
	}
	ERROR_PROPAGATE(sockets_sendto(verbose, socket_ids, 1, address, addrlen, data, size, messages));
	ERROR_PROPAGATE(sockets_recvfrom(verbose, socket_ids, 1, address, &addrlen, data, size, messages));
	ERROR_PROPAGATE(sockets_close(verbose, socket_ids, 1));
	if(verbose){
		printf("\tOK\n");
	}

	if(verbose){
		printf("%d sockets, 1 message\n", sockets);
	}

	ERROR_PROPAGATE(sockets_create(verbose, socket_ids, sockets, family, type));
	ERROR_PROPAGATE(sockets_close(verbose, socket_ids, sockets));
	if(verbose){
		printf("\tOK\n");
	}

	ERROR_PROPAGATE(sockets_create(verbose, socket_ids, sockets, family, type));
	if(type == SOCK_STREAM){
		ERROR_PROPAGATE(sockets_connect(verbose, socket_ids, sockets, address, addrlen));
	}
	ERROR_PROPAGATE(sockets_sendto_recvfrom(verbose, socket_ids, sockets, address, &addrlen, data, size, 1));
	ERROR_PROPAGATE(sockets_close(verbose, socket_ids, sockets));
	if(verbose){
		printf("\tOK\n");
	}

	ERROR_PROPAGATE(sockets_create(verbose, socket_ids, sockets, family, type));
	if(type == SOCK_STREAM){
		ERROR_PROPAGATE(sockets_connect(verbose, socket_ids, sockets, address, addrlen));
	}
	ERROR_PROPAGATE(sockets_sendto(verbose, socket_ids, sockets, address, addrlen, data, size, 1));
	ERROR_PROPAGATE(sockets_recvfrom(verbose, socket_ids, sockets, address, &addrlen, data, size, 1));
	ERROR_PROPAGATE(sockets_close(verbose, socket_ids, sockets));
	if(verbose){
		printf("\tOK\n");
	}

	if(verbose){
		printf("%d sockets, %d messages\n", sockets, messages);
	}

	ERROR_PROPAGATE(sockets_create(verbose, socket_ids, sockets, family, type));
	if(type == SOCK_STREAM){
		ERROR_PROPAGATE(sockets_connect(verbose, socket_ids, sockets, address, addrlen));
	}
	ERROR_PROPAGATE(sockets_sendto_recvfrom(verbose, socket_ids, sockets, address, &addrlen, data, size, messages));
	ERROR_PROPAGATE(sockets_close(verbose, socket_ids, sockets));
	if(verbose){
		printf("\tOK\n");
	}

	ERROR_PROPAGATE(sockets_create(verbose, socket_ids, sockets, family, type));
	if(type == SOCK_STREAM){
		ERROR_PROPAGATE(sockets_connect(verbose, socket_ids, sockets, address, addrlen));
	}
	ERROR_PROPAGATE(sockets_sendto(verbose, socket_ids, sockets, address, addrlen, data, size, messages));
	ERROR_PROPAGATE(sockets_recvfrom(verbose, socket_ids, sockets, address, &addrlen, data, size, messages));
	ERROR_PROPAGATE(sockets_close(verbose, socket_ids, sockets));

	if(ERROR_OCCURRED(gettimeofday(&time_after, NULL))){
		fprintf(stderr, "Get time of day error %d\n", ERROR_CODE);
		return ERROR_CODE;
	}

	if(verbose){
		printf("\tOK\n");
	}

	printf("Tested in %d microseconds\n", tv_sub(&time_after, &time_before));

	if(verbose){
		printf("Exiting\n");
	}

	return EOK;
}

void nettest1_print_help(void){
	printf(
		"Network Networking test 1 aplication - sockets\n" \
		"Usage: echo [options] numeric_address\n" \
		"Where options are:\n" \
		"-f protocol_family | --family=protocol_family\n" \
		"\tThe listenning socket protocol family. Only the PF_INET and PF_INET6 are supported.\n"
		"\n" \
		"-h | --help\n" \
		"\tShow this application help.\n"
		"\n" \
		"-m count | --messages=count\n" \
		"\tThe number of messages to send and receive per socket. The default is 10.\n" \
		"\n" \
		"-n sockets | --sockets=count\n" \
		"\tThe number of sockets to use. The default is 10.\n" \
		"\n" \
		"-p port_number | --port=port_number\n" \
		"\tThe port number the application should send messages to. The default is 7.\n" \
		"\n" \
		"-s packet_size | --size=packet_size\n" \
		"\tThe packet data size the application sends. The default is 28 bytes.\n" \
		"\n" \
		"-v | --verbose\n" \
		"\tShow all output messages.\n"
	);
}

void nettest1_refresh_data(char * data, size_t size){
	size_t length;

	// fill the data
	length = 0;
	while(size > length + sizeof(NETTEST1_TEXT) - 1){
		memcpy(data + length, NETTEST1_TEXT, sizeof(NETTEST1_TEXT) - 1);
		length += sizeof(NETTEST1_TEXT) - 1;
	}
	memcpy(data + length, NETTEST1_TEXT, size - length);
	data[size] = '\0';
}

/** @}
 */
