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

/** @addtogroup ping
 *  @{
 */

/** @file
 *  Ping application.
 */

#include <stdio.h>
#include <string.h>
#include <task.h>
#include <time.h>
#include <ipc/ipc.h>
#include <ipc/services.h>

#include "../../include/icmp_api.h"
#include "../../include/in.h"
#include "../../include/in6.h"
#include "../../include/inet.h"
#include "../../include/ip_codes.h"
#include "../../include/socket_errno.h"

#include "../../err.h"

#include "../parse.h"
#include "../print_error.h"

/** Echo module name.
 */
#define NAME	"Ping"

/** Module entry point.
 *  Reads command line parameters and pings.
 *  @param[in] argc The number of command line parameters.
 *  @param[in] argv The command line parameters.
 *  @returns EOK on success.
 */
int main(int argc, char * argv[]);

/** Prints the application help.
 */
void ping_print_help(void);

int main(int argc, char * argv[]){
	ERROR_DECLARE;

	size_t size			= 38;
	int verbose			= 0;
	int dont_fragment	= 0;
	ip_ttl_t ttl		= 0;
	ip_tos_t tos		= 0;
	int count			= 3;
	suseconds_t timeout	= 3000;
	int family			= AF_INET;

	socklen_t max_length				= sizeof(struct sockaddr_in6);
	uint8_t address_data[max_length];
	struct sockaddr * address			= (struct sockaddr *) address_data;
	struct sockaddr_in * address_in		= (struct sockaddr_in *) address;
	struct sockaddr_in6 * address_in6	= (struct sockaddr_in6 *) address;
	socklen_t addrlen;
	char address_string[INET6_ADDRSTRLEN];
	uint8_t * address_start;
	int icmp_phone;
	struct timeval time_before;
	struct timeval time_after;
	int result;
	int value;
	int index;

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
				case 'c':
					ERROR_PROPAGATE(parse_parameter_int(argc, argv, &index, &count, "count", 0));
					break;
				case 'f':
					ERROR_PROPAGATE(parse_parameter_name_int(argc, argv, &index, &family, "address family", 0, parse_address_family));
					break;
				case 'h':
					ping_print_help();
					return EOK;
					break;
				case 's':
					ERROR_PROPAGATE(parse_parameter_int(argc, argv, &index, &value, "packet size", 0));
					size = (value >= 0) ? (size_t) value : 0;
					break;
				case 't':
					ERROR_PROPAGATE(parse_parameter_int(argc, argv, &index, &value, "timeout", 0));
					timeout = (value >= 0) ? (suseconds_t) value : 0;
					break;
				case 'v':
					verbose = 1;
					break;
				// long options with the double minus sign ('-')
				case '-':
					if(str_lcmp(argv[index] + 2, "count=", 6) == 0){
						ERROR_PROPAGATE(parse_parameter_int(argc, argv, &index, &count, "received count", 8));
					}else if(str_lcmp(argv[index] + 2, "dont_fragment", 13) == 0){
						dont_fragment = 1;
					}else if(str_lcmp(argv[index] + 2, "family=", 7) == 0){
						ERROR_PROPAGATE(parse_parameter_name_int(argc, argv, &index, &family, "address family", 9, parse_address_family));
					}else if(str_lcmp(argv[index] + 2, "help", 5) == 0){
						ping_print_help();
						return EOK;
					}else if(str_lcmp(argv[index] + 2, "size=", 5) == 0){
						ERROR_PROPAGATE(parse_parameter_int(argc, argv, &index, &value, "packet size", 7));
						size = (value >= 0) ? (size_t) value : 0;
					}else if(str_lcmp(argv[index] + 2, "timeout=", 8) == 0){
						ERROR_PROPAGATE(parse_parameter_int(argc, argv, &index, &value, "timeout", 7));
						timeout = (value >= 0) ? (suseconds_t) value : 0;
					}else if(str_lcmp(argv[index] + 2, "tos=", 4) == 0){
						ERROR_PROPAGATE(parse_parameter_int(argc, argv, &index, &value, "type of service", 7));
						tos = (value >= 0) ? (ip_tos_t) value : 0;
					}else if(str_lcmp(argv[index] + 2, "ttl=", 4) == 0){
						ERROR_PROPAGATE(parse_parameter_int(argc, argv, &index, &value, "time to live", 7));
						ttl = (value >= 0) ? (ip_ttl_t) value : 0;
					}else if(str_lcmp(argv[index] + 2, "verbose", 8) == 0){
						verbose = 1;
					}else{
						print_unrecognized(index, argv[index] + 2);
						ping_print_help();
						return EINVAL;
					}
					break;
				default:
					print_unrecognized(index, argv[index] + 1);
					ping_print_help();
					return EINVAL;
			}
		}else{
			print_unrecognized(index, argv[index]);
			ping_print_help();
			return EINVAL;
		}
	}

	// if not before the last argument containing the address
	if(index >= argc){
		printf("Command line error: missing address\n");
		ping_print_help();
		return EINVAL;
	}

	// prepare the address buffer
	bzero(address_data, max_length);
	switch(family){
		case AF_INET:
			address_in->sin_family = AF_INET;
			address_start = (uint8_t *) &address_in->sin_addr.s_addr;
			addrlen = sizeof(struct sockaddr_in);
			break;
		case AF_INET6:
			address_in6->sin6_family = AF_INET6;
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

	// connect to the ICMP module
	icmp_phone = icmp_connect_module(SERVICE_ICMP, ICMP_CONNECT_TIMEOUT);
	if(icmp_phone < 0){
		fprintf(stderr, "ICMP connect error %d\n", icmp_phone);
		return icmp_phone;
	}

	// print the ping header
	printf("PING %d bytes of data\n", size);
	if(ERROR_OCCURRED(inet_ntop(address->sa_family, address_start, address_string, sizeof(address_string)))){
		fprintf(stderr, "Address error %d\n", ERROR_CODE);
	}else{
		printf("Address %s:\n", address_string);
	}

	// do count times
	while(count > 0){

		// get the starting time
		if(ERROR_OCCURRED(gettimeofday(&time_before, NULL))){
			fprintf(stderr, "Get time of day error %d\n", ERROR_CODE);
			// release the ICMP phone
			ipc_hangup(icmp_phone);
			return ERROR_CODE;
		}

		// request the ping
		result = icmp_echo_msg(icmp_phone, size, timeout, ttl, tos, dont_fragment, address, addrlen);

		// get the ending time
		if(ERROR_OCCURRED(gettimeofday(&time_after, NULL))){
			fprintf(stderr, "Get time of day error %d\n", ERROR_CODE);
			// release the ICMP phone
			ipc_hangup(icmp_phone);
			return ERROR_CODE;
		}

		// print the result
		switch(result){
			case ICMP_ECHO:
				printf("Ping round trip time %d miliseconds\n", tv_sub(&time_after, &time_before) / 1000);
				break;
			case ETIMEOUT:
				printf("Timed out.\n");
				break;
			default:
				print_error(stdout, result, NULL, "\n");
		}
		-- count;
	}

	if(verbose){
		printf("Exiting\n");
	}

	// release the ICMP phone
	ipc_hangup(icmp_phone);

	return EOK;
}

void ping_print_help(void){
	printf(
		"Network Ping aplication\n" \
		"Usage: ping [options] numeric_address\n" \
		"Where options are:\n" \
		"\n" \
		"-c request_count | --count=request_count\n" \
		"\tThe number of packets the application sends. The default is three (3).\n" \
		"\n" \
		"--dont_fragment\n" \
		"\tDisable packet fragmentation.\n"
		"\n" \
		"-f address_family | --family=address_family\n" \
		"\tThe given address family. Only the AF_INET and AF_INET6 are supported.\n"
		"\n" \
		"-h | --help\n" \
		"\tShow this application help.\n"
		"\n" \
		"-s packet_size | --size=packet_size\n" \
		"\tThe packet data size the application sends. The default is 38 bytes.\n" \
		"\n" \
		"-t timeout | --timeout=timeout\n" \
		"\tThe number of miliseconds the application waits for a reply. The default is three thousands (3 000).\n" \
		"\n" \
		"--tos=tos\n" \
		"\tThe type of service to be used.\n" \
		"\n" \
		"--ttl=ttl\n" \
		"\tThe time to live to be used.\n" \
		"\n" \
		"-v | --verbose\n" \
		"\tShow all output messages.\n"
	);
}

/** @}
 */
