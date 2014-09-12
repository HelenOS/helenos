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
 * @brief Internet Protocol service
 */

#include <adt/list.h>
#include <async.h>
#include <errno.h>
#include <fibril_synch.h>
#include <io/log.h>
#include <ipc/inet.h>
#include <ipc/services.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
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
	.addr6 = {0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01, 0xff, 0, 0, 0},
	.prefix = 104
};

static inet_addr_t broadcast4_all_hosts = {
	.version = ip_v4,
	.addr = 0xffffffff
};

static inet_addr_t multicast_all_nodes = {
	.version = ip_v6,
	.addr6 = {0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01}
};

static void inet_client_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg);

static FIBRIL_MUTEX_INITIALIZE(client_list_lock);
static LIST_INITIALIZE(client_list);

static int inet_init(void)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_init()");
	
	async_set_client_connection(inet_client_conn);
	
	int rc = loc_server_register(NAME);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering server (%d).", rc);
		return EEXIST;
	}
	
	service_id_t sid;
	rc = loc_service_register_with_iface(SERVICE_NAME_INET, &sid,
	    INET_PORT_DEFAULT);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering service (%d).", rc);
		return EEXIST;
	}
	
	rc = loc_service_register_with_iface(SERVICE_NAME_INETCFG, &sid,
	    INET_PORT_CFG);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering service (%d).", rc);
		return EEXIST;
	}
	
	rc = loc_service_register_with_iface(SERVICE_NAME_INETPING, &sid,
	    INET_PORT_PING);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering service (%d).", rc);
		return EEXIST;
	}
	
	return EOK;
}

static void inet_callback_create_srv(inet_client_t *client, ipc_callid_t callid,
    ipc_call_t *call)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_callback_create_srv()");

	async_sess_t *sess = async_callback_receive(EXCHANGE_SERIALIZE);
	if (sess == NULL) {
		async_answer_0(callid, ENOMEM);
		return;
	}

	client->sess = sess;
	async_answer_0(callid, EOK);
}

static int inet_find_dir(inet_addr_t *src, inet_addr_t *dest, uint8_t tos,
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

int inet_route_packet(inet_dgram_t *dgram, uint8_t proto, uint8_t ttl,
    int df)
{
	inet_dir_t dir;
	inet_link_t *ilink;
	int rc;

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

static int inet_send(inet_client_t *client, inet_dgram_t *dgram,
    uint8_t proto, uint8_t ttl, int df)
{
	return inet_route_packet(dgram, proto, ttl, df);
}

int inet_get_srcaddr(inet_addr_t *remote, uint8_t tos, inet_addr_t *local)
{
	inet_dir_t dir;
	int rc;

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

static void inet_get_srcaddr_srv(inet_client_t *client, ipc_callid_t iid,
    ipc_call_t *icall)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_get_srcaddr_srv()");
	
	uint8_t tos = IPC_GET_ARG1(*icall);
	
	ipc_callid_t callid;
	size_t size;
	if (!async_data_write_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}
	
	if (size != sizeof(inet_addr_t)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	inet_addr_t remote;
	int rc = async_data_write_finalize(callid, &remote, size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
	}
	
	inet_addr_t local;
	rc = inet_get_srcaddr(&remote, tos, &local);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}
	
	if (!async_data_read_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}
	
	if (size != sizeof(inet_addr_t)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	rc = async_data_read_finalize(callid, &local, size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		return;
	}
	
	async_answer_0(iid, rc);
}

static void inet_send_srv(inet_client_t *client, ipc_callid_t iid,
    ipc_call_t *icall)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_send_srv()");
	
	inet_dgram_t dgram;
	
	dgram.iplink = IPC_GET_ARG1(*icall);
	dgram.tos = IPC_GET_ARG2(*icall);
	
	uint8_t ttl = IPC_GET_ARG3(*icall);
	int df = IPC_GET_ARG4(*icall);
	
	ipc_callid_t callid;
	size_t size;
	if (!async_data_write_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}
	
	if (size != sizeof(inet_addr_t)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	int rc = async_data_write_finalize(callid, &dgram.src, size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
	}
	
	if (!async_data_write_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}
	
	if (size != sizeof(inet_addr_t)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	rc = async_data_write_finalize(callid, &dgram.dest, size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
	}
	
	rc = async_data_write_accept(&dgram.data, false, 0, 0, 0,
	    &dgram.size);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}
	
	rc = inet_send(client, &dgram, client->protocol, ttl, df);
	
	free(dgram.data);
	async_answer_0(iid, rc);
}

static void inet_set_proto_srv(inet_client_t *client, ipc_callid_t callid,
    ipc_call_t *call)
{
	sysarg_t proto;

	proto = IPC_GET_ARG1(*call);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_set_proto_srv(%lu)", (unsigned long) proto);

	if (proto > UINT8_MAX) {
		async_answer_0(callid, EINVAL);
		return;
	}

	client->protocol = proto;
	async_answer_0(callid, EOK);
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

static void inet_default_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	inet_client_t client;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_default_conn()");

	/* Accept the connection */
	async_answer_0(iid, EOK);

	inet_client_init(&client);

	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		sysarg_t method = IPC_GET_IMETHOD(call);

		if (!method) {
			/* The other side has hung up */
			async_answer_0(callid, EOK);
			return;
		}

		switch (method) {
		case INET_CALLBACK_CREATE:
			inet_callback_create_srv(&client, callid, &call);
			break;
		case INET_GET_SRCADDR:
			inet_get_srcaddr_srv(&client, callid, &call);
			break;
		case INET_SEND:
			inet_send_srv(&client, callid, &call);
			break;
		case INET_SET_PROTO:
			inet_set_proto_srv(&client, callid, &call);
			break;
		default:
			async_answer_0(callid, EINVAL);
		}
	}

	inet_client_fini(&client);
}

static void inet_client_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	sysarg_t port;

	port = IPC_GET_ARG1(*icall);

	switch (port) {
	case INET_PORT_DEFAULT:
		inet_default_conn(iid, icall, arg);
		break;
	case INET_PORT_CFG:
		inet_cfg_conn(iid, icall, arg);
		break;
	case INET_PORT_PING:
		inetping_conn(iid, icall, arg);
		break;
	default:
		async_answer_0(iid, ENOTSUP);
		break;
	}
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

int inet_ev_recv(inet_client_t *client, inet_dgram_t *dgram)
{
	async_exch_t *exch = async_exchange_begin(client->sess);
	
	ipc_call_t answer;
	aid_t req = async_send_1(exch, INET_EV_RECV, dgram->tos, &answer);
	
	int rc = async_data_write_start(exch, &dgram->src, sizeof(inet_addr_t));
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
	
	sysarg_t retval;
	async_wait_for(req, &retval);
	
	return (int) retval;
}

int inet_recv_dgram_local(inet_dgram_t *dgram, uint8_t proto)
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

int inet_recv_packet(inet_packet_t *packet)
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
	int rc;

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
	async_manager();

	/* Not reached */
	return 0;
}

/** @}
 */
