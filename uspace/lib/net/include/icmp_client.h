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

/** @addtogroup icmp
 *  @{
 */

/** @file
 *  ICMP client interface.
 */

#ifndef __NET_ICMP_CLIENT_H__
#define __NET_ICMP_CLIENT_H__

#include <icmp_codes.h>
#include <packet/packet.h>

/** Processes the received packet prefixed with an ICMP header.
 *  @param[in] packet The received packet.
 *  @param[out] type The ICMP header type.
 *  @param[out] code The ICMP header code.
 *  @param[out] pointer The ICMP header pointer.
 *  @param[out] mtu The ICMP header MTU.
 *  @returns The ICMP header length.
 *  @returns Zero (0) if the packet contains no data.
 */
extern int icmp_client_process_packet(packet_t packet, icmp_type_t * type, icmp_code_t * code, icmp_param_t * pointer, icmp_param_t * mtu);

/** Returns the ICMP header length.
 *  @param[in] packet The packet.
 *  @returns The ICMP header length in bytes.
 */
extern size_t icmp_client_header_length(packet_t packet);

#endif

/** @}
 */
