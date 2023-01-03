/*
 * Copyright (c) 2023 Jiri Svoboda
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
 * @brief IP link server stub
 */

#include <errno.h>
#include <inet/eth_addr.h>
#include <ipc/iplink.h>
#include <stdlib.h>
#include <stddef.h>
#include <inet/addr.h>
#include <inet/iplink_srv.h>

static void iplink_get_mtu_srv(iplink_srv_t *srv, ipc_call_t *call)
{
	size_t mtu;
	errno_t rc = srv->ops->get_mtu(srv, &mtu);
	async_answer_1(call, rc, mtu);
}

static void iplink_get_mac48_srv(iplink_srv_t *srv, ipc_call_t *icall)
{
	eth_addr_t mac;
	errno_t rc = srv->ops->get_mac48(srv, &mac);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	ipc_call_t call;
	size_t size;
	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(eth_addr_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_read_finalize(&call, &mac, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	async_answer_0(icall, rc);
}

static void iplink_set_mac48_srv(iplink_srv_t *srv, ipc_call_t *icall)
{
	errno_t rc;
	size_t size;
	eth_addr_t mac;

	ipc_call_t call;
	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
	}

	rc = srv->ops->set_mac48(srv, &mac);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	rc = async_data_read_finalize(&call, &mac, sizeof(eth_addr_t));
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	async_answer_0(icall, rc);
}

static void iplink_addr_add_srv(iplink_srv_t *srv, ipc_call_t *icall)
{
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

	inet_addr_t addr;
	errno_t rc = async_data_write_finalize(&call, &addr, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	rc = srv->ops->addr_add(srv, &addr);
	async_answer_0(icall, rc);
}

static void iplink_addr_remove_srv(iplink_srv_t *srv, ipc_call_t *icall)
{
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

	inet_addr_t addr;
	errno_t rc = async_data_write_finalize(&call, &addr, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	rc = srv->ops->addr_remove(srv, &addr);
	async_answer_0(icall, rc);
}

static void iplink_send_srv(iplink_srv_t *srv, ipc_call_t *icall)
{
	iplink_sdu_t sdu;

	sdu.src = ipc_get_arg1(icall);
	sdu.dest = ipc_get_arg2(icall);

	errno_t rc = async_data_write_accept(&sdu.data, false, 0, 0, 0,
	    &sdu.size);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	rc = srv->ops->send(srv, &sdu);
	free(sdu.data);
	async_answer_0(icall, rc);
}

static void iplink_send6_srv(iplink_srv_t *srv, ipc_call_t *icall)
{
	iplink_sdu6_t sdu;
	ipc_call_t call;
	size_t size;

	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(eth_addr_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	errno_t rc = async_data_write_finalize(&call, &sdu.dest, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
	}

	rc = async_data_write_accept(&sdu.data, false, 0, 0, 0,
	    &sdu.size);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	rc = srv->ops->send6(srv, &sdu);
	free(sdu.data);
	async_answer_0(icall, rc);
}

void iplink_srv_init(iplink_srv_t *srv)
{
	fibril_mutex_initialize(&srv->lock);
	srv->connected = false;
	srv->ops = NULL;
	srv->arg = NULL;
	srv->client_sess = NULL;
}

errno_t iplink_conn(ipc_call_t *icall, void *arg)
{
	iplink_srv_t *srv = (iplink_srv_t *) arg;
	errno_t rc;

	fibril_mutex_lock(&srv->lock);
	if (srv->connected) {
		fibril_mutex_unlock(&srv->lock);
		async_answer_0(icall, EBUSY);
		return EBUSY;
	}

	srv->connected = true;
	fibril_mutex_unlock(&srv->lock);

	/* Accept the connection */
	async_accept_0(icall);

	async_sess_t *sess = async_callback_receive(EXCHANGE_SERIALIZE);
	if (sess == NULL)
		return ENOMEM;

	srv->client_sess = sess;

	rc = srv->ops->open(srv);
	if (rc != EOK)
		return rc;

	while (true) {
		ipc_call_t call;
		async_get_call(&call);
		sysarg_t method = ipc_get_imethod(&call);

		if (!method) {
			/* The other side has hung up */
			fibril_mutex_lock(&srv->lock);
			srv->connected = false;
			fibril_mutex_unlock(&srv->lock);
			async_answer_0(&call, EOK);
			break;
		}

		switch (method) {
		case IPLINK_GET_MTU:
			iplink_get_mtu_srv(srv, &call);
			break;
		case IPLINK_GET_MAC48:
			iplink_get_mac48_srv(srv, &call);
			break;
		case IPLINK_SET_MAC48:
			iplink_set_mac48_srv(srv, &call);
			break;
		case IPLINK_SEND:
			iplink_send_srv(srv, &call);
			break;
		case IPLINK_SEND6:
			iplink_send6_srv(srv, &call);
			break;
		case IPLINK_ADDR_ADD:
			iplink_addr_add_srv(srv, &call);
			break;
		case IPLINK_ADDR_REMOVE:
			iplink_addr_remove_srv(srv, &call);
			break;
		default:
			async_answer_0(&call, EINVAL);
		}
	}

	return srv->ops->close(srv);
}

/* XXX Version should be part of @a sdu */
errno_t iplink_ev_recv(iplink_srv_t *srv, iplink_recv_sdu_t *sdu, ip_ver_t ver)
{
	if (srv->client_sess == NULL)
		return EIO;

	async_exch_t *exch = async_exchange_begin(srv->client_sess);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, IPLINK_EV_RECV, (sysarg_t)ver,
	    &answer);

	errno_t rc = async_data_write_start(exch, sdu->data, sdu->size);
	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);
	if (retval != EOK)
		return retval;

	return EOK;
}

errno_t iplink_ev_change_addr(iplink_srv_t *srv, eth_addr_t *addr)
{
	if (srv->client_sess == NULL)
		return EIO;

	async_exch_t *exch = async_exchange_begin(srv->client_sess);

	ipc_call_t answer;
	aid_t req = async_send_0(exch, IPLINK_EV_CHANGE_ADDR, &answer);

	errno_t rc = async_data_write_start(exch, addr, sizeof(eth_addr_t));
	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);
	if (retval != EOK)
		return retval;

	return EOK;
}

/** @}
 */
