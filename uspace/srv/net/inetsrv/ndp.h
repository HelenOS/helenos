/*
 * Copyright (c) 2013 Antonin Steinhauser
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

#ifndef NDP_H_
#define NDP_H_

#include <inet/addr.h>
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
	addr48_t sender_hw_addr;
	/** Sender protocol address */
	addr128_t sender_proto_addr;
	/** Target hardware address */
	addr48_t target_hw_addr;
	/** Target protocol address */
	addr128_t target_proto_addr;
	/** Solicited IPv6 address */
	addr128_t solicited_ip;
} ndp_packet_t;

extern errno_t ndp_received(inet_dgram_t *);
extern errno_t ndp_translate(addr128_t, addr128_t, addr48_t, inet_link_t *);

#endif
