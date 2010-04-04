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

/** @addtogroup net_app
 *  @{
 */

/** @file
 *  Generic application parsing functions implementation.
 */

#include <stdio.h>
#include <str.h>

#include <socket.h>
#include <net_err.h>

#include "parse.h"

int parse_address_family(const char * name){
	if(str_lcmp(name, "AF_INET", 7) == 0){
		return AF_INET;
	}else if(str_lcmp(name, "AF_INET6", 8) == 0){
		return AF_INET6;
	}
	return EAFNOSUPPORT;
}

int parse_parameter_int(int argc, char ** argv, int * index, int * value, const char * name, int offset){
	char * rest;

	if(offset){
		*value = strtol(argv[*index] + offset, &rest, 10);
	}else if((*index) + 1 < argc){
		++ (*index);
		*value = strtol(argv[*index], &rest, 10);
	}else{
		fprintf(stderr, "Command line error: missing %s\n", name);
		return EINVAL;
	}
	if(rest && (*rest)){
		fprintf(stderr, "Command line error: %s unrecognized (%d: %s)\n", name, * index, argv[*index]);
		return EINVAL;
	}
	return EOK;
}

int parse_parameter_name_int(int argc, char ** argv, int * index, int * value, const char * name, int offset, int (*parse_value)(const char * value)){
	ERROR_DECLARE;

	char * parameter;

	ERROR_PROPAGATE(parse_parameter_string(argc, argv, index, &parameter, name, offset));
	*value = (*parse_value)(parameter);
	if((*value) == ENOENT){
		fprintf(stderr, "Command line error: unrecognized %s value (%d: %s)\n", name, * index, parameter);
		return ENOENT;
	}
	return EOK;
}

int parse_parameter_string(int argc, char ** argv, int * index, char ** value, const char * name, int offset){
	if(offset){
		*value = argv[*index] + offset;
	}else if((*index) + 1 < argc){
		++ (*index);
		*value = argv[*index];
	}else{
		fprintf(stderr, "Command line error: missing %s\n", name);
		return EINVAL;
	}
	return EOK;
}

int parse_protocol_family(const char * name){
	if(str_lcmp(name, "PF_INET", 7) == 0){
		return PF_INET;
	}else if(str_lcmp(name, "PF_INET6", 8) == 0){
		return PF_INET6;
	}
	return EPFNOSUPPORT;
}

int parse_socket_type(const char * name){
	if(str_lcmp(name, "SOCK_DGRAM", 11) == 0){
		return SOCK_DGRAM;
	}else if(str_lcmp(name, "SOCK_STREAM", 12) == 0){
		return SOCK_STREAM;
	}
	return ESOCKTNOSUPPORT;
}

void print_unrecognized(int index, const char * parameter){
	fprintf(stderr, "Command line error: unrecognized argument (%d: %s)\n", index, parameter);
}

/** @}
 */
