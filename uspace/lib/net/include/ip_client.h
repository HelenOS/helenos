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
 * @{
 */

/** @file
 * IP client interface.
 */

#ifndef LIBNET_IP_CLIENT_H_
#define LIBNET_IP_CLIENT_H_

#include <net/socket_codes.h>
#include <sys/types.h>

#include <net/packet.h>
#include <net/ip_codes.h>
#include <ip_interface.h>

extern int ip_client_prepare_packet(packet_t *, ip_protocol_t, ip_ttl_t,
    ip_tos_t, int, size_t);
extern int ip_client_process_packet(packet_t *, ip_protocol_t *, ip_ttl_t *,
    ip_tos_t *, int *, size_t *);
extern size_t ip_client_header_length(packet_t *);
extern int ip_client_set_pseudo_header_data_length(void *, size_t, size_t);
extern int ip_client_get_pseudo_header(ip_protocol_t, struct sockaddr *,
    socklen_t, struct sockaddr *, socklen_t, size_t, void **, size_t *);

// TODO ipopt manipulation

#endif

/** @}
 */
