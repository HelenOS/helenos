/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
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

#ifndef ETHIP_H_
#define ETHIP_H_

#include <adt/list.h>
#include <async.h>
#include <inet/addr.h>
#include <inet/eth_addr.h>
#include <inet/iplink_srv.h>
#include <loc.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
	link_t link;
	inet_addr_t addr;
} ethip_link_addr_t;

typedef struct ethip_nic {
	link_t link;
	service_id_t svc_id;
	char *svc_name;
	async_sess_t *sess;

	iplink_srv_t iplink;
	service_id_t iplink_sid;

	/** MAC address */
	eth_addr_t mac_addr;

	/**
	 * List of IP addresses configured on this link
	 * (of the type ethip_link_addr_t)
	 */
	list_t addr_list;
} ethip_nic_t;

/** Ethernet frame */
typedef struct {
	/** Destination Address */
	eth_addr_t dest;
	/** Source Address */
	eth_addr_t src;
	/** Ethertype or Length */
	uint16_t etype_len;
	/** Payload */
	void *data;
	/** Payload size */
	size_t size;
} eth_frame_t;

/** ARP opcode */
typedef enum {
	/** Request */
	aop_request,
	/** Reply */
	aop_reply
} arp_opcode_t;

/** ARP packet (for 48-bit MAC addresses and IPv4)
 *
 * Internal representation
 */
typedef struct {
	/** Opcode */
	arp_opcode_t opcode;
	/** Sender hardware address */
	eth_addr_t sender_hw_addr;
	/** Sender protocol address */
	addr32_t sender_proto_addr;
	/** Target hardware address */
	eth_addr_t target_hw_addr;
	/** Target protocol address */
	addr32_t target_proto_addr;
} arp_eth_packet_t;

/** Address translation table element */
typedef struct {
	link_t atrans_list;
	addr32_t ip_addr;
	eth_addr_t mac_addr;
} ethip_atrans_t;

extern errno_t ethip_iplink_init(ethip_nic_t *);
extern errno_t ethip_received(iplink_srv_t *, void *, size_t);

#endif

/** @}
 */
