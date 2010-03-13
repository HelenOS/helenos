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

/** @addtogroup net
 *  @{
 */

/** @file
 *  Internet common definitions.
 */

#ifndef __NET_INET_H__
#define __NET_INET_H__

#include <sys/types.h>

#include "byteorder.h"

/** Type definition of the socket address.
 *  @see sockaddr
 */
typedef struct sockaddr		sockaddr_t;

/** Type definition of the address information.
 *  @see addrinfo
 */
typedef struct addrinfo		addrinfo_t;

/** Prints the address into the character buffer.
 *  @param[in] family The address family.
 *  @param[in] data The address data.
 *  @param[out] address The character buffer to be filled.
 *  @param[in] length The buffer length.
 *  @returns EOK on success.
 *  @returns EINVAL if the data or address parameter is NULL.
 *  @returns ENOMEM if the character buffer is not long enough.
 *  @returns ENOTSUP if the address family is not supported.
 */
int inet_ntop(uint16_t family, const uint8_t * data, char * address, size_t length);

/** Parses the character string into the address.
 *  If the string is shorter than the full address, zero bytes are added.
 *  @param[in] family The address family.
 *  @param[in] address The character buffer to be parsed.
 *  @param[out] data The address data to be filled.
 *  @returns EOK on success.
 *  @returns EINVAL if the data parameter is NULL.
 *  @returns ENOENT if the address parameter is NULL.
 *  @returns ENOTSUP if the address family is not supported.
 */
int inet_pton(uint16_t family, const char * address, uint8_t * data);

/** Socket address.
 */
struct sockaddr{
	/** Address family.
	 *  @see socket.h
	 */
	uint16_t sa_family;
	/** 14 byte protocol address.
	 */
	uint8_t sa_data[14];
};

// TODO define address information
// /** Address information.
// * \todo
// */
//struct addrinfo{
//	int				ai_flags;		// AI_PASSIVE, AI_CANONNAME, etc.
//	uint16_t		ai_family;		// AF_INET, AF_INET6, AF_UNSPEC
//	int				ai_socktype;	// SOCK_STREAM, SOCK_DGRAM
//	int				ai_protocol;	// use 0 for "any"
//	size_t			ai_addrlen;		// size of ai_addr in bytes
//	struct sockaddr *	ai_addr;	// struct sockaddr_in or _in6
//	char *			ai_canonname;	// full canonical hostname
//	struct addrinfo *	ai_next;	// linked list, next node
//};

/*int getaddrinfo(const char *node, // e.g. "www.example.com" or IP
const char *service, // e.g. "http" or port number
const struct addrinfo *hints,
struct addrinfo **res);
getnameinfo
*/

#endif

/** @}
 */
