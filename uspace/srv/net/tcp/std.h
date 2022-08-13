/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup tcp
 * @{
 */
/** @file TCP header definitions
 *
 * Based on IETF RFC 793
 */

#ifndef STD_H
#define STD_H

#include <stdint.h>
#include <inet/addr.h>

#define IP_PROTO_TCP  6

/** TCP Header (fixed part) */
typedef struct {
	/** Source port */
	uint16_t src_port;
	/** Destination port */
	uint16_t dest_port;
	/** Sequence number */
	uint32_t seq;
	/** Acknowledgement number */
	uint32_t ack;
	/** Data Offset, Reserved, Flags */
	uint16_t doff_flags;
	/** Window */
	uint16_t window;
	/* Checksum */
	uint16_t checksum;
	/** Urgent pointer */
	uint16_t urg_ptr;
} tcp_header_t;

/** Bits in tcp_header_t.doff_flags */
enum doff_flags_bits {
	DF_DATA_OFFSET_h	= 15,
	DF_DATA_OFFSET_l	= 12,
	DF_URG			= 5,
	DF_ACK			= 4,
	DF_PSH			= 3,
	DF_RST			= 2,
	DF_SYN			= 1,
	DF_FIN			= 0
};

/** TCP over IPv4 checksum pseudo header */
typedef struct {
	/** Source address */
	uint32_t src;
	/** Destination address */
	uint32_t dest;
	/** Zero */
	uint8_t zero;
	/** Protocol */
	uint8_t protocol;
	/** TCP length */
	uint16_t tcp_length;
} tcp_phdr_t;

/** TCP over IPv6 checksum pseudo header */
typedef struct {
	/** Source address */
	addr128_t src;
	/** Destination address */
	addr128_t dest;
	/** TCP length */
	uint32_t tcp_length;
	/** Zeroes */
	uint8_t zeroes[3];
	/** Next header */
	uint8_t next;
} tcp_phdr6_t;

/** Option kind */
enum opt_kind {
	/** End of option list */
	OPT_END_LIST		= 0,
	/** No-operation */
	OPT_NOP			= 1,
	/** Maximum segment size */
	OPT_MAX_SEG_SIZE	= 2
};

#endif

/** @}
 */
