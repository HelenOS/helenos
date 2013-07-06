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
 *  INET6 family common definitions.
 */

#ifndef LIBC_IN6_H_
#define LIBC_IN6_H_

#include <net/inet.h>
#include <net/ip_protocols.h>
#include <sys/types.h>

/** INET6 string address maximum length. */
#define INET6_ADDRSTRLEN  (8 * 4 + 7 + 1)

/** INET6 address. */
typedef struct in6_addr {
	/** 16 byte IPv6 address. */
	uint8_t s6_addr[16];
} in6_addr_t;

/** INET6 socket address.
 * @see sockaddr
 */
typedef struct sockaddr_in6 {
	/** Address family. Should be AF_INET6. */
	uint16_t sin6_family;
	/** Port number. */
	uint16_t sin6_port;
	/** IPv6 flow information. */
	uint32_t sin6_flowinfo;
	/** IPv6 address. */
	struct in6_addr sin6_addr;
	/** Scope identifier. */
	uint32_t sin6_scope_id;
} sockaddr_in6_t;

extern const in6_addr_t in6addr_any;

#endif

/** @}
 */
