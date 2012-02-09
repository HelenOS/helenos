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

/** @addtogroup inet
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef INET_H_
#define INET_H_

#include <adt/list.h>
#include <inet/iplink.h>
#include <ipc/loc.h>
#include <sys/types.h>
#include <async.h>

/** Inet Client */
typedef struct {
	async_sess_t *sess;
	uint8_t protocol;
	link_t client_list;
} inet_client_t;

/** Host address */
typedef struct {
	uint32_t ipv4;
} inet_addr_t;

/** Network address */
typedef struct {
	/** Address */
	uint32_t ipv4;
	/** Number of valid bits in @c ipv4 */
	int bits;
} inet_naddr_t;

typedef struct {
	inet_addr_t src;
	inet_addr_t dest;
	uint8_t tos;
	void *data;
	size_t size;
} inet_dgram_t;

typedef struct {
	link_t link_list;
	service_id_t svc_id;
	char *svc_name;
	async_sess_t *sess;
	iplink_t *iplink;
} inet_link_t;

typedef struct {
	link_t addr_list;
	inet_naddr_t naddr;
	inet_link_t *ilink;
} inet_addrobj_t;

extern int inet_ev_recv(inet_client_t *, inet_dgram_t *);
extern int inet_recv_packet(inet_dgram_t *, uint8_t ttl, int df);

#endif

/** @}
 */
