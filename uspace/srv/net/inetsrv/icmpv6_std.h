/*
 * Copyright (c) 2013 Antonin Steinhauser
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

/** @addtogroup inet
 * @{
 */
/**
 * @file ICMPv6 standard definitions
 *
 */

#ifndef ICMPV6_STD_H_
#define ICMPV6_STD_H_

#include <stdint.h>

#define IP_PROTO_ICMPV6  58

/** Type of service used for ICMP */
#define ICMPV6_TOS  0

#define INET6_HOP_LIMIT_MAX  255

#define NDP_FLAG_ROUTER     0x80
#define NDP_FLAG_OVERRIDE   0x40
#define NDP_FLAG_SOLICITED  0x20

/** ICMPv6 message type */
enum icmpv6_type {
	ICMPV6_ECHO_REQUEST = 128,
	ICMPV6_ECHO_REPLY = 129,
	ICMPV6_ROUTER_SOLICITATION = 133,
	ICMPV6_ROUTER_ADVERTISEMENT = 134,
	ICMPV6_NEIGHBOUR_SOLICITATION = 135,
	ICMPV6_NEIGHBOUR_ADVERTISEMENT = 136
};

/** NDP options */
enum ndp_option {
	SOURCE_LINK_LAYER = 1,
	TARGET_LINK_LAYER = 2,
	PREFIX_INFORMATION = 3
};

/** ICMPv6 message header */
typedef struct {
	/** ICMPv6 message type */
	uint8_t type;
	/** Code (0) */
	uint8_t code;
	/** Internet checksum of the ICMP message */
	uint16_t checksum;
	union {
		struct {
			/** Indentifier */
			uint16_t ident;
			/** Sequence number */
			uint16_t seq_no;
		} echo;
		struct {
			/** Flags byte */
			uint8_t flags;
			/** Reserved bytes */
			uint8_t reserved[3];
		} ndp;
	} un;
} icmpv6_message_t;

/** ICMPv6 pseudoheader for checksum computation */
typedef struct {
	/** Source IPv6 address */
	uint8_t src_addr[16];
	/** Target IPv6 address */
	uint8_t dest_addr[16];
	/** ICMPv6 length */
	uint32_t length;
	/** Zeroes */
	uint8_t zeroes[3];
	/** Next header */
	uint8_t next;
} icmpv6_phdr_t;

/** NDP neighbour body */
typedef struct {
	/** Target IPv6 address */
	uint8_t target_address[16];
	/** Option code */
	uint8_t option;
	/** Option length */
	uint8_t length;
	/** MAC address */
	uint8_t mac[6];
} ndp_message_t;

/** NDP prefix information structure */
typedef struct {
	/** Option code - must be 3 = PREFIX_INFORMATION */
	uint8_t option;
	/** Option length - may be 4 */
	uint8_t length;
	/** Prefix length */
	uint8_t prefixlen;
	/** Flags */
	uint8_t flags;
	/** Valid lifetime */
	uint32_t valid_lftm;
	/** Preferred lifetime */
	uint32_t pref_lftm;
	/** Reserved */
	uint32_t reserved;
	/** Prefix */
	uint8_t prefix[16];
} ndp_prefix_t;

#endif

/** @}
 */
