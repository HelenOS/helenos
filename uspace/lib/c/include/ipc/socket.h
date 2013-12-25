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

/** @addtogroup libc
 * @{
 */

/** @file
 * Socket messages.
 */

#ifndef LIBC_SOCKET_MESSAGES_H_
#define LIBC_SOCKET_MESSAGES_H_

/** Socket client messages. */
typedef enum {
	/** Creates a new socket. @see socket() */
	NET_SOCKET = IPC_FIRST_USER_METHOD,
	/** Binds the socket. @see bind() */
	NET_SOCKET_BIND,
	/** Creates a new socket. @see socket() */
	NET_SOCKET_LISTEN,
	/** Accepts an incomming connection. @see accept() */
	NET_SOCKET_ACCEPT,
	/** Connects the socket. @see connect() */
	NET_SOCKET_CONNECT,
	/** Closes the socket. @see closesocket() */
	NET_SOCKET_CLOSE,
	/** Sends data via the stream socket. @see send() */
	NET_SOCKET_SEND,
	/** Sends data via the datagram socket. @see sendto() */
	NET_SOCKET_SENDTO,
	/** Receives data from the stream socket. @see socket() */
	NET_SOCKET_RECV,
	/** Receives data from the datagram socket. @see socket() */
	NET_SOCKET_RECVFROM,
	/** Gets the socket option. @see getsockopt() */
	NET_SOCKET_GETSOCKOPT,
	/** Sets the socket option. @see setsockopt() */
	NET_SOCKET_SETSOCKOPT,
	/** New socket for acceptence notification message. */
	NET_SOCKET_ACCEPTED,
	/** New data received notification message. */
	NET_SOCKET_RECEIVED,
	/** New socket data fragment size notification message. */
	NET_SOCKET_DATA_FRAGMENT_SIZE
} socket_messages;

/** @name Socket specific message parameters definitions
 */
/*@{*/

/** Sets the socket identifier in the message answer.
 * @param[out] answer	The message answer structure.
 */
#define SOCKET_SET_SOCKET_ID(answer, value) \
	do { \
		sysarg_t argument = (sysarg_t) (value); \
		IPC_SET_ARG1(answer, argument); \
	} while (0)

/** Returns the socket identifier message parameter.
 * @param[in] call	The message call structure.
 */
#define SOCKET_GET_SOCKET_ID(call) \
	({ \
		int socket_id = (int) IPC_GET_ARG1(call); \
		socket_id; \
	})

/** Sets the read data length in the message answer.
 * @param[out] answer	The message answer structure.
 */
#define SOCKET_SET_READ_DATA_LENGTH(answer, value) \
	do { \
		sysarg_t argument = (sysarg_t) (value); \
		IPC_SET_ARG1(answer, argument); \
	} while (0)

/** Returns the read data length message parameter.
 * @param[in] call	The message call structure.
 */
#define SOCKET_GET_READ_DATA_LENGTH(call) \
	({ \
		int data_length = (int) IPC_GET_ARG1(call); \
		data_length; \
	})

/** Returns the backlog message parameter.
 * @param[in] call	The message call structure.
 */
#define SOCKET_GET_BACKLOG(call) \
	({ \
		int backlog = (int) IPC_GET_ARG2(call); \
		backlog; \
	})

/** Returns the option level message parameter.
 * @param[in] call	The message call structure.
 */
#define SOCKET_GET_OPT_LEVEL(call) \
	({ \
		int opt_level = (int) IPC_GET_ARG2(call); \
		opt_level; \
	})

/** Returns the data fragment size message parameter.
 * @param[in] call	The message call structure.
 */
#define SOCKET_GET_DATA_FRAGMENT_SIZE(call) \
	({ \
		size_t size = (size_t) IPC_GET_ARG2(call); \
		size; \
	})

/** Sets the data fragment size in the message answer.
 * @param[out] answer	The message answer structure.
 */
#define SOCKET_SET_DATA_FRAGMENT_SIZE(answer, value) \
	do { \
		sysarg_t argument = (sysarg_t) (value); \
		IPC_SET_ARG2(answer, argument); \
	} while (0)

/** Sets the address length in the message answer.
 * @param[out] answer	The message answer structure.
 */
#define SOCKET_SET_ADDRESS_LENGTH(answer, value) \
	do { \
		sysarg_t argument = (sysarg_t) (value); \
		IPC_SET_ARG3(answer, argument);\
	} while (0)

/** Returns the address length message parameter.
 * @param[in] call	The message call structure.
 */
#define SOCKET_GET_ADDRESS_LENGTH(call) \
	({ \
		socklen_t address_length = (socklen_t) IPC_GET_ARG3(call); \
		address_length; \
	})

/** Sets the header size in the message answer.
 * @param[out] answer	The message answer structure.
 */
#define SOCKET_SET_HEADER_SIZE(answer, value) \
	do { \
		sysarg_t argument = (sysarg_t) (value); \
		IPC_SET_ARG3(answer, argument); \
	} while (0)

/** Returns the header size message parameter.
 *  @param[in] call	The message call structure.
 */
#define SOCKET_GET_HEADER_SIZE(call) \
	({ \
		size_t size = (size_t) IPC_GET_ARG3(call); \
		size; \
	})

/** Returns the flags message parameter.
 *  @param[in] call	The message call structure.
 */
#define SOCKET_GET_FLAGS(call) \
	({ \
		int flags = (int) IPC_GET_ARG4(call); \
		flags; \
	})

/** Returns the option name message parameter.
 *  @param[in] call	The message call structure.
 */
#define SOCKET_GET_OPT_NAME(call) \
	({ \
		int opt_name = (int) IPC_GET_ARG2(call); \
		opt_name; \
	})

/** Returns the data fragments message parameter.
 *  @param[in] call	The message call structure.
 */
#define SOCKET_GET_DATA_FRAGMENTS(call) \
	({ \
		int fragments = (int) IPC_GET_ARG5(call); \
		fragments; \
	})

/** Returns the new socket identifier message parameter.
 *  @param[in] call	The message call structure.
 */
#define SOCKET_GET_NEW_SOCKET_ID(call) \
	({ \
		int socket_id = (int) IPC_GET_ARG5(call); \
		socket_id; \
	})

/*@}*/

#endif

/** @}
 */
