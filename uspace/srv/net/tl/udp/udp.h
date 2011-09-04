/*
 * Copyright (c) 2008 Lukas Mejdrech
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

/** @addtogroup udp
 * @{
 */

/** @file
 * UDP module.
 */

#ifndef NET_UDP_H_
#define NET_UDP_H_

#include <async.h>
#include <fibril_synch.h>
#include <socket_core.h>
#include <tl_common.h>

/** Type definition of the UDP global data.
 * @see udp_globals
 */
typedef struct udp_globals udp_globals_t;

/** UDP global data. */
struct udp_globals {
	/** Networking module session. */
	async_sess_t *net_sess;
	/** IP module session. */
	async_sess_t *ip_sess;
	/** ICMP module session. */
	async_sess_t *icmp_sess;
	/** Packet dimension. */
	packet_dimension_t packet_dimension;
	/** Indicates whether UDP checksum computing is enabled. */
	int checksum_computing;
	/** Indicates whether UDP autobnding on send is enabled. */
	int autobinding;
	/** Last used free port. */
	int last_used_port;
	/** Active sockets. */
	socket_ports_t sockets;
	/** Device packet dimensions. */
	packet_dimensions_t dimensions;
	/** Safety lock. */
	fibril_rwlock_t lock;
};

#endif

/** @}
 */
