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
 * Networking test support functions implementation.
 */

#include <stdio.h>
#include <net/socket.h>

#include "nettest.h"
#include "print_error.h"

/** Creates new sockets.
 *
 * @param[in] verbose A value indicating whether to print out verbose information.
 * @param[out] socket_ids A field to store the socket identifiers.
 * @param[in] sockets The number of sockets to create. Should be at most the size of the field.
 * @param[in] family The socket address family.
 * @param[in] type The socket type.
 * @return EOK on success.
 * @return Other error codes as defined for the socket() function.
 */
int sockets_create(int verbose, int *socket_ids, unsigned int sockets,
    uint16_t family, sock_type_t type)
{
	if (verbose)
		printf("Create\t");
	
	fflush(stdout);
	
	for (unsigned int index = 0; index < sockets; index++) {
		socket_ids[index] = socket(family, type, 0);
		if (socket_ids[index] < 0) {
			printf("Socket %u (%d) error:\n", index, socket_ids[index]);
			socket_print_error(stderr, socket_ids[index], "Socket create: ", "\n");
			return socket_ids[index];
		}
		
		if (verbose)
			print_mark(index);
	}
	
	return EOK;
}

/** Closes sockets.
 *
 * @param[in] verbose A value indicating whether to print out verbose information.
 * @param[in] socket_ids A field of stored socket identifiers.
 * @param[in] sockets The number of sockets in the field. Should be at most the size of the field.
 * @return EOK on success.
 * @return Other error codes as defined for the closesocket() function.
 */
int sockets_close(int verbose, int *socket_ids, unsigned int sockets)
{
	if (verbose)
		printf("\tClose\t");
	
	fflush(stdout);
	
	for (unsigned int index = 0; index < sockets; index++) {
		int rc = closesocket(socket_ids[index]);
		if (rc != EOK) {
			printf("Socket %u (%d) error:\n", index, socket_ids[index]);
			socket_print_error(stderr, rc, "Socket close: ", "\n");
			return rc;
		}
		
		if (verbose)
			print_mark(index);
	}
	
	return EOK;
}

/** Connects sockets.
 *
 * @param[in] verbose A value indicating whether to print out verbose information.
 * @param[in] socket_ids A field of stored socket identifiers.
 * @param[in] sockets The number of sockets in the field. Should be at most the size of the field.
 * @param[in] address The destination host address to connect to.
 * @param[in] addrlen The length of the destination address in bytes.
 * @return EOK on success.
 * @return Other error codes as defined for the connect() function.
 */
int sockets_connect(int verbose, int *socket_ids, unsigned int sockets,
    struct sockaddr *address, socklen_t addrlen)
{
	if (verbose)
		printf("\tConnect\t");
	
	fflush(stdout);
	
	for (unsigned int index = 0; index < sockets; index++) {
		int rc = connect(socket_ids[index], address, addrlen);
		if (rc != EOK) {
			socket_print_error(stderr, rc, "Socket connect: ", "\n");
			return rc;
		}
		
		if (verbose)
			print_mark(index);
	}
	
	return EOK;
}

/** Send data via sockets.
 *
 * @param[in] verbose    Print out verbose information.
 * @param[in] socket_ids Stored socket identifiers.
 * @param[in] sockets    Number of sockets.
 * @param[in] address    Destination host address to send data to.
 * @param[in] addrlen    Length of the destination address in bytes.
 * @param[in] data       Data to be sent.
 * @param[in] size       Size of the data in bytes.
 * @param[in] messages   Number of datagrams per socket to be sent.
 * @param[in] type       Socket type.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the sendto() function.
 *
 */
int sockets_sendto(int verbose, int *socket_ids, unsigned int sockets,
    struct sockaddr *address, socklen_t addrlen, char *data, size_t size,
    unsigned int messages, sock_type_t type)
{
	if (verbose)
		printf("\tSendto\t");
	
	fflush(stdout);
	
	for (unsigned int index = 0; index < sockets; index++) {
		for (unsigned int message = 0; message < messages; message++) {
			int rc;
			
			switch (type) {
			case SOCK_STREAM:
				rc = send(socket_ids[index], data, size, 0);
				break;
			case SOCK_DGRAM:
				rc = sendto(socket_ids[index], data, size, 0, address, addrlen);
				break;
			default:
				rc = EINVAL;
			}
			
			if (rc != EOK) {
				printf("Socket %u (%d), message %u error:\n",
				    index, socket_ids[index], message);
				socket_print_error(stderr, rc, "Socket send: ", "\n");
				return rc;
			}
		}
		
		if (verbose)
			print_mark(index);
	}
	
	return EOK;
}

/** Receive data via sockets.
 *
 * @param[in]     verbose    Print out verbose information.
 * @param[in]     socket_ids Stored socket identifiers.
 * @param[in]     sockets    Number of sockets.
 * @param[in]     address    Source host address of received datagrams.
 * @param[in,out] addrlen    Maximum length of the source address in bytes.
 *                           The actual size of the source address is set.
 * @param[out]    data       Received data.
 * @param[in]     size       Maximum data size in bytes.
 * @param[in]     messages   Number of datagrams per socket to be received.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the recvfrom() function.
 *
 */
int sockets_recvfrom(int verbose, int *socket_ids, unsigned int sockets,
    struct sockaddr *address, socklen_t *addrlen, char *data, size_t size,
    unsigned int messages)
{
	if (verbose)
		printf("\tRecvfrom\t");
	
	fflush(stdout);
	
	for (unsigned int index = 0; index < sockets; index++) {
		for (unsigned int message = 0; message < messages; message++) {
			int rc = recvfrom(socket_ids[index], data, size, 0, address, addrlen);
			if (rc < 0) {
				printf("Socket %u (%d), message %u error:\n",
				    index, socket_ids[index], message);
				socket_print_error(stderr, rc, "Socket receive: ", "\n");
				return rc;
			}
		}
		
		if (verbose)
			print_mark(index);
	}
	
	return EOK;
}

/** Send and receive data via sockets.
 *
 * Each datagram is sent and a reply read consequently.
 * The next datagram is sent after the reply is received.
 *
 * @param[in]     verbose    Print out verbose information.
 * @param[in]     socket_ids Stored socket identifiers.
 * @param[in]     sockets    Number of sockets.
 * @param[in,out] address    Destination host address to send data to.
 *                           The source host address of received datagrams is set.
 * @param[in]     addrlen    Length of the destination address in bytes.
 * @param[in,out] data       Data to be sent. The received data are set.
 * @param[in]     size       Size of the data in bytes.
 * @param[in]     messages   Number of datagrams per socket to be received.
 * @param[in]     type       Socket type.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the recvfrom() function.
 *
 */
int sockets_sendto_recvfrom(int verbose, int *socket_ids, unsigned int sockets,
    struct sockaddr *address, socklen_t *addrlen, char *data,
    size_t size, unsigned int messages, sock_type_t type)
{
	if (verbose)
		printf("\tSendto and recvfrom\t");
	
	fflush(stdout);
	
	for (unsigned int index = 0; index < sockets; index++) {
		for (unsigned int message = 0; message < messages; message++) {
			int rc;
			
			switch (type) {
			case SOCK_STREAM:
				rc = send(socket_ids[index], data, size, 0);
				break;
			case SOCK_DGRAM:
				rc = sendto(socket_ids[index], data, size, 0, address, *addrlen);
				break;
			default:
				rc = EINVAL;
			}
			
			if (rc != EOK) {
				printf("Socket %u (%d), message %u error:\n",
				    index, socket_ids[index], message);
				socket_print_error(stderr, rc, "Socket send: ", "\n");
				return rc;
			}
			
			rc = recvfrom(socket_ids[index], data, size, 0, address, addrlen);
			if (rc < 0) {
				printf("Socket %u (%d), message %u error:\n",
				    index, socket_ids[index], message);
				socket_print_error(stderr, rc, "Socket receive: ", "\n");
				return rc;
			}
		}
		
		if (verbose)
			print_mark(index);
	}
	
	return EOK;
}

/** Prints a mark.
 *
 * If the index is a multiple of ten, a different mark is printed.
 *
 * @param[in] index The index of the mark to be printed.
 *
 */
void print_mark(unsigned int index)
{
	if ((index + 1) % 10)
		printf("*");
	else
		printf("|");
	
	fflush(stdout);
}

/** @}
 */
