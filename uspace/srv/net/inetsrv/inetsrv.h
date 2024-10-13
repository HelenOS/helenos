/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup inetsrv
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef INETSRV_H_
#define INETSRV_H_

#include <adt/list.h>
#include <stdbool.h>
#include <inet/addr.h>
#include <inet/eth_addr.h>
#include <inet/iplink.h>
#include <ipc/loc.h>
#include <sif.h>
#include <stddef.h>
#include <stdint.h>
#include <types/inet.h>
#include <async.h>

/** Inet Client */
typedef struct {
	async_sess_t *sess;
	uint8_t protocol;
	link_t client_list;
} inet_client_t;

/** Inetping Client */
typedef struct {
	/** Callback session */
	async_sess_t *sess;
	/** Session identifier */
	uint16_t ident;
	/** Link to client list */
	link_t client_list;
} inetping_client_t;

/** Inetping6 Client */
typedef struct {
	/** Callback session */
	async_sess_t *sess;
	/** Session identifier */
	uint16_t ident;
	/** Link to client list */
	link_t client_list;
} inetping6_client_t;

typedef struct {
	/** Local link ID */
	service_id_t link_id;
	/** Source address */
	inet_addr_t src;
	/** Destination address */
	inet_addr_t dest;
	/** Type of service */
	uint8_t tos;
	/** Protocol */
	uint8_t proto;
	/** Time to live */
	uint8_t ttl;
	/** Identifier */
	uint32_t ident;
	/** Do not fragment */
	bool df;
	/** More fragments */
	bool mf;
	/** Offset of fragment into datagram, in bytes */
	size_t offs;
	/** Packet data */
	void *data;
	/** Packet data size in bytes */
	size_t size;
} inet_packet_t;

typedef struct {
	link_t link_list;
	service_id_t svc_id;
	char *svc_name;
	async_sess_t *sess;
	iplink_t *iplink;
	size_t def_mtu;
	eth_addr_t mac;
	bool mac_valid;
} inet_link_t;

/** Link information needed for autoconfiguration */
typedef struct {
	service_id_t svc_id;
	char *svc_name;
} inet_link_cfg_info_t;

/** Address object */
typedef struct {
	/** Link to list of addresses */
	link_t addr_list;
	/** Address object ID */
	sysarg_t id;
	/** Network address */
	inet_naddr_t naddr;
	/** Underlying IP link */
	inet_link_t *ilink;
	/** Address name */
	char *name;
	/** Temporary object */
	bool temp;
} inet_addrobj_t;

/** Static route configuration */
typedef struct {
	link_t sroute_list;
	/** ID */
	sysarg_t id;
	/** Destination network */
	inet_naddr_t dest;
	/** Router via which to route packets */
	inet_addr_t router;
	/** Route name */
	char *name;
	/** Temporary route */
	bool temp;
} inet_sroute_t;

typedef enum {
	/** Destination is on this network node */
	dt_local,
	/** Destination is directly reachable */
	dt_direct,
	/** Destination is behind a router */
	dt_router
} inet_dir_type_t;

/** Direction (next hop) to a destination */
typedef struct {
	/** Route type */
	inet_dir_type_t dtype;
	/** Address object (direction) */
	inet_addrobj_t *aobj;
	/** Local destination address */
	inet_addr_t ldest;
} inet_dir_t;

/** Internet server configuration */
typedef struct {
	/** Configuration file path */
	char *cfg_path;
} inet_cfg_t;

extern inet_cfg_t *cfg;

extern errno_t inet_ev_recv(inet_client_t *, inet_dgram_t *);
extern errno_t inet_recv_packet(inet_packet_t *);
extern errno_t inet_route_packet(inet_dgram_t *, uint8_t, uint8_t, int);
extern errno_t inet_get_srcaddr(inet_addr_t *, uint8_t, inet_addr_t *);
extern errno_t inet_recv_dgram_local(inet_dgram_t *, uint8_t);

#endif

/** @}
 */
