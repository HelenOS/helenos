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
 *  Networking test support functions implementation.
 */

#include <stdio.h>

#include <socket.h>
#include <net_err.h>

#include "nettest.h"
#include "print_error.h"

int sockets_create(int verbose, int * socket_ids, int sockets, int family, sock_type_t type){
	int index;

	if(verbose){
		printf("Create\t");
	}
	fflush(stdout);
	for(index = 0; index < sockets; ++ index){
		socket_ids[index] = socket(family, type, 0);
		if(socket_ids[index] < 0){
			printf("Socket %d (%d) error:\n", index, socket_ids[index]);
			socket_print_error(stderr, socket_ids[index], "Socket create: ", "\n");
			return socket_ids[index];
		}
		if(verbose){
			print_mark(index);
		}
	}
	return EOK;
}

int sockets_close(int verbose, int * socket_ids, int sockets){
	ERROR_DECLARE;

	int index;

	if(verbose){
		printf("\tClose\t");
	}
	fflush(stdout);
	for(index = 0; index < sockets; ++ index){
		if(ERROR_OCCURRED(closesocket(socket_ids[index]))){
			printf("Socket %d (%d) error:\n", index, socket_ids[index]);
			socket_print_error(stderr, ERROR_CODE, "Socket close: ", "\n");
			return ERROR_CODE;
		}
		if(verbose){
			print_mark(index);
		}
	}
	return EOK;
}

int sockets_connect(int verbose, int * socket_ids, int sockets, struct sockaddr * address, socklen_t addrlen){
	ERROR_DECLARE;

	int index;

	if(verbose){
		printf("\tConnect\t");
	}
	fflush(stdout);
	for(index = 0; index < sockets; ++ index){
		if(ERROR_OCCURRED(connect(socket_ids[index], address, addrlen))){
			socket_print_error(stderr, ERROR_CODE, "Socket connect: ", "\n");
			return ERROR_CODE;
		}
		if(verbose){
			print_mark(index);
		}
	}
	return EOK;
}

int sockets_sendto(int verbose, int * socket_ids, int sockets, struct sockaddr * address, socklen_t addrlen, char * data, int size, int messages){
	ERROR_DECLARE;

	int index;
	int message;

	if(verbose){
		printf("\tSendto\t");
	}
	fflush(stdout);
	for(index = 0; index < sockets; ++ index){
		for(message = 0; message < messages; ++ message){
			if(ERROR_OCCURRED(sendto(socket_ids[index], data, size, 0, address, addrlen))){
				printf("Socket %d (%d), message %d error:\n", index, socket_ids[index], message);
				socket_print_error(stderr, ERROR_CODE, "Socket send: ", "\n");
				return ERROR_CODE;
			}
		}
		if(verbose){
			print_mark(index);
		}
	}
	return EOK;
}

int sockets_recvfrom(int verbose, int * socket_ids, int sockets, struct sockaddr * address, socklen_t * addrlen, char * data, int size, int messages){
	int value;
	int index;
	int message;

	if(verbose){
		printf("\tRecvfrom\t");
	}
	fflush(stdout);
	for(index = 0; index < sockets; ++ index){
		for(message = 0; message < messages; ++ message){
			value = recvfrom(socket_ids[index], data, size, 0, address, addrlen);
			if(value < 0){
				printf("Socket %d (%d), message %d error:\n", index, socket_ids[index], message);
				socket_print_error(stderr, value, "Socket receive: ", "\n");
				return value;
			}
		}
		if(verbose){
			print_mark(index);
		}
	}
	return EOK;
}

int sockets_sendto_recvfrom(int verbose, int * socket_ids, int sockets, struct sockaddr * address, socklen_t * addrlen, char * data, int size, int messages){
	ERROR_DECLARE;

	int value;
	int index;
	int message;

	if(verbose){
		printf("\tSendto and recvfrom\t");
	}
	fflush(stdout);
	for(index = 0; index < sockets; ++ index){
		for(message = 0; message < messages; ++ message){
			if(ERROR_OCCURRED(sendto(socket_ids[index], data, size, 0, address, * addrlen))){
				printf("Socket %d (%d), message %d error:\n", index, socket_ids[index], message);
				socket_print_error(stderr, ERROR_CODE, "Socket send: ", "\n");
				return ERROR_CODE;
			}
			value = recvfrom(socket_ids[index], data, size, 0, address, addrlen);
			if(value < 0){
				printf("Socket %d (%d), message %d error:\n", index, socket_ids[index], message);
				socket_print_error(stderr, value, "Socket receive: ", "\n");
				return value;
			}
		}
		if(verbose){
			print_mark(index);
		}
	}
	return EOK;
}

void print_mark(int index){
	if((index + 1) % 10){
		printf("*");
	}else{
		printf("|");
	}
	fflush(stdout);
}

/** @}
 */
