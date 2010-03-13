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
 *  Networking test support functions.
 */

#ifndef __NET_TEST__
#define __NET_TEST__

#include "../include/socket.h"

/** Prints a mark.
 *  If the index is a multiple of ten, a different mark is printed.
 *  @param[in] index The index of the mark to be printed.
 */
void print_mark(int index);

/** Creates new sockets.
 *  @param[in] verbose A value indicating whether to print out verbose information.
 *  @param[out] socket_ids A field to store the socket identifiers.
 *  @param[in] sockets The number of sockets to create. Should be at most the size of the field.
 *  @param[in] family The socket address family.
 *  @param[in] type The socket type.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the socket() function.
 */
int sockets_create(int verbose, int * socket_ids, int sockets, int family, sock_type_t type);

/** Closes sockets.
 *  @param[in] verbose A value indicating whether to print out verbose information.
 *  @param[in] socket_ids A field of stored socket identifiers.
 *  @param[in] sockets The number of sockets in the field. Should be at most the size of the field.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the closesocket() function.
 */
int sockets_close(int verbose, int * socket_ids, int sockets);

/** Connects sockets.
 *  @param[in] verbose A value indicating whether to print out verbose information.
 *  @param[in] socket_ids A field of stored socket identifiers.
 *  @param[in] sockets The number of sockets in the field. Should be at most the size of the field.
 *  @param[in] address The destination host address to connect to.
 *  @param[in] addrlen The length of the destination address in bytes.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the connect() function.
 */
int sockets_connect(int verbose, int * socket_ids, int sockets, struct sockaddr * address, socklen_t addrlen);

/** Sends data via sockets.
 *  @param[in] verbose A value indicating whether to print out verbose information.
 *  @param[in] socket_ids A field of stored socket identifiers.
 *  @param[in] sockets The number of sockets in the field. Should be at most the size of the field.
 *  @param[in] address The destination host address to send data to.
 *  @param[in] addrlen The length of the destination address in bytes.
 *  @param[in] data The data to be sent.
 *  @param[in] size The data size in bytes.
 *  @param[in] messages The number of datagrams per socket to be sent.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the sendto() function.
 */
int sockets_sendto(int verbose, int * socket_ids, int sockets, struct sockaddr * address, socklen_t addrlen, char * data, int size, int messages);

/** Receives data via sockets.
 *  @param[in] verbose A value indicating whether to print out verbose information.
 *  @param[in] socket_ids A field of stored socket identifiers.
 *  @param[in] sockets The number of sockets in the field. Should be at most the size of the field.
 *  @param[in] address The source host address of received datagrams.
 *  @param[in,out] addrlen The maximum length of the source address in bytes. The actual size of the source address is set instead.
 *  @param[out] data The received data.
 *  @param[in] size The maximum data size in bytes.
 *  @param[in] messages The number of datagrams per socket to be received.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the recvfrom() function.
 */
int sockets_recvfrom(int verbose, int * socket_ids, int sockets, struct sockaddr * address, socklen_t * addrlen, char * data, int size, int messages);

/** Sends and receives data via sockets.
 *  Each datagram is sent and a reply read consequently.
 *  The next datagram is sent after the reply is received.
 *  @param[in] verbose A value indicating whether to print out verbose information.
 *  @param[in] socket_ids A field of stored socket identifiers.
 *  @param[in] sockets The number of sockets in the field. Should be at most the size of the field.
 *  @param[in,out] address The destination host address to send data to. The source host address of received datagrams is set instead.
 *  @param[in] addrlen The length of the destination address in bytes.
 *  @param[in,out] data The data to be sent. The received data are set instead.
 *  @param[in] size The data size in bytes.
 *  @param[in] messages The number of datagrams per socket to be received.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the recvfrom() function.
 */
int sockets_sendto_recvfrom(int verbose, int * socket_ids, int sockets, struct sockaddr * address, socklen_t * addrlen, char * data, int size, int messages);

#endif

/** @}
 */
