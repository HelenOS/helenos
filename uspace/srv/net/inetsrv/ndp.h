/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 * SPDX-FileCopyrightText: 2013 Antonin Steinhauser
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup ethip
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef NDP_H_
#define NDP_H_

#include <inet/addr.h>
#include <inet/eth_addr.h>
#include "inetsrv.h"
#include "icmpv6_std.h"

typedef enum icmpv6_type ndp_opcode_t;

/** NDP packet (for 48-bit MAC addresses)
 *
 * Internal representation
 */
typedef struct {
	/** Opcode */
	ndp_opcode_t opcode;
	/** Sender hardware address */
	eth_addr_t sender_hw_addr;
	/** Sender protocol address */
	addr128_t sender_proto_addr;
	/** Target hardware address */
	eth_addr_t target_hw_addr;
	/** Target protocol address */
	addr128_t target_proto_addr;
	/** Solicited IPv6 address */
	addr128_t solicited_ip;
} ndp_packet_t;

extern errno_t ndp_received(inet_dgram_t *);
extern errno_t ndp_translate(addr128_t, addr128_t, eth_addr_t *, inet_link_t *);

#endif
