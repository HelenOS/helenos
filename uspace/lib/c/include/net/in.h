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
 *  INET family common definitions.
 */

#ifndef LIBC_IN_H_
#define LIBC_IN_H_

#include <net/inet.h>
#include <net/ip_protocols.h>
#include <sys/types.h>

/** INET string address maximum length. */
#define INET_ADDRSTRLEN  (4 * 3 + 3 + 1)

#define INADDR_ANY  0

/** INET address. */
typedef struct in_addr {
	/** 4 byte IP address. */
	uint32_t s_addr;
} in_addr_t;

/** INET socket address.
 * @see sockaddr
 */
typedef struct sockaddr_in {
	/** Address family. Should be AF_INET. */
	uint16_t sin_family;
	/** Port number. */
	uint16_t sin_port;
	/** Internet address. */
	in_addr_t sin_addr;
	/** Padding to meet the sockaddr size. */
	uint8_t sin_zero[8];
} sockaddr_in_t;

#endif

/** @}
 */
