/*
 * Copyright (c) 2022 Jiri Svoboda
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

/** @addtogroup dhcp
 * @{
 */
/**
 * @file
 * @brief DHCP standard definitions
 */

#ifndef DHCP_STD_H
#define DHCP_STD_H

#include <stdint.h>

enum {
	dhcp_server_port = 67,
	dhcp_client_port = 68
};

/** Fixed part of DHCP message */
typedef struct {
	/** Message op code */
	uint8_t op;
	/** Hardware address type */
	uint8_t htype;
	/** Hardware address length */
	uint8_t hlen;
	/** Hops */
	uint8_t hops;
	/** Transaction ID */
	uint32_t xid;
	/** Seconds elapsed since client began address acqusition or renewal */
	uint16_t secs;
	/** Flags */
	uint16_t flags;
	/** Client IP address */
	uint32_t ciaddr;
	/** Your (client) IP address */
	uint32_t yiaddr;
	/** IP address of next server */
	uint32_t siaddr;
	/** Relay agent IP address */
	uint32_t giaddr;
	/** Client hardware address */
	uint8_t chaddr[16];
	/** Server host name */
	uint8_t sname[64];
	/** Boot file name */
	uint8_t file[128];
	/** Magic cookie signalling the start of DHCP options */
	uint32_t opt_magic;
} dhcp_hdr_t;

/** Values for dhcp_hdr_t.op */
enum dhcp_op {
	op_bootrequest = 1,
	op_bootreply = 2
};

/** Values for dhcp_hdr_t.flags */
enum dhcp_flags {
	flag_broadcast = 0x8000
};

/** Magic cookie signalling the start of DHCP options field */
enum {
	dhcp_opt_magic = (99 << 24) | (130 << 16) | (83 << 8) | 99
};

enum dhcp_option_code {
	/** Padding */
	opt_pad = 0,
	/** Subnet mask */
	opt_subnet_mask = 1,
	/** Router IP address */
	opt_router = 3,
	/** Domain name server */
	opt_dns_server = 6,
	/** Requested IP address */
	opt_req_ip_addr = 50,
	/** DHCP message type */
	opt_msg_type = 53,
	/** Server identifier */
	opt_server_id = 54,
	/** Parameter request list */
	opt_param_req_list = 55,
	/** End */
	opt_end = 255
};

/** DHCP message type */
enum dhcp_msg_type {
	msg_dhcpdiscover = 1,
	msg_dhcpoffer = 2,
	msg_dhcprequest = 3,
	msg_dhcpdecline = 4,
	msg_dhcpack = 5,
	msg_dhcpnak = 6,
	msg_dhcprelease = 7
};

#endif

/** @}
 */
