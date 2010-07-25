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

/** @addtogroup ip
 *  @{
 */

/** @file
 *  IP header and options definitions.
 *  Based on the RFC~791.
 */

#ifndef __NET_IP_HEADER_H__
#define __NET_IP_HEADER_H__

#include <byteorder.h>
#include <sys/types.h>

/** Returns the fragment offest high bits.
 *  @param[in] length The prefixed data total length.
 */
#define IP_COMPUTE_FRAGMENT_OFFSET_HIGH(length) ((((length) / 8u) &0x1F00) >> 8)

/** Returns the fragment offest low bits.
 *  @param[in] length The prefixed data total length.
 */
#define IP_COMPUTE_FRAGMENT_OFFSET_LOW(length) (((length) / 8u) &0xFF)

/** Returns the IP header length.
 *  @param[in] length The IP header length in bytes.
 */
#define IP_COMPUTE_HEADER_LENGTH(length)		((uint8_t) ((length) / 4u))

/** Returns the fragment offest.
 *  @param[in] header The IP packet header.
 */
#define IP_FRAGMENT_OFFSET(header) ((((header)->fragment_offset_high << 8) + (header)->fragment_offset_low) * 8u)

/** Returns the IP packet header checksum.
 *  @param[in] header The IP packet header.
 */
#define IP_HEADER_CHECKSUM(header)	(htons(ip_checksum((uint8_t *)(header), IP_HEADER_LENGTH(header))))

/** Returns the actual IP packet data length.
 *  @param[in] header The IP packet header.
 */
#define IP_HEADER_DATA_LENGTH(header)	(IP_TOTAL_LENGTH(header) - IP_HEADER_LENGTH(header))

/** Returns the actual IP header length in bytes.
 *  @param[in] header The IP packet header.
 */
#define IP_HEADER_LENGTH(header)		((header)->header_length * 4u)

/** Returns the actual IP packet total length.
 *  @param[in] header The IP packet header.
 */
#define IP_TOTAL_LENGTH(header)		ntohs((header)->total_length)

/** @name IP flags definitions
 */
/*@{*/

/** Fragment flag field shift.
 */
#define IPFLAG_FRAGMENT_SHIFT		1

/** Fragmented flag field shift.
 */
#define IPFLAG_FRAGMENTED_SHIFT		0

/** Don't fragment flag value.
 *  Permits the packet fragmentation.
 */
#define IPFLAG_DONT_FRAGMENT		(0x1 << IPFLAG_FRAGMENT_SHIFT)

/** Last fragment flag value.
 *  Indicates the last packet fragment.
 */
#define IPFLAG_LAST_FRAGMENT		(0x0 << IPFLAG_FRAGMENTED_SHIFT)

/** May fragment flag value.
 *  Allows the packet fragmentation.
 */
#define IPFLAG_MAY_FRAGMENT			(0x0 << IPFLAG_FRAGMENT_SHIFT)

/** More fragments flag value.
 *  Indicates that more packet fragments follow.
 */
#define IPFLAG_MORE_FRAGMENTS		(0x1 << IPFLAG_FRAGMENTED_SHIFT)

/*@}*/

/** Type definition of the internet header.
 *  @see ip_header
 */
typedef struct ip_header	ip_header_t;

/** Type definition of the internet header pointer.
 *  @see ip_header
 */
typedef ip_header_t *		ip_header_ref;

/** Type definition of the internet option header.
 *  @see ip_header
 */
typedef struct ip_option	ip_option_t;

/** Type definition of the internet option header pointer.
 *  @see ip_header
 */
typedef ip_option_t *		ip_option_ref;

/** Type definition of the internet version 4 pseudo header.
 *  @see ipv4_pseudo_header
 */
typedef struct ipv4_pseudo_header	ipv4_pseudo_header_t;

/** Type definition of the internet version 4 pseudo header pointer.
 *  @see ipv4_pseudo_header
 */
typedef ipv4_pseudo_header_t *		ipv4_pseudo_header_ref;

/** Internet header.
 *  The variable options should be included after the header itself and indicated by the increased header length value.
 */
struct ip_header{
#ifdef ARCH_IS_BIG_ENDIAN
	/** The Version field indicates the format of the internet header.
	 */
	uint8_t version:4;
	/** Internet Header Length is the length of the internet header in 32~bit words, and thus points to the beginning of the data.
	 *  Note that the minimum value for a~correct header is~5.
	 */
	uint8_t header_length:4;
#else
	/** Internet Header Length is the length of the internet header in 32~bit words, and thus points to the beginning of the data.
	 *  Note that the minimum value for a~correct header is~5.
	 */
	uint8_t header_length:4;
	/** The Version field indicates the format of the internet header.
	 */
	uint8_t version:4;
#endif
	/** The Type of Service provides an indication of the abstract parameters of the quality of service desired.
	 *  These parameters are to be used to guide the selection of the actual service parameters when transmitting a~datagram through a~particular network.
	 *  Several networks offer service precedence, which somehow treats high precedence traffic as more important than other traffic (generally by accepting only traffic above a~certain precedence at time of high load).
	 *  The major choice is a~three way tradeoff between low-delay, high-reliability, and high-throughput.
	 */
	uint8_t tos;
	/** Total Length is the length of the datagram, measured in octets, including internet header and data.
	 *  This field allows the length of a~datagram to be up to 65,535~octets.
	 */
	uint16_t total_length;
	/** An identifying value assigned by the sender to aid in assembling the fragments of a~datagram.
	 */
	uint16_t identification;
#ifdef ARCH_IS_BIG_ENDIAN
	/** Various control flags.
	 */
	uint8_t flags:3;
	/** This field indicates where in the datagram this fragment belongs.
	 *  High bits.
	 */
	uint8_t fragment_offset_high:5;
#else
	/** This field indicates where in the datagram this fragment belongs.
	 *  High bits.
	 */
	uint8_t fragment_offset_high:5;
	/** Various control flags.
	 */
	uint8_t flags:3;
#endif
	/** This field indicates where in the datagram this fragment belongs.
	 *  Low bits.
	 */
	uint8_t fragment_offset_low;
	/** This field indicates the maximum time the datagram is allowed to remain in the internet system.
	 *  If this field contains the value zero, then the datagram must be destroyed.
	 *  This field is modified in internet header processing.
	 *  The time is measured in units of seconds, but since every module that processes a~datagram must decrease the TTL by at least one even if it process the datagram in less than a~second, the TTL must be thought of only as an upper bound on the time a~datagram may exist.
	 *  The intention is to cause undeliverable datagrams to be discarded, and to bound the maximum datagram lifetime.
	 */
	uint8_t ttl;
	/** This field indicates the next level protocol used in the data portion of the internet datagram.
	 */
	uint8_t protocol;
	/** A checksum of the header only.
	 *  Since some header fields change (e.g., time to live), this is recomputed and verified at each point that the internet header is processed.
	 *  The checksum algorithm is: The checksum field is the 16~bit one's complement of the one's complement sum of all 16~bit words in the header.
	 *  For purposes of computing the checksum, the value of the checksum field is zero.
	 */
	uint16_t header_checksum;
	/** The source address.
	 */
	uint32_t source_address;
	/** The destination address.
	 */
	uint32_t destination_address;
} __attribute__ ((packed));

/** Internet option header.
 *  Only type field is always valid.
 *  Other fields' validity depends on the option type.
 */
struct ip_option{
	/** A single octet of option-type.
	 */
	uint8_t type;
	/** An option length octet.
	 */
	uint8_t length;
	/** A~pointer.
	 */
	uint8_t pointer;
#ifdef ARCH_IS_BIG_ENDIAN
	/** The number of IP modules that cannot register timestamps due to lack of space.
	 */
	uint8_t overflow:4;
	/** Various internet timestamp control flags.
	 */
	uint8_t flags:4;
#else
	/** Various internet timestamp control flags.
	 */
	uint8_t flags:4;
	/** The number of IP modules that cannot register timestamps due to lack of space.
	 */
	uint8_t overflow:4;
#endif
} __attribute__ ((packed));

/** Internet version 4 pseudo header.
 */
struct ipv4_pseudo_header{
	/** The source address.
	 */
	uint32_t source_address;
	/** The destination address.
	 */
	uint32_t destination_address;
	/** Reserved byte.
	 *  Must be zero.
	 */
	uint8_t reserved;
	/** This field indicates the next level protocol used in the data portion of the internet datagram.
	 */
	uint8_t protocol;
	/** Data length is the length of the datagram, measured in octets.
	 */
	uint16_t data_length;
} __attribute__ ((packed));

#endif

/** @}
 */
