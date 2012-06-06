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

/** @addtogroup loopip
 * @{
 */
/**
 * @file
 * @brief Loopback IP link provider
 */

#include <adt/prodcons.h>
#include <async.h>
#include <errno.h>
#include <inet/iplink_srv.h>
#include <io/log.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>

#define NAME "loopip"

static int loopip_open(iplink_srv_t *srv);
static int loopip_close(iplink_srv_t *srv);
static int loopip_send(iplink_srv_t *srv, iplink_srv_sdu_t *sdu);
static int loopip_get_mtu(iplink_srv_t *srv, size_t *mtu);
static int loopip_addr_add(iplink_srv_t *srv, iplink_srv_addr_t *addr);
static int loopip_addr_remove(iplink_srv_t *srv, iplink_srv_addr_t *addr);

static void loopip_client_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg);

static iplink_ops_t loopip_iplink_ops = {
	.open = loopip_open,
	.close = loopip_close,
	.send = loopip_send,
	.get_mtu = loopip_get_mtu,
	.addr_add = loopip_addr_add,
	.addr_remove = loopip_addr_remove
};

static iplink_srv_t loopip_iplink;
static prodcons_t loopip_rcv_queue;

typedef struct {
	link_t link;
	iplink_srv_sdu_t sdu;
} rqueue_entry_t;

static int loopip_recv_fibril(void *arg)
{
	while (true) {
		log_msg(LVL_DEBUG, "loopip_recv_fibril(): Wait for one item");
		link_t *link = prodcons_consume(&loopip_rcv_queue);
		rqueue_entry_t *rqe = list_get_instance(link, rqueue_entry_t, link);

		(void) iplink_ev_recv(&loopip_iplink, &rqe->sdu);
	}

	return 0;
}

static int loopip_init(void)
{
	int rc;
	service_id_t sid;
	category_id_t iplink_cat;
	const char *svc_name = "net/loopback";
	
	async_set_client_connection(loopip_client_conn);
	
	rc = loc_server_register(NAME);
	if (rc != EOK) {
		log_msg(LVL_ERROR, "Failed registering server.");
		return rc;
	}

	iplink_srv_init(&loopip_iplink);
	loopip_iplink.ops = &loopip_iplink_ops;
	loopip_iplink.arg = NULL;

	prodcons_initialize(&loopip_rcv_queue);

	rc = loc_service_register(svc_name, &sid);
	if (rc != EOK) {
		log_msg(LVL_ERROR, "Failed registering service %s.", svc_name);
		return rc;
	}

	rc = loc_category_get_id("iplink", &iplink_cat, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		log_msg(LVL_ERROR, "Failed resolving category 'iplink'.");
		return rc;
	}

	rc = loc_service_add_to_cat(sid, iplink_cat);
	if (rc != EOK) {
		log_msg(LVL_ERROR, "Failed adding %s to category.", svc_name);
		return rc;
	}

	fid_t fid = fibril_create(loopip_recv_fibril, NULL);
	if (fid == 0)
		return ENOMEM;

	fibril_add_ready(fid);

	return EOK;
}

static void loopip_client_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	log_msg(LVL_DEBUG, "loopip_client_conn()");
	iplink_conn(iid, icall, &loopip_iplink);
}

static int loopip_open(iplink_srv_t *srv)
{
	log_msg(LVL_DEBUG, "loopip_open()");
	return EOK;
}

static int loopip_close(iplink_srv_t *srv)
{
	log_msg(LVL_DEBUG, "loopip_close()");
	return EOK;
}

static int loopip_send(iplink_srv_t *srv, iplink_srv_sdu_t *sdu)
{
	rqueue_entry_t *rqe;

	log_msg(LVL_DEBUG, "loopip_send()");

	rqe = calloc(1, sizeof(rqueue_entry_t));
	if (rqe == NULL)
		return ENOMEM;
	/*
	 * Clone SDU
	 */
	rqe->sdu.lsrc = sdu->ldest;
	rqe->sdu.ldest = sdu->lsrc;
	rqe->sdu.data = malloc(sdu->size);
	if (rqe->sdu.data == NULL) {
		free(rqe);
		return ENOMEM;
	}

	memcpy(rqe->sdu.data, sdu->data, sdu->size);
	rqe->sdu.size = sdu->size;

	/*
	 * Insert to receive queue
	 */
	prodcons_produce(&loopip_rcv_queue, &rqe->link);

	return EOK;
}

static int loopip_get_mtu(iplink_srv_t *srv, size_t *mtu)
{
	log_msg(LVL_DEBUG, "loopip_get_mtu()");
	*mtu = 1500;
	return EOK;
}

static int loopip_addr_add(iplink_srv_t *srv, iplink_srv_addr_t *addr)
{
	log_msg(LVL_DEBUG, "loopip_addr_add(0x%" PRIx32 ")", addr->ipv4);
	return EOK;
}

static int loopip_addr_remove(iplink_srv_t *srv, iplink_srv_addr_t *addr)
{
	log_msg(LVL_DEBUG, "loopip_addr_remove(0x%" PRIx32 ")", addr->ipv4);
	return EOK;
}

int main(int argc, char *argv[])
{
	int rc;

	printf(NAME ": HelenOS loopback IP link provider\n");

	if (log_init(NAME, LVL_WARN) != EOK) {
		printf(NAME ": Failed to initialize logging.\n");
		return 1;
	}

	rc = loopip_init();
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
