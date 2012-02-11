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

/** @addtogroup ethip
 * @{
 */
/**
 * @file
 * @brief IP link provider for Ethernet
 *
 * Based on the IETF RFC 894 standard.
 */

#include <async.h>
#include <errno.h>
#include <inet/iplink_srv.h>
#include <io/log.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>

#include "ethip.h"
#include "ethip_nic.h"
#include "pdu.h"
#include "std.h"

#define NAME "eth"

static int ethip_open(iplink_conn_t *conn);
static int ethip_close(iplink_conn_t *conn);
static int ethip_send(iplink_conn_t *conn, iplink_srv_sdu_t *sdu);
static int ethip_get_mtu(iplink_conn_t *conn, size_t *mtu);

static void ethip_client_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg);

static iplink_ops_t ethip_iplink_ops = {
	.open = ethip_open,
	.close = ethip_close,
	.send = ethip_send,
	.get_mtu = ethip_get_mtu
};

static int ethip_init(void)
{
	int rc;

	async_set_client_connection(ethip_client_conn);

	rc = loc_server_register(NAME);
	if (rc != EOK) {
		log_msg(LVL_ERROR, "Failed registering server.");
		return rc;
	}

	rc = ethip_nic_discovery_start();
	if (rc != EOK)
		return rc;

	return EOK;
}

int ethip_iplink_init(ethip_nic_t *nic)
{
	int rc;
	service_id_t sid;
	category_id_t iplink_cat;
	static unsigned link_num = 0;
	char *svc_name = NULL;

	log_msg(LVL_DEBUG, "ethip_iplink_init()");

	nic->iplink.ops = &ethip_iplink_ops;
	nic->iplink.arg = nic;

	rc = asprintf(&svc_name, "net/eth%u", ++link_num);
	if (rc < 0) {
		log_msg(LVL_ERROR, "Out of memory.");
		goto error;
	}

	rc = loc_service_register(svc_name, &sid);
	if (rc != EOK) {
		log_msg(LVL_ERROR, "Failed registering service %s.", svc_name);
		goto error;
	}

	nic->iplink_sid = sid;

	rc = loc_category_get_id("iplink", &iplink_cat, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		log_msg(LVL_ERROR, "Failed resolving category 'iplink'.");
		goto error;
	}

	rc = loc_service_add_to_cat(sid, iplink_cat);
	if (rc != EOK) {
		log_msg(LVL_ERROR, "Failed adding %s to category.", svc_name);
		goto error;
	}

	return EOK;

error:
	if (svc_name != NULL)
		free(svc_name);
	return rc;
}

static void ethip_client_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	ethip_nic_t *nic;
	service_id_t sid;

	sid = (service_id_t)IPC_GET_ARG1(*icall);
	log_msg(LVL_DEBUG, "ethip_client_conn(%u)", (unsigned)sid);
	nic = ethip_nic_find_by_iplink_sid(sid);
	if (nic == NULL) {
		log_msg(LVL_WARN, "Uknown service ID.");
		return;
	}

	iplink_conn(iid, icall, &nic->iplink);
}

static int ethip_open(iplink_conn_t *conn)
{
	log_msg(LVL_DEBUG, "ethip_open()");
	return EOK;
}

static int ethip_close(iplink_conn_t *conn)
{
	log_msg(LVL_DEBUG, "ethip_open()");
	return EOK;
}

static int ethip_send(iplink_conn_t *conn, iplink_srv_sdu_t *sdu)
{
	ethip_nic_t *nic = (ethip_nic_t *)conn->srv->arg;
	eth_frame_t frame;
	void *data;
	size_t size;
	int rc;

	log_msg(LVL_DEBUG, "ethip_send()");

	frame.dest.addr = 0xdeeedeeedeee;
	frame.src.addr =  0xaafeedfaceee;
	frame.etype_len = ETYPE_IP;
	frame.data = sdu->data;
	frame.size = sdu->size;

	rc = eth_pdu_encode(&frame, &data, &size);
	if (rc != EOK)
		return rc;

	rc = ethip_nic_send(nic, data, size);
	free(data);

	return rc;
}

int ethip_received(iplink_srv_t *srv, void *data, size_t size)
{
	eth_frame_t frame;
	iplink_srv_sdu_t sdu;
	int rc;

	rc = eth_pdu_decode(data, size, &frame);
	if (rc != EOK)
		return rc;

	sdu.data = frame.data;
	sdu.size = frame.size;
	(void) sdu;
	//rc = iplink_ev_recv(conn, &sdu);

	free(frame.data);
	return rc;
}

static int ethip_get_mtu(iplink_conn_t *conn, size_t *mtu)
{
	log_msg(LVL_DEBUG, "ethip_get_mtu()");
	*mtu = 1500;
	return EOK;
}

int main(int argc, char *argv[])
{
	int rc;

	printf(NAME ": HelenOS IP over Ethernet service\n");

	if (log_init(NAME, LVL_DEBUG) != EOK) {
		printf(NAME ": Failed to initialize logging.\n");
		return 1;
	}

	rc = ethip_init();
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
