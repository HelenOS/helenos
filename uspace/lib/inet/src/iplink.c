/*
 * Copyright (c) 2021 Jiri Svoboda
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

/** @addtogroup libinet
 * @{
 */
/**
 * @file
 * @brief IP link client stub
 */

#include <async.h>
#include <assert.h>
#include <errno.h>
#include <inet/addr.h>
#include <inet/eth_addr.h>
#include <inet/iplink.h>
#include <ipc/iplink.h>
#include <ipc/services.h>
#include <loc.h>
#include <stdlib.h>

static void iplink_cb_conn(ipc_call_t *icall, void *arg);

errno_t iplink_open(async_sess_t *sess, iplink_ev_ops_t *ev_ops, void *arg,
    iplink_t **riplink)
{
	iplink_t *iplink = calloc(1, sizeof(iplink_t));
	if (iplink == NULL)
		return ENOMEM;

	iplink->sess = sess;
	iplink->ev_ops = ev_ops;
	iplink->arg = arg;

	async_exch_t *exch = async_exchange_begin(sess);

	port_id_t port;
	errno_t rc = async_create_callback_port(exch, INTERFACE_IPLINK_CB, 0, 0,
	    iplink_cb_conn, iplink, &port);

	async_exchange_end(exch);

	if (rc != EOK)
		goto error;

	*riplink = iplink;
	return EOK;

error:
	if (iplink != NULL)
		free(iplink);

	return rc;
}

void iplink_close(iplink_t *iplink)
{
	/* XXX Synchronize with iplink_cb_conn */
	free(iplink);
}

errno_t iplink_send(iplink_t *iplink, iplink_sdu_t *sdu)
{
	async_exch_t *exch = async_exchange_begin(iplink->sess);

	ipc_call_t answer;
	aid_t req = async_send_2(exch, IPLINK_SEND, (sysarg_t) sdu->src,
	    (sysarg_t) sdu->dest, &answer);

	errno_t rc = async_data_write_start(exch, sdu->data, sdu->size);

	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);

	return retval;
}

errno_t iplink_send6(iplink_t *iplink, iplink_sdu6_t *sdu)
{
	async_exch_t *exch = async_exchange_begin(iplink->sess);

	ipc_call_t answer;
	aid_t req = async_send_0(exch, IPLINK_SEND6, &answer);

	errno_t rc = async_data_write_start(exch, &sdu->dest, sizeof(eth_addr_t));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}

	rc = async_data_write_start(exch, sdu->data, sdu->size);

	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);

	return retval;
}

errno_t iplink_get_mtu(iplink_t *iplink, size_t *rmtu)
{
	async_exch_t *exch = async_exchange_begin(iplink->sess);

	sysarg_t mtu;
	errno_t rc = async_req_0_1(exch, IPLINK_GET_MTU, &mtu);

	async_exchange_end(exch);

	if (rc != EOK)
		return rc;

	*rmtu = mtu;
	return EOK;
}

errno_t iplink_get_mac48(iplink_t *iplink, eth_addr_t *mac)
{
	async_exch_t *exch = async_exchange_begin(iplink->sess);

	ipc_call_t answer;
	aid_t req = async_send_0(exch, IPLINK_GET_MAC48, &answer);

	errno_t rc = async_data_read_start(exch, mac, sizeof(eth_addr_t));

	loc_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);

	return retval;
}

errno_t iplink_set_mac48(iplink_t *iplink, eth_addr_t *mac)
{
	async_exch_t *exch = async_exchange_begin(iplink->sess);

	ipc_call_t answer;
	aid_t req = async_send_0(exch, IPLINK_GET_MAC48, &answer);

	errno_t rc = async_data_read_start(exch, mac, sizeof(eth_addr_t));

	loc_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);

	return retval;
}

errno_t iplink_addr_add(iplink_t *iplink, inet_addr_t *addr)
{
	async_exch_t *exch = async_exchange_begin(iplink->sess);

	ipc_call_t answer;
	aid_t req = async_send_0(exch, IPLINK_ADDR_ADD, &answer);

	errno_t rc = async_data_write_start(exch, addr, sizeof(inet_addr_t));
	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);

	return retval;
}

errno_t iplink_addr_remove(iplink_t *iplink, inet_addr_t *addr)
{
	async_exch_t *exch = async_exchange_begin(iplink->sess);

	ipc_call_t answer;
	aid_t req = async_send_0(exch, IPLINK_ADDR_REMOVE, &answer);

	errno_t rc = async_data_write_start(exch, addr, sizeof(inet_addr_t));
	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);

	return retval;
}

void *iplink_get_userptr(iplink_t *iplink)
{
	return iplink->arg;
}

static void iplink_ev_recv(iplink_t *iplink, ipc_call_t *icall)
{
	iplink_recv_sdu_t sdu;

	ip_ver_t ver = ipc_get_arg1(icall);

	errno_t rc = async_data_write_accept(&sdu.data, false, 0, 0, 0,
	    &sdu.size);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	rc = iplink->ev_ops->recv(iplink, &sdu, ver);
	free(sdu.data);
	async_answer_0(icall, rc);
}

static void iplink_ev_change_addr(iplink_t *iplink, ipc_call_t *icall)
{
	eth_addr_t *addr;
	size_t size;

	errno_t rc = async_data_write_accept((void **) &addr, false,
	    sizeof(eth_addr_t), sizeof(eth_addr_t), 0, &size);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	rc = iplink->ev_ops->change_addr(iplink, addr);
	free(addr);
	async_answer_0(icall, EOK);
}

static void iplink_cb_conn(ipc_call_t *icall, void *arg)
{
	iplink_t *iplink = (iplink_t *) arg;

	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			async_answer_0(&call, EOK);
			return;
		}

		switch (ipc_get_imethod(&call)) {
		case IPLINK_EV_RECV:
			iplink_ev_recv(iplink, &call);
			break;
		case IPLINK_EV_CHANGE_ADDR:
			iplink_ev_change_addr(iplink, &call);
			break;
		default:
			async_answer_0(&call, ENOTSUP);
		}
	}
}

/** @}
 */
