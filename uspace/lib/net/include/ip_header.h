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

/** @addtogroup libnet 
 * @{
 */

/** @file
 * IP header and options definitions.
 * Based on the RFC 791.
 */

#ifndef LIBNET_IP_HEADER_H_
#define LIBNET_IP_HEADER_H_

#include <byteorder.h>
#include <sys/types.h>

/** Returns the fragment offest high bits.
 * @param[in] length The prefixed data total length.
 */
#define IP_COMPUTE_FRAGMENT_OFFSET_HIGH(length) \
	((((length) / 8U) & 0x1f00) >> 8)

/** Returns the fragment offest low bits.
 * @param[in] length The prefixed data total length.
 */
#define IP_COMPUTE_FRAGMENT_OFFSET_LOW(length) \
	(((length) / 8U) & 0xff)

/** Returns the IP header length.
 * @param[in] length The IP header length in bytes.
 */
#define IP_COMPUTE_HEADER_LENGTH(length) \
	((uint8_t) ((length) / 4U))

/** Returns the fragment offest.
 * @param[in] header The IP packet header.
 */
#define IP_FRAGMENT_OFFSET(header) \
	((((header)->fragment_offset_high << 8) + \
	    (header)->fragment_offset_low) * 8U)

/** Returns the IP packet header checksum.
 *  @param[in] header The IP packet header.
 */
#define IP_HEADER_CHECKSUM(header) \
	(htons(ip_checksum((uint8_t *) (header), IP_HEADER_LENGTH(header))))

/** Returns the actual IP packet data length.
 * @param[in] header The IP packet header.
 */
#define IP_HEADER_DATA_LENGTH(header) \
	(IP_TOTAL_LENGTH(header) - IP_HEADER_LENGTH(header))

/** Returns the actual IP header length in bytes.
 * @param[in] header The IP packet header.
 */
#define IP_HEADER_LENGTH(header) \
	((header)->header_length * 4U)

/** Returns the actual IP packet total length.
 * @param[in] header The IP packet header.
 */
#define IP_TOTAL_LENGTH(header) \
	ntohs((header)->total_length)

/** @name IP flags definitions */
/*@{*/

/** Fragment flag field shift. */
#define IPFLAG_FRAGMENT_SHIFT	1

/** Fragmented flag field shift. */
#define IPFLAG_FRAGMENTED_SHIFT	0

/** Don't fragment flag value.
 * Permits the packet fragmentation.
 */
#define IPFLAG_DONT_FRAGMENT	(0x1 << IPFLAG_FRAGMENT_SHIFT)

/** Last fragment flag value.
 * Indicates the last packet fragment.
 */
#define IPFLAG_LAST_FRAGMENT	(0x0 << IPFLAG_FRAGMENTED_SHIFT)

/** May fragment flag value.
 * Allows the packet fragmentation.
 */
#define IPFLAG_MAY_FRAGMENT	(0x0 << IPFLAG_FRAGMENT_SHIFT)

/** More fragments flag value.
 * Indicates that more packet fragments follow.
 */
#define IPFLAG_MORE_FRAGMENTS	(0x1 << IPFLAG_FRAGMENTED_SHIFT)

/*@}*/

/** Type definition of the internet header.
 * @see ip_header
 */
typedef struct ip_header ip_header_t;

/** Type definition of the internet header pointer.
 * @see ip_header
 */
typedef ip_header_t *ip_header_ref;

/** Type definition of the internet option header.
 * @see ip_header
 */
typedef struct ip_option ip_option_t;

/** Type definition of the internet option header pointer.
 * @see ip_header
 */
typedef ip_option_t *ip_option_ref;

/** Type definition of the internet version 4 pseudo header.
 * @see ipv4_pseudo_header
 */
typedef struct ipv4_pseudo_header ipv4_pseudo_header_t;

/** Type definition of the internet version 4 pseudo header pointer.
 * @see ipv4_pseudo_header
 */
typedef ipv4_pseudo_header_t *ipv4_pseudo_header_ref;

/** Internet header.
 *
 * The variable options should be included after the header itself and
 * indicated by the increased header length value.
 */
struct ip_header {
#ifdef ARCH_IS_BIG_ENDIAN
	uint8_t version : 4;
	uint8_t header_length : 4;
#else
	uint8_t header_length : 4;
	uint8_t version : 4;
#endif

	uint8_t tos;
	uint16_t total_length;
	uint16_t identification;

#ifdef ARCH_IS_BIG_ENDIAN
	uint8_t flags : 3;
	uint8_t fragment_offset_high : 5;
#else
	uint8_t fragment_offset_high : 5;
	uint8_t flags : 3;
#endif

	uint8_t fragment_offset_low;
	uint8_t ttl;
	uint8_t protocol;
	uint16_t header_checksum;
	uint32_t source_address;
	uint32_t destination_address;
} __attribute__ ((packed));

/** Internet option header.
 *
 * Only type field is always valid.
 * Other fields' validity depends on the option type.
 */
struct ip_option {
	uint8_t type;
	uint8_t length;
	uint8_t pointer;

#ifdef ARCH_IS_BIG_ENDIAN
	uint8_t overflow : 4;
	uint8_t flags : 4;
#else
	uint8_t flags : 4;
	uint8_t overflow : 4;
#endif
} __attribute__ ((packed));

/** Internet version 4 pseudo header. */
struct ipv4_pseudo_header {
	uint32_t source_address;
	uint32_t destination_address;
	uint8_t reserved;
	uint8_t protocol;
	uint16_t data_length;
} __attribute__ ((packed));

#endif

/** @}
 */
