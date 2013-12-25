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
 *  @{
 */

/** @file
 *  Socket codes and definitions.
 *  This is a part of the network application library.
 */

#ifndef LIBC_SOCKET_CODES_H_
#define LIBC_SOCKET_CODES_H_

#include <sys/types.h>

/** @name Address families definitions */
/*@{*/

enum {
	AF_NONE = 0,
	AF_INET,  /* IPv4 address */
	AF_INET6  /* IPv6 address */
};

/*@}*/

/** @name Protocol families definitions
 * Same as address families.
 */
/*@{*/

#define PF_INET   AF_INET
#define PF_INET6  AF_INET6

/*@}*/

/** Socket types. */
typedef enum sock_type {
	/** Stream (connection oriented) socket. */
	SOCK_STREAM = 1,
	/** Datagram (connectionless oriented) socket. */
	SOCK_DGRAM = 2,
	/** Raw socket. */
	SOCK_RAW = 3
} sock_type_t;

/** Type definition of the socket length. */
typedef int32_t socklen_t;

/* Socket options */

enum {
	SOL_SOCKET = 1,

	/* IP link to transmit on */
	SO_IPLINK
};

#endif

/** @}
 */
