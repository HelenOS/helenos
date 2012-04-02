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
 * @{
 */

/** @file
 * Packet Internet Network Grouper.
 */

#include <async.h>
#include <stdio.h>
#include <str.h>
#include <task.h>
#include <time.h>
#include <ipc/services.h>
#include <str_error.h>
#include <errno.h>
#include <arg_parse.h>

#include <net/icmp_api.h>
#include <net/in.h>
#include <net/in6.h>
#include <net/inet.h>
#include <net/socket_parse.h>
#include <net/ip_codes.h>

#include "print_error.h"

#define NAME  "ping"

#define CL_OK           0
#define CL_USAGE        -1
#define CL_MISSING      -2
#define CL_INVALID      -3
#define CL_UNSUPPORTED  -4
#define CL_ERROR        -5

/** Ping configuration
 *
 */
typedef struct {
	bool verbose;               /**< Verbose printouts. */
	size_t size;                /**< Outgoing packet size. */
	unsigned int count;         /**< Number of packets to send. */
	suseconds_t timeout;        /**< Reply wait timeout. */
	int af;                     /**< Address family. */
	ip_tos_t tos;               /**< Type of service. */
	ip_ttl_t ttl;               /**< Time-to-live. */
	bool fragments;             /**< Fragmentation. */
	
	char *dest_addr;            /**< Destination address. */
	struct sockaddr_in dest;    /**< IPv4 destionation. */
	struct sockaddr_in6 dest6;  /**< IPv6 destionation. */
	
	struct sockaddr *dest_raw;  /**< Raw destination address. */
	socklen_t dest_len;         /**< Raw destination address length. */
	
	/** Converted address string. */
	char dest_str[INET6_ADDRSTRLEN];
} ping_config_t;


static void usage(void)
{
	printf(
	    "Usage: ping [-c count] [-s size] [-W timeout] [-f family] [-t ttl]\n" \
	    "            [-Q tos] [--dont_fragment] destination\n" \
	    "\n" \
	    "Options:\n" \
	    "\t-c count\n" \
	    "\t--count=count\n" \
	    "\t\tNumber of outgoing packets (default: 4)\n" \
	    "\n" \
	    "\t-s size\n" \
	    "\t--size=bytes\n" \
	    "\t\tOutgoing packet size (default: 56 bytes)\n" \
	    "\n" \
	    "\t-W timeout\n" \
	    "\t--timeout=ms\n" \
	    "\t\tReply wait timeout (default: 3000 ms)\n" \
	    "\n" \
	    "\t-f family\n" \
	    "\t--family=family\n" \
	    "\t\tDestination address family, AF_INET or AF_INET6 (default: AF_INET)\n" \
	    "\n" \
	    "\t-t ttl\n" \
	    "\t--ttl=ttl\n" \
	    "\t\tOutgoing packet time-to-live (default: 0)\n" \
	    "\n" \
	    "\t-Q tos\n" \
	    "\t--tos=tos\n" \
	    "\t\tOutgoing packet type of service (default: 0)\n" \
	    "\n" \
	    "\t--dont_fragment\n" \
	    "\t\tDisable packet fragmentation (default: enabled)\n" \
	    "\n" \
	    "\t-v\n" \
	    "\t--verbose\n" \
	    "\t\tVerbose operation\n" \
	    "\n" \
	    "\t-h\n" \
	    "\t--help\n" \
	    "\t\tPrint this usage information\n"
	);
}

static int args_parse(int argc, char *argv[], ping_config_t *config)
{
	if (argc < 2)
		return CL_USAGE;
	
	int i;
	int ret;
	
	for (i = 1; i < argc; i++) {
		
		/* Not an option */
		if (argv[i][0] != '-')
			break;
		
		/* Options terminator */
		if (str_cmp(argv[i], "--") == 0) {
			i++;
			break;
		}
		
		int off;
		
		/* Usage */
		if ((off = arg_parse_short_long(argv[i], "-h", "--help")) != -1)
			return CL_USAGE;
		
		/* Verbose */
		if ((off = arg_parse_short_long(argv[i], "-v", "--verbose")) != -1) {
			config->verbose = true;
			continue;
		}
		
		/* Don't fragment */
		if (str_cmp(argv[i], "--dont_fragment") == 0) {
			config->fragments = false;
			continue;
		}
		
		/* Count */
		if ((off = arg_parse_short_long(argv[i], "-c", "--count=")) != -1) {
			int tmp;
			ret = arg_parse_int(argc, argv, &i, &tmp, off);
			
			if ((ret != EOK) || (tmp < 0))
				return i;
			
			config->count = (unsigned int) tmp;
			continue;
		}
		
		/* Outgoing packet size */
		if ((off = arg_parse_short_long(argv[i], "-s", "--size=")) != -1) {
			int tmp;
			ret = arg_parse_int(argc, argv, &i, &tmp, off);
			
			if ((ret != EOK) || (tmp < 0))
				return i;
			
			config->size = (size_t) tmp;
			continue;
		}
		
		/* Reply wait timeout */
		if ((off = arg_parse_short_long(argv[i], "-W", "--timeout=")) != -1) {
			int tmp;
			ret = arg_parse_int(argc, argv, &i, &tmp, off);
			
			if ((ret != EOK) || (tmp < 0))
				return i;
			
			config->timeout = (suseconds_t) tmp;
			continue;
		}
		
		/* Address family */
		if ((off = arg_parse_short_long(argv[i], "-f", "--family=")) != -1) {
			ret = arg_parse_name_int(argc, argv, &i, &config->af, off,
			    socket_parse_address_family);
			
			if (ret != EOK)
				return i;
			
			continue;
		}
		
		/* Type of service */
		if ((off = arg_parse_short_long(argv[i], "-Q", "--tos=")) != -1) {
			int tmp;
			ret = arg_parse_name_int(argc, argv, &i, &tmp, off,
			    socket_parse_address_family);
			
			if ((ret != EOK) || (tmp < 0))
				return i;
			
			config->tos = (ip_tos_t) tmp;
			continue;
		}
		
		/* Time to live */
		if ((off = arg_parse_short_long(argv[i], "-t", "--ttl=")) != -1) {
			int tmp;
			ret = arg_parse_name_int(argc, argv, &i, &tmp, off,
			    socket_parse_address_family);
			
			if ((ret != EOK) || (tmp < 0))
				return i;
			
			config->ttl = (ip_ttl_t) tmp;
			continue;
		}
	}
	
	if (i >= argc)
		return CL_MISSING;
	
	config->dest_addr = argv[i];
	
	/* Resolve destionation address */
	switch (config->af) {
	case AF_INET:
		config->dest_raw = (struct sockaddr *) &config->dest;
		config->dest_len = sizeof(config->dest);
		config->dest.sin_family = config->af;
		ret = inet_pton(config->af, config->dest_addr,
		    (uint8_t *) &config->dest.sin_addr.s_addr);
		break;
	case AF_INET6:
		config->dest_raw = (struct sockaddr *) &config->dest6;
		config->dest_len = sizeof(config->dest6);
		config->dest6.sin6_family = config->af;
		ret = inet_pton(config->af, config->dest_addr,
		    (uint8_t *) &config->dest6.sin6_addr.s6_addr);
		break;
	default:
		return CL_UNSUPPORTED;
	}
	
	if (ret != EOK)
		return CL_INVALID;
	
	/* Convert destination address back to string */
	switch (config->af) {
	case AF_INET:
		ret = inet_ntop(config->af,
		    (uint8_t *) &config->dest.sin_addr.s_addr,
		    config->dest_str, sizeof(config->dest_str));
		break;
	case AF_INET6:
		ret = inet_ntop(config->af,
		    (uint8_t *) &config->dest6.sin6_addr.s6_addr,
		    config->dest_str, sizeof(config->dest_str));
		break;
	default:
		return CL_UNSUPPORTED;
	}
	
	if (ret != EOK)
		return CL_ERROR;
	
	return CL_OK;
}

int main(int argc, char *argv[])
{
	ping_config_t config;
	
	/* Default configuration */
	config.verbose = false;
	config.size = 56;
	config.count = 4;
	config.timeout = 3000;
	config.af = AF_INET;
	config.tos = 0;
	config.ttl = 0;
	config.fragments = true;
	
	int ret = args_parse(argc, argv, &config);
	
	switch (ret) {
	case CL_OK:
		break;
	case CL_USAGE:
		usage();
		return 0;
	case CL_MISSING:
		fprintf(stderr, "%s: Destination address missing\n", NAME);
		return 1;
	case CL_INVALID:
		fprintf(stderr, "%s: Destination address '%s' invalid or malformed\n",
		    NAME, config.dest_addr);
		return 2;
	case CL_UNSUPPORTED:
		fprintf(stderr, "%s: Destination address '%s' unsupported\n",
		    NAME, config.dest_addr);
		return 3;
	case CL_ERROR:
		fprintf(stderr, "%s: Destination address '%s' error\n",
		    NAME, config.dest_addr);
		return 4;
	default:
		fprintf(stderr, "%s: Unknown or invalid option '%s'\n", NAME,
		    argv[ret]);
		return 5;
	}
	
	printf("PING %s (%s) %zu(%zu) bytes of data\n", config.dest_addr,
	    config.dest_str, config.size, config.size);
	
	async_sess_t *sess = icmp_connect_module();
	if (!sess) {
		fprintf(stderr, "%s: Unable to connect to ICMP service (%s)\n", NAME,
		    str_error(errno));
		return errno;
	}
	
	unsigned int seq;
	for (seq = 0; seq < config.count; seq++) {
		struct timeval t0;
		ret = gettimeofday(&t0, NULL);
		if (ret != EOK) {
			fprintf(stderr, "%s: gettimeofday failed (%s)\n", NAME,
			    str_error(ret));
			
			async_hangup(sess);
			return ret;
		}
		
		/* Ping! */
		int result = icmp_echo_msg(sess, config.size, config.timeout,
		    config.ttl, config.tos, !config.fragments, config.dest_raw,
		    config.dest_len);
		
		struct timeval t1;
		ret = gettimeofday(&t1, NULL);
		if (ret != EOK) {
			fprintf(stderr, "%s: gettimeofday failed (%s)\n", NAME,
			    str_error(ret));
			
			async_hangup(sess);
			return ret;
		}
		
		suseconds_t elapsed = tv_sub(&t1, &t0);
		
		switch (result) {
		case ICMP_ECHO:
			printf("%zu bytes from ? (?): icmp_seq=%u ttl=? time=%ld.%04ld\n",
				config.size, seq, elapsed / 1000, elapsed % 1000);
			break;
		case ETIMEOUT:
			printf("%zu bytes from ? (?): icmp_seq=%u Timed out\n",
				config.size, seq);
			break;
		default:
			print_error(stdout, result, NULL, "\n");
		}
	}
	
	async_hangup(sess);
	
	return 0;
}

/** @}
 */
