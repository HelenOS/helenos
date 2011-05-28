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
	(((GET_IP_HEADER_FRAGMENT_OFFSET_HIGH(header) << 8) + \
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
	(GET_IP_HEADER_LENGTH(header) * 4U)

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

/** Type definition of the internet option header.
 * @see ip_header
 */
typedef struct ip_option ip_option_t;

/** Type definition of the internet version 4 pseudo header.
 * @see ipv4_pseudo_header
 */
typedef struct ipv4_pseudo_header ipv4_pseudo_header_t;

/** Internet header.
 *
 * The variable options should be included after the header itself and
 * indicated by the increased header length value.
 */
struct ip_header {
	uint8_t vhl; /* version, header_length */

#define GET_IP_HEADER_VERSION(header) \
	(((header)->vhl & 0xf0) >> 4)
#define SET_IP_HEADER_VERSION(header, version) \
	((header)->vhl = \
	 ((version & 0x0f) << 4) | ((header)->vhl & 0x0f))

#define GET_IP_HEADER_LENGTH(header) \
	((header)->vhl & 0x0f)
#define SET_IP_HEADER_LENGTH(header, length) \
	((header)->vhl = \
	 (length & 0x0f) | ((header)->vhl & 0xf0))

	uint8_t tos;
	uint16_t total_length;
	uint16_t identification;

	uint8_t ffoh; /* flags, fragment_offset_high */

#define GET_IP_HEADER_FLAGS(header) \
	(((header)->ffoh & 0xe0) >> 5)
#define SET_IP_HEADER_FLAGS(header, flags) \
	((header)->ffoh = \
	 ((flags & 0x07) << 5) | ((header)->ffoh & 0x1f))

#define GET_IP_HEADER_FRAGMENT_OFFSET_HIGH(header) \
	((header)->ffoh & 0x1f)
#define SET_IP_HEADER_FRAGMENT_OFFSET_HIGH(header, fragment_offset_high) \
	((header)->ffoh = \
	 (fragment_offset_high & 0x1f) | ((header)->ffoh & 0xe0))

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

	uint8_t of; /* overflow, flags */

#define GET_IP_OPTION_OVERFLOW(option) \
	(((option)->of & 0xf0) >> 4)
#define SET_IP_OPTION_OVERFLOW(option, overflow) \
	((option)->of = \
	 ((overflow & 0x0f) << 4) | ((option)->of & 0x0f))

#define GET_IP_OPTION_FLAGS(option) \
	((option)->of & 0x0f)
#define SET_IP_OPTION_FLAGS(option, flags) \
	((option)->of = \
	 (flags & 0x0f) | ((option)->of & 0xf0))

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
