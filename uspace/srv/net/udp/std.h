/*
 * SPDX-FileCopyrightText: 2012 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
