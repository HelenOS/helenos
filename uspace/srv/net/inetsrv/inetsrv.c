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

/** @addtogroup inet
 * @{
 */
/**
 * @file
 * @brief Internet Protocol service
 */

#include <adt/list.h>
#include <async.h>
#include <errno.h>
#include <str_error.h>
#include <fibril_synch.h>
#include <io/log.h>
#include <ipc/inet.h>
#include <ipc/services.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <task.h>
#include "addrobj.h"
#include "icmp.h"
#include "icmp_std.h"
#include "icmpv6.h"
#include "icmpv6_std.h"
#include "inetsrv.h"
#include "inetcfg.h"
#include "inetping.h"
#include "inet_link.h"
#include "reass.h"
#include "sroute.h"

#define NAME "inetsrv"

static inet_naddr_t solicited_node_mask = {
	.version = ip_v6,
	.addr6 = { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01, 0xff, 0, 0, 0 },
	.prefix = 104
};

static inet_addr_t broadcast4_all_hosts = {
	.version = ip_v4,
	.addr = 0xffffffff
};

static inet_addr_t multicast_all_nodes = {
	.version = ip_v6,
	.addr6 = { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01 }
};

static const char *inet_cfg_path = "/w/cfg/inetsrv.sif";

static FIBRIL_MUTEX_INITIALIZE(client_list_lock);
static LIST_INITIALIZE(client_list);
inet_cfg_t *cfg;

static void inet_default_conn(ipc_call_t *, void *);

static errno_t inet_init(void)
{
	port_id_t port;
	errno_t rc;
	loc_srv_t *srv;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_init()");

	rc = inet_link_discovery_start();
	if (rc != EOK)
		return rc;

	rc = inet_cfg_open(inet_cfg_path, &cfg);
	if (rc != EOK)
		return rc;

	rc = async_create_port(INTERFACE_INET,
	    inet_default_conn, NULL, &port);
	if (rc != EOK)
		return rc;

	rc = async_create_port(INTERFACE_INETCFG,
	    inet_cfg_conn, NULL, &port);
	if (rc != EOK)
		return rc;

	rc = async_create_port(INTERFACE_INETPING,
	    inetping_conn, NULL, &port);
	if (rc != EOK)
		return rc;

	rc = loc_server_register(NAME, &srv);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering server: %s.", str_error(rc));
		return EEXIST;
	}

	service_id_t sid;
	rc = loc_service_register(srv, SERVICE_NAME_INET, &sid);
	if (rc != EOK) {
		loc_server_unregister(srv);
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering service: %s.", str_error(rc));
		return EEXIST;
	}

	return EOK;
}

static void inet_callback_create_srv(inet_client_t *client, ipc_call_t *call)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_callback_create_srv()");

	async_sess_t *sess = async_callback_receive(EXCHANGE_SERIALIZE);
	if (sess == NULL) {
		async_answer_0(call, ENOMEM);
		return;
	}

	client->sess = sess;
	async_answer_0(call, EOK);
}

static errno_t inet_find_dir(inet_addr_t *src, inet_addr_t *dest, uint8_t tos,
    inet_dir_t *dir)
{
	inet_sroute_t *sr;

	/* XXX Handle case where source address is specified */
	(void) src;

	dir->aobj = inet_addrobj_find(dest, iaf_net);
	if (dir->aobj != NULL) {
		dir->ldest = *dest;
		dir->dtype = dt_direct;
	} else {
		/* No direct path, try using a static route */
		sr = inet_sroute_find(dest);
		if (sr != NULL) {
			dir->aobj = inet_addrobj_find(&sr->router, iaf_net);
			dir->ldest = sr->router;
			dir->dtype = dt_router;
		}
	}

	if (dir->aobj == NULL) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_send: No route to destination.");
		return ENOENT;
	}

	return EOK;
}

errno_t inet_route_packet(inet_dgram_t *dgram, uint8_t proto, uint8_t ttl,
    int df)
{
	inet_dir_t dir;
	inet_link_t *ilink;
	errno_t rc;

	if (dgram->iplink != 0) {
		/* XXX TODO - IPv6 */
		log_msg(LOG_DEFAULT, LVL_DEBUG, "dgram directly to iplink %zu",
		    dgram->iplink);
		/* Send packet directly to the specified IP link */
		ilink = inet_link_get_by_id(dgram->iplink);
		if (ilink == 0)
			return ENOENT;

		if (dgram->src.version != ip_v4 ||
		    dgram->dest.version != ip_v4)
			return EINVAL;

		return inet_link_send_dgram(ilink, dgram->src.addr,
		    dgram->dest.addr, dgram, proto, ttl, df);
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "dgram to be routed");

	/* Route packet using source/destination addresses */

	rc = inet_find_dir(&dgram->src, &dgram->dest, dgram->tos, &dir);
	if (rc != EOK)
		return rc;

	return inet_addrobj_send_dgram(dir.aobj, &dir.ldest, dgram,
	    proto, ttl, df);
}

static errno_t inet_send(inet_client_t *client, inet_dgram_t *dgram,
    uint8_t proto, uint8_t ttl, int df)
{
	return inet_route_packet(dgram, proto, ttl, df);
}

errno_t inet_get_srcaddr(inet_addr_t *remote, uint8_t tos, inet_addr_t *local)
{
	inet_dir_t dir;
	errno_t rc;

	rc = inet_find_dir(NULL, remote, tos, &dir);
	if (rc != EOK)
		return rc;

	/* XXX dt_local? */

	/* Take source address from the address object */
	if (remote->version == ip_v4 && remote->addr == 0xffffffff) {
		/* XXX TODO - IPv6 */
		local->version = ip_v4;
		local->addr = 0;
		return EOK;
	}

	inet_naddr_addr(&dir.aobj->naddr, local);
	return EOK;
}

static void inet_get_srcaddr_srv(inet_client_t *client, ipc_call_t *icall)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_get_srcaddr_srv()");

	uint8_t tos = ipc_get_arg1(icall);

	ipc_call_t call;
	size_t size;
	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(inet_addr_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	inet_addr_t remote;
	errno_t rc = async_data_write_finalize(&call, &remote, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
	}

	inet_addr_t local;
	rc = inet_get_srcaddr(&remote, tos, &local);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(inet_addr_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_read_finalize(&call, &local, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	async_answer_0(icall, rc);
}

static void inet_send_srv(inet_client_t *client, ipc_call_t *icall)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_send_srv()");

	inet_dgram_t dgram;

	dgram.iplink = ipc_get_arg1(icall);
	dgram.tos = ipc_get_arg2(icall);

	uint8_t ttl = ipc_get_arg3(icall);
	int df = ipc_get_arg4(icall);

	ipc_call_t call;
	size_t size;
	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(inet_addr_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	errno_t rc = async_data_write_finalize(&call, &dgram.src, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
	}

	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(inet_addr_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_write_finalize(&call, &dgram.dest, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
	}

	rc = async_data_write_accept(&dgram.data, false, 0, 0, 0,
	    &dgram.size);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	rc = inet_send(client, &dgram, client->protocol, ttl, df);

	free(dgram.data);
	async_answer_0(icall, rc);
}

static void inet_set_proto_srv(inet_client_t *client, ipc_call_t *call)
{
	sysarg_t proto;

	proto = ipc_get_arg1(call);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_set_proto_srv(%lu)", (unsigned long) proto);

	if (proto > UINT8_MAX) {
		async_answer_0(call, EINVAL);
		return;
	}

	client->protocol = proto;
	async_answer_0(call, EOK);
}

static void inet_client_init(inet_client_t *client)
{
	client->sess = NULL;

	fibril_mutex_lock(&client_list_lock);
	list_append(&client->client_list, &client_list);
	fibril_mutex_unlock(&client_list_lock);
}

static void inet_client_fini(inet_client_t *client)
{
	async_hangup(client->sess);
	client->sess = NULL;

	fibril_mutex_lock(&client_list_lock);
	list_remove(&client->client_list);
	fibril_mutex_unlock(&client_list_lock);
}

static void inet_default_conn(ipc_call_t *icall, void *arg)
{
	inet_client_t client;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_default_conn()");

	/* Accept the connection */
	async_accept_0(icall);

	inet_client_init(&client);

	while (true) {
		ipc_call_t call;
		async_get_call(&call);
		sysarg_t method = ipc_get_imethod(&call);

		if (!method) {
			/* The other side has hung up */
			async_answer_0(&call, EOK);
			return;
		}

		switch (method) {
		case INET_CALLBACK_CREATE:
			inet_callback_create_srv(&client, &call);
			break;
		case INET_GET_SRCADDR:
			inet_get_srcaddr_srv(&client, &call);
			break;
		case INET_SEND:
			inet_send_srv(&client, &call);
			break;
		case INET_SET_PROTO:
			inet_set_proto_srv(&client, &call);
			break;
		default:
			async_answer_0(&call, EINVAL);
		}
	}

	inet_client_fini(&client);
}

static inet_client_t *inet_client_find(uint8_t proto)
{
	fibril_mutex_lock(&client_list_lock);

	list_foreach(client_list, client_list, inet_client_t, client) {
		if (client->protocol == proto) {
			fibril_mutex_unlock(&client_list_lock);
			return client;
		}
	}

	fibril_mutex_unlock(&client_list_lock);
	return NULL;
}

errno_t inet_ev_recv(inet_client_t *client, inet_dgram_t *dgram)
{
	async_exch_t *exch = async_exchange_begin(client->sess);

	ipc_call_t answer;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_ev_recv: iplink=%zu",
	    dgram->iplink);

	aid_t req = async_send_2(exch, INET_EV_RECV, dgram->tos,
	    dgram->iplink, &answer);

	errno_t rc = async_data_write_start(exch, &dgram->src, sizeof(inet_addr_t));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}

	rc = async_data_write_start(exch, &dgram->dest, sizeof(inet_addr_t));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}

	rc = async_data_write_start(exch, dgram->data, dgram->size);

	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);

	return retval;
}

errno_t inet_recv_dgram_local(inet_dgram_t *dgram, uint8_t proto)
{
	inet_client_t *client;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_recv_dgram_local()");

	/* ICMP and ICMPv6 messages are handled internally */
	if (proto == IP_PROTO_ICMP)
		return icmp_recv(dgram);

	if (proto == IP_PROTO_ICMPV6)
		return icmpv6_recv(dgram);

	client = inet_client_find(proto);
	if (client == NULL) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "No client found for protocol 0x%" PRIx8,
		    proto);
		return ENOENT;
	}

	return inet_ev_recv(client, dgram);
}

errno_t inet_recv_packet(inet_packet_t *packet)
{
	inet_addrobj_t *addr;
	inet_dgram_t dgram;

	addr = inet_addrobj_find(&packet->dest, iaf_addr);
	if ((addr != NULL) ||
	    (inet_naddr_compare_mask(&solicited_node_mask, &packet->dest)) ||
	    (inet_addr_compare(&multicast_all_nodes, &packet->dest)) ||
	    (inet_addr_compare(&broadcast4_all_hosts, &packet->dest))) {
		/* Destined for one of the local addresses */

		/* Check if packet is a complete datagram */
		if (packet->offs == 0 && !packet->mf) {
			/* It is complete deliver it immediately */
			dgram.iplink = packet->link_id;
			dgram.src = packet->src;
			dgram.dest = packet->dest;
			dgram.tos = packet->tos;
			dgram.data = packet->data;
			dgram.size = packet->size;

			return inet_recv_dgram_local(&dgram, packet->proto);
		} else {
			/* It is a fragment, queue it for reassembly */
			inet_reass_queue_packet(packet);
		}
	}

	return ENOENT;
}

int main(int argc, char *argv[])
{
	errno_t rc;

	printf(NAME ": HelenOS Internet Protocol service\n");

	if (log_init(NAME) != EOK) {
		printf(NAME ": Failed to initialize logging.\n");
		return 1;
	}

	rc = inet_init();
	if (rc != EOK)
		return 1;

	printf(NAME ": Accepting connections.\n");

	task_retval(0);

	(void)inet_link_autoconf();
	async_manager();

	/* Not reached */
	return 0;
}

/** @}
 */
