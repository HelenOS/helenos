/*
 * Copyright (c) 2012 Jiri Svoboda
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

/** @addtogroup udp
 * @{
 */
/** @file UDP type definitions
 */

#ifndef UDP_TYPE_H
#define UDP_TYPE_H

#include <socket_core.h>
#include <sys/types.h>

typedef struct {
	uint32_t ipv4;
} netaddr_t;

typedef struct {
	netaddr_t addr;
	uint16_t port;
} udp_sock_t;

typedef struct {
	udp_sock_t local;
	udp_sock_t foreign;
} udp_sockpair_t;

/** Unencoded UDP message (datagram) */
typedef struct {
	/** Message data */
	void *data;
	/** Message data size */
	size_t data_size;
} udp_msg_t;

/** Encoded PDU */
typedef struct {
	/** Source address */
	netaddr_t src;
	/** Destination address */
	netaddr_t dest;

	/** Encoded PDU data including header */
	void *data;
	/** Encoded PDU data size */
	size_t data_size;
} udp_pdu_t;

#endif

/** @}
 */
