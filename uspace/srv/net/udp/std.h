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
/** @file UDP standard definitions
 *
 * Based on IETF RFC 768
 */

#ifndef STD_H
#define STD_H

#include <stdint.h>

#define IP_PROTO_UDP  17

/** UDP Header */
typedef struct {
	/** Source port */
	uint16_t src_port;
	/** Destination port */
	uint16_t dest_port;
	/** Length (header + data) */
	uint16_t length;
	/* Checksum */
	uint16_t checksum;
} udp_header_t;

/** UDP over IPv4 checksum pseudo header */
typedef struct {
	/** Source address */
	uint32_t src_addr;
	/** Destination address */
	uint32_t dest_addr;
	/** Zero */
	uint8_t zero;
	/** Protocol */
	uint8_t protocol;
	/** UDP length */
	uint16_t udp_length;
} udp_phdr_t;

/** UDP over IPv6 checksum pseudo header */
typedef struct {
	/** Source address */
	addr128_t src_addr;
	/** Destination address */
	addr128_t dest_addr;
	/** UDP length */
	uint32_t udp_length;
	/** Zeroes */
	uint8_t zeroes[3];
	/** Next header */
	uint8_t next;
} udp_phdr6_t;

#endif

/** @}
 */
