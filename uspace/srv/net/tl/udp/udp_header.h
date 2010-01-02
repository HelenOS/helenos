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

/** @addtogroup udp
 *  @{
 */

/** @file
 *  UDP header definition.
 *  Based on the RFC~768.
 */

#ifndef __NET_UDP_HEADER_H__
#define __NET_UDP_HEADER_H__

#include <sys/types.h>

/** Type definition of the user datagram header.
 *  @see udp_header
 */
typedef struct udp_header	udp_header_t;

/** Type definition of the user datagram header pointer.
 *  @see udp_header
 */
typedef udp_header_t *		udp_header_ref;

/** User datagram header.
 */
struct udp_header{
	/** Source Port is an optional field, when meaningful, it indicates the port of the sending process, and may be assumed to be the port to which a reply should be addressed in the absence of any other information.
	 *  If not used, a value of zero is inserted.
	 */
	uint16_t	source_port;
	/** Destination port has a meaning within the context of a particular internet destination address.
	 */
	uint16_t	destination_port;
	/** Length is the length in octets of this user datagram including this header and the data.
	 *  This means the minimum value of the length is eight.
	 */
	uint16_t	total_length;
	/** Checksum is the 16-bit one's complement of the one's complement sum of a pseudo header of information from the IP header, the UDP header, and the data, padded with zero octets at the end (if necessary) to make a multiple of two octets.
	 *  The pseudo header conceptually prefixed to the UDP header contains the source address, the destination address, the protocol, and the UDP length.
	 *  This information gives protection against misrouted datagrams.
	 *  If the computed checksum is zero, it is transmitted as all ones (the equivalent in one's complement arithmetic).
	 *  An all zero transmitted checksum value means that the transmitter generated no checksum (for debugging or for higher level protocols that don't care).
	 */
	uint16_t	checksum;
} __attribute__ ((packed));

#endif

/** @}
 */
