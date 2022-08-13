/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

typedef struct {
	link_t addr_list;
	sysarg_t id;
	inet_naddr_t naddr;
	inet_link_t *ilink;
	char *name;
} inet_addrobj_t;

/** Static route configuration */
typedef struct {
	link_t sroute_list;
	sysarg_t id;
	/** Destination network */
	inet_naddr_t dest;
	/** Router via which to route packets */
	inet_addr_t router;
	char *name;
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

extern errno_t inet_ev_recv(inet_client_t *, inet_dgram_t *);
extern errno_t inet_recv_packet(inet_packet_t *);
extern errno_t inet_route_packet(inet_dgram_t *, uint8_t, uint8_t, int);
extern errno_t inet_get_srcaddr(inet_addr_t *, uint8_t, inet_addr_t *);
extern errno_t inet_recv_dgram_local(inet_dgram_t *, uint8_t);

#endif

/** @}
 */
