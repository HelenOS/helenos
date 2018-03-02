/*
 * Copyright (c) 2012 Jiri Svoboda
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
#include <inet/iplink_srv.h>
#include <inet/addr.h>
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
	addr48_t mac_addr;

	/**
	 * List of IP addresses configured on this link
	 * (of the type ethip_link_addr_t)
	 */
	list_t addr_list;
} ethip_nic_t;

/** Ethernet frame */
typedef struct {
	/** Destination Address */
	addr48_t dest;
	/** Source Address */
	addr48_t src;
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
	addr48_t sender_hw_addr;
	/** Sender protocol address */
	addr32_t sender_proto_addr;
	/** Target hardware address */
	addr48_t target_hw_addr;
	/** Target protocol address */
	addr32_t target_proto_addr;
} arp_eth_packet_t;

/** Address translation table element */
typedef struct {
	link_t atrans_list;
	addr32_t ip_addr;
	addr48_t mac_addr;
} ethip_atrans_t;

extern errno_t ethip_iplink_init(ethip_nic_t *);
extern errno_t ethip_received(iplink_srv_t *, void *, size_t);

#endif

/** @}
 */
