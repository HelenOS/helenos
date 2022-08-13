/*
 * SPDX-FileCopyrightText: 2012 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup inet
 * @{
 */
/**
 * @file IP header definitions
 *
 */

#ifndef INET_STD_H_
#define INET_STD_H_

#include <stdint.h>

#define IP6_NEXT_FRAGMENT  44

/** IPv4 Datagram header (fixed part) */
typedef struct {
	/** Version, Internet Header Length */
	uint8_t ver_ihl;
	/* Type of Service */
	uint8_t tos;
	/** Total Length */
	uint16_t tot_len;
	/** Identifier */
	uint16_t id;
	/** Flags, Fragment Offset */
	uint16_t flags_foff;
	/** Time to Live */
	uint8_t ttl;
	/** Protocol */
	uint8_t proto;
	/** Header Checksum */
	uint16_t chksum;
	/** Source Address */
	uint32_t src_addr;
	/** Destination Address */
	uint32_t dest_addr;
} ip_header_t;

/** Bits in ip_header_t.ver_ihl */
enum ver_ihl_bits {
	/** Version, highest bit */
	VI_VERSION_h = 7,
	/** Version, lowest bit */
	VI_VERSION_l = 4,
	/** Internet Header Length, highest bit */
	VI_IHL_h     = 3,
	/** Internet Header Length, lowest bit */
	VI_IHL_l     = 0
};

/** Bits in ip_header_t.flags_foff */
enum flags_foff_bits {
	/** Reserved, must be zero */
	FF_FLAG_RSVD = 15,
	/** Don't Fragment */
	FF_FLAG_DF = 14,
	/** More Fragments */
	FF_FLAG_MF = 13,
	/** Fragment Offset, highest bit */
	FF_FRAGOFF_h = 12,
	/** Fragment Offset, lowest bit */
	FF_FRAGOFF_l = 0
};

/** Bits in ip6_header_fragment_t.offsmf */
enum flags_offsmt_bits {
	/** More fragments */
	OF_FLAG_M = 0,
	/** Fragment offset, highest bit */
	OF_FRAGOFF_h = 15,
	/** Fragment offset, lowest bit */
	OF_FRAGOFF_l = 3
};

/** IPv6 Datagram header (fixed part) */
typedef struct {
	/** Version, Traffic class first 4 bits */
	uint8_t ver_tc;
	/** Traffic class (the rest), Flow label */
	uint8_t tc_fl[3];
	/* Payload length */
	uint16_t payload_len;
	/** Next header */
	uint8_t next;
	/** Hop limit */
	uint8_t hop_limit;
	/** Source address */
	uint8_t src_addr[16];
	/** Destination address */
	uint8_t dest_addr[16];
} ip6_header_t;

/** IPv6 Datagram Fragment extension header */
typedef struct {
	/** Next header */
	uint8_t next;
	/** Reserved */
	uint8_t reserved;
	/** Fragmentation offset, reserved and M flag */
	uint16_t offsmf;
	/** Identifier */
	uint32_t id;
} ip6_header_fragment_t;

/** Fragment offset is expressed in units of 8 bytes */
#define FRAG_OFFS_UNIT 8

#endif

/** @}
 */
