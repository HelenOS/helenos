/*
 * SPDX-FileCopyrightText: 2012 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup inet
 * @{
 */
/**
 * @file ICMP standard definitions
 *
 */

#ifndef ICMP_STD_H_
#define ICMP_STD_H_

#include <stdint.h>

#define IP_PROTO_ICMP 1

/** Type of service used for ICMP */
#define ICMP_TOS  0

/** ICMP message type */
enum icmp_type {
	ICMP_ECHO_REPLY   = 0,
	ICMP_ECHO_REQUEST = 8
};

/** ICMP Echo Request or Reply message header */
typedef struct {
	/** ICMP message type */
	uint8_t type;
	/** Code (0) */
	uint8_t code;
	/** Internet checksum of the ICMP message */
	uint16_t checksum;
	/** Indentifier */
	uint16_t ident;
	/** Sequence number */
	uint16_t seq_no;
} icmp_echo_t;

#endif

/** @}
 */
