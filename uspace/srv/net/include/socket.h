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

/** @addtogroup socket
 *  @{
 */

/** @file
 *  Socket application program interface (API).
 *  This is a part of the network application library.
 *  Based on the BSD socket interface.
 */

#ifndef __NET_SOCKET_H__
#define __NET_SOCKET_H__

#include "byteorder.h"
#include "in.h"
#include "in6.h"
#include "inet.h"

#include "socket_codes.h"
#include "socket_errno.h"

/** @name Socket application programming interface
 */
/*@{*/

/** Creates a new socket.
 *  @param[in] domain The socket protocol family.
 *  @param[in] type Socket type.
 *  @param[in] protocol Socket protocol.
 *  @returns The socket identifier on success.
 *  @returns EPFNOTSUPPORT if the protocol family is not supported.
 *  @returns ESOCKNOTSUPPORT if the socket type is not supported.
 *  @returns EPROTONOSUPPORT if the protocol is not supported.
 *  @returns ENOMEM if there is not enough memory left.
 *  @returns Other error codes as defined for the NET_SOCKET message.
 */
int	socket( int domain, int type, int protocol );

/** Binds the socket to a port address.
 *  @param[in] socket_id Socket identifier.
 *  @param[in] my_addr The port address.
 *  @param[in] addrlen The address length.
 *  @returns EOK on success.
 *  @returns ENOTSOCK if the socket is not found.
 *  @returns EBADMEM if the my_addr parameter is NULL.
 *  @returns NO_DATA if the addlen parameter is zero (0).
 *  @returns Other error codes as defined for the NET_SOCKET_BIND message.
 */
int	bind( int socket_id, const struct sockaddr * my_addr, socklen_t addrlen );

/** Sets the number of connections waiting to be accepted.
 *  @param[in] socket_id Socket identifier.
 *  @param[in] backlog The maximum number of waiting sockets to be accepted.
 *  @returns EOK on success.
 *  @returns EINVAL if the backlog parameter is not positive (<=0).
 *  @returns ENOTSOCK if the socket is not found.
 *  @returns Other error codes as defined for the NET_SOCKET_LISTEN message.
 */
int	listen( int socket_id, int backlog );

/** Accepts waiting socket.
 *  Blocks until such a socket exists.
 *  @param[in] socket_id Socket identifier.
 *  @param[out] cliaddr The remote client address.
 *  @param[in] addrlen The address length.
 *  @returns EOK on success.
 *  @returns EBADMEM if the cliaddr or addrlen parameter is NULL.
 *  @returns EINVAL if the backlog parameter is not positive (<=0).
 *  @returns ENOTSOCK if the socket is not found.
 *  @returns Other error codes as defined for the NET_SOCKET_ACCEPT message.
 */
int	accept( int socket_id, struct sockaddr * cliaddr, socklen_t * addrlen );

/** Connects socket to the remote server.
 *  @param[in] socket_id Socket identifier.
 *  @param[in] serv_addr The remote server address.
 *  @param[in] addrlen The address length.
 *  @returns EOK on success.
 *  @returns EBADMEM if the serv_addr parameter is NULL.
 *  @returns NO_DATA if the addlen parameter is zero (0).
 *  @returns ENOTSOCK if the socket is not found.
 *  @returns Other error codes as defined for the NET_SOCKET_CONNECT message.
 */
int	connect( int socket_id, const struct sockaddr * serv_addr, socklen_t addrlen );

/** Closes the socket.
 *  @param[in] socket_id Socket identifier.
 *  @returns EOK on success.
 *  @returns ENOTSOCK if the socket is not found.
 *  @returns EINPROGRESS if there is another blocking function in progress.
 *  @returns Other error codes as defined for the NET_SOCKET_CLOSE message.
 */
int	closesocket( int socket_id );

/** Sends data via the socket.
 *  @param[in] socket_id Socket identifier.
 *  @param[in] data The data to be sent.
 *  @param[in] datalength The data length.
 *  @param[in] flags Various send flags.
 *  @returns EOK on success.
 *  @returns ENOTSOCK if the socket is not found.
 *  @returns EBADMEM if the data parameter is NULL.
 *  @returns NO_DATA if the datalength parameter is zero (0).
 *  @returns Other error codes as defined for the NET_SOCKET_SEND message.
 */
int send( int socket_id, void * data, size_t datalength, int flags );

/** Sends data via the socket to the remote address.
 *  Binds the socket to a free port if not already connected/bound.
 *  @param[in] socket_id Socket identifier.
 *  @param[in] data The data to be sent.
 *  @param[in] datalength The data length.
 *  @param[in] flags Various send flags.
 *  @param[in] toaddr The destination address.
 *  @param[in] addrlen The address length.
 *  @returns EOK on success.
 *  @returns ENOTSOCK if the socket is not found.
 *  @returns EBADMEM if the data or toaddr parameter is NULL.
 *  @returns NO_DATA if the datalength or the addrlen parameter is zero (0).
 *  @returns Other error codes as defined for the NET_SOCKET_SENDTO message.
 */
int sendto( int socket_id, const void * data, size_t datalength, int flags, const struct sockaddr * toaddr, socklen_t addrlen );

/** Receives data via the socket.
 *  @param[in] socket_id Socket identifier.
 *  @param[out] data The data buffer to be filled.
 *  @param[in] datalength The data length.
 *  @param[in] flags Various receive flags.
 *  @returns EOK on success.
 *  @returns ENOTSOCK if the socket is not found.
 *  @returns EBADMEM if the data parameter is NULL.
 *  @returns NO_DATA if the datalength parameter is zero (0).
 *  @returns Other error codes as defined for the NET_SOCKET_RECV message.
 */
int recv( int socket_id, void * data, size_t datalength, int flags );

/** Receives data via the socket.
 *  @param[in] socket_id Socket identifier.
 *  @param[out] data The data buffer to be filled.
 *  @param[in] datalength The data length.
 *  @param[in] flags Various receive flags.
 *  @param[out] fromaddr The source address.
 *  @param[in,out] addrlen The address length. The maximum address length is read. The actual address length is set.
 *  @returns EOK on success.
 *  @returns ENOTSOCK if the socket is not found.
 *  @returns EBADMEM if the data or fromaddr parameter is NULL.
 *  @returns NO_DATA if the datalength or addrlen parameter is zero (0).
 *  @returns Other error codes as defined for the NET_SOCKET_RECVFROM message.
 */
int recvfrom( int socket_id, void * data, size_t datalength, int flags, struct sockaddr * fromaddr, socklen_t * addrlen );

/** Gets socket option.
 *  @param[in] socket_id Socket identifier.
 *  @param[in] level The socket options level.
 *  @param[in] optname The socket option to be get.
 *  @param[out] value The value buffer to be filled.
 *  @param[in,out] optlen The value buffer length. The maximum length is read. The actual length is set.
 *  @returns EOK on success.
 *  @returns ENOTSOCK if the socket is not found.
 *  @returns EBADMEM if the value or optlen parameter is NULL.
 *  @returns NO_DATA if the optlen parameter is zero (0).
 *  @returns Other error codes as defined for the NET_SOCKET_GETSOCKOPT message.
 */
int	getsockopt( int socket_id, int level, int optname, void * value, size_t * optlen );

/** Sets socket option.
 *  @param[in] socket_id Socket identifier.
 *  @param[in] level The socket options level.
 *  @param[in] optname The socket option to be set.
 *  @param[in] value The value to be set.
 *  @param[in] optlen The value length.
 *  @returns EOK on success.
 *  @returns ENOTSOCK if the socket is not found.
 *  @returns EBADMEM if the value parameter is NULL.
 *  @returns NO_DATA if the optlen parameter is zero (0).
 *  @returns Other error codes as defined for the NET_SOCKET_SETSOCKOPT message.
 */
int	setsockopt( int socket_id, int level, int optname, const void * value, size_t optlen );

/*@}*/

#endif

/** @}
 */
