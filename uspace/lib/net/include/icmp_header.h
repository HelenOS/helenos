/*
 * Copyright (c) 2009 Lukas Mejdrech
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

/** @addtogroup libnet
 *  @{
 */

/** @file
 * ICMP header definition.
 * Based on the RFC 792.
 */

#ifndef LIBNET_ICMP_HEADER_H_
#define LIBNET_ICMP_HEADER_H_

#include <sys/types.h>

#include <net/in.h>
#include <net/icmp_codes.h>

/** ICMP header size in bytes. */
#define ICMP_HEADER_SIZE  sizeof(icmp_header_t)

/** Echo specific data. */
typedef struct icmp_echo {
	/** Message idintifier. */
	icmp_param_t identifier;
	/** Message sequence number. */
	icmp_param_t sequence_number;
} __attribute__((packed)) icmp_echo_t;

/** Internet control message header. */
typedef struct icmp_header {
	/** The type of the message. */
	uint8_t type;
	
	/**
	 * The error code for the datagram reported by the ICMP message.
	 * The interpretation is dependent on the message type.
	 */
	uint8_t code;
	
	/**
	 * The checksum is the 16-bit ones's complement of the one's complement
	 * sum of the ICMP message starting with the ICMP Type. For computing
	 * the checksum, the checksum field should be zero. If the checksum does
	 * not match the contents, the datagram is discarded.
	 */
	uint16_t checksum;
	
	/** Message specific data. */
	union {
		/** Echo specific data. */
		icmp_echo_t echo;
		/** Proposed gateway value. */
		in_addr_t gateway;
		
		/** Fragmentation needed specific data. */
		struct {
			/** Reserved field. Must be zero. */
			icmp_param_t reserved;
			/** Proposed MTU. */
			icmp_param_t mtu;
		} frag;
		
		/** Parameter problem specific data. */
		struct {
			/** Problem pointer. */
			icmp_param_t pointer;
			/** Reserved field. Must be zero. */
			icmp_param_t reserved;
		} param;
	} un;
} __attribute__((packed)) icmp_header_t;

#endif

/** @}
 */
