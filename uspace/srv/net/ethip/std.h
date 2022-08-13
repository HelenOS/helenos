/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup ethip
 * @{
 */
/**
 * @file Ethernet, IP/Ethernet standard definitions
 *
 */

#ifndef ETHIP_STD_H_
#define ETHIP_STD_H_

#include <stdint.h>

#define ETH_ADDR_SIZE       6
#define IPV4_ADDR_SIZE      4
#define ETH_FRAME_MIN_SIZE  60

/** Ethernet frame header */
typedef struct {
	/** Destination Address */
	uint8_t dest[ETH_ADDR_SIZE];
	/** Source Address */
	uint8_t src[ETH_ADDR_SIZE];
	/** Ethertype or Length */
	uint16_t etype_len;
} eth_header_t;

/** ARP packet format (for 48-bit MAC addresses and IPv4) */
typedef struct {
	/** Hardware address space */
	uint16_t hw_addr_space;
	/** Protocol address space */
	uint16_t proto_addr_space;
	/** Hardware address size */
	uint8_t hw_addr_size;
	/** Protocol address size */
	uint8_t proto_addr_size;
	/** Opcode */
	uint16_t opcode;
	/** Sender hardware address */
	uint8_t sender_hw_addr[ETH_ADDR_SIZE];
	/** Sender protocol address */
	uint32_t sender_proto_addr;
	/** Target hardware address */
	uint8_t target_hw_addr[ETH_ADDR_SIZE];
	/** Target protocol address */
	uint32_t target_proto_addr;
} __attribute__((packed)) arp_eth_packet_fmt_t;

enum arp_opcode_fmt {
	AOP_REQUEST = 1,
	AOP_REPLY   = 2
};

enum arp_hw_addr_space {
	AHRD_ETHERNET = 1
};

/** IP Ethertype */
enum ether_type {
	ETYPE_ARP  = 0x0806,
	ETYPE_IP   = 0x0800,
	ETYPE_IPV6 = 0x86DD
};

#endif

/** @}
 */
