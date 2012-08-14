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

/** @addtogroup libc
 * @{
 */
/**
 * @file
 * @brief Block device server stub
 */
#include <errno.h>
#include <ipc/bd.h>
#include <macros.h>
#include <stdlib.h>
#include <sys/types.h>

#include <bd_srv.h>

static void bd_read_blocks_srv(bd_srv_t *srv, ipc_callid_t callid,
    ipc_call_t *call)
{
	aoff64_t ba;
	size_t cnt;
	void *buf;
	size_t size;
	int rc;
	ipc_callid_t rcallid;

	ba = MERGE_LOUP32(IPC_GET_ARG1(*call), IPC_GET_ARG2(*call));
	cnt = IPC_GET_ARG3(*call);

	if (!async_data_read_receive(&rcallid, &size)) {
		async_answer_0(callid, EINVAL);
		return;
	}

	buf = malloc(size);
	if (buf == NULL) {
		async_answer_0(rcallid, ENOMEM);
		async_answer_0(callid, ENOMEM);
		return;
	}

	if (srv->ops->read_blocks == NULL) {
		async_answer_0(rcallid, ENOTSUP);
		async_answer_0(callid, ENOTSUP);
		return;
	}

	rc = srv->ops->read_blocks(srv, ba, cnt, buf, size);
	if (rc != EOK) {
		async_answer_0(rcallid, ENOMEM);
		async_answer_0(callid, ENOMEM);
		return;
	}

	async_data_read_finalize(rcallid, buf, size);

	free(buf);
	async_answer_0(callid, EOK);
}

static void bd_read_toc_srv(bd_srv_t *srv, ipc_callid_t callid,
    ipc_call_t *call)
{
	uint8_t session;
	void *buf;
	size_t size;
	int rc;
	ipc_callid_t rcallid;

	session = IPC_GET_ARG1(*call);

	if (!async_data_read_receive(&rcallid, &size)) {
		async_answer_0(callid, EINVAL);
		return;
	}

	buf = malloc(size);
	if (buf == NULL) {
		async_answer_0(rcallid, ENOMEM);
		async_answer_0(callid, ENOMEM);
		return;
	}

	if (srv->ops->read_toc == NULL) {
		async_answer_0(rcallid, ENOTSUP);
		async_answer_0(callid, ENOTSUP);
		return;
	}

	rc = srv->ops->read_toc(srv, session, buf, size);
	if (rc != EOK) {
		async_answer_0(rcallid, ENOMEM);
		async_answer_0(callid, ENOMEM);
		return;
	}

	async_data_read_finalize(rcallid, buf, size);

	free(buf);
	async_answer_0(callid, EOK);
}

static void bd_write_blocks_srv(bd_srv_t *srv, ipc_callid_t callid,
    ipc_call_t *call)
{
	aoff64_t ba;
	size_t cnt;
	void *data;
	size_t size;
	int rc;

	ba = MERGE_LOUP32(IPC_GET_ARG1(*call), IPC_GET_ARG2(*call));
	cnt = IPC_GET_ARG3(*call);

	rc = async_data_write_accept(&data, false, 0, 0, 0, &size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}

	if (srv->ops->write_blocks == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	rc = srv->ops->write_blocks(srv, ba, cnt, data, size);
	free(data);
	async_answer_0(callid, rc);
}

static void bd_get_block_size_srv(bd_srv_t *srv, ipc_callid_t callid,
    ipc_call_t *call)
{
	int rc;
	size_t block_size;

	if (srv->ops->get_block_size == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	rc = srv->ops->get_block_size(srv, &block_size);
	async_answer_1(callid, rc, block_size);
}

static void bd_get_num_blocks_srv(bd_srv_t *srv, ipc_callid_t callid,
    ipc_call_t *call)
{
	int rc;
	aoff64_t num_blocks;

	if (srv->ops->get_num_blocks == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	rc = srv->ops->get_num_blocks(srv, &num_blocks);
	async_answer_2(callid, rc, LOWER32(num_blocks), UPPER32(num_blocks));
}

void bd_srv_init(bd_srv_t *srv)
{
	fibril_mutex_initialize(&srv->lock);
	srv->connected = false;
	srv->ops = NULL;
	srv->arg = NULL;
	srv->client_sess = NULL;
}

int bd_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	bd_srv_t *srv = (bd_srv_t *)arg;
	int rc;

	fibril_mutex_lock(&srv->lock);
	if (srv->connected) {
		fibril_mutex_unlock(&srv->lock);
		async_answer_0(iid, EBUSY);
		return EBUSY;
	}

	srv->connected = true;
	fibril_mutex_unlock(&srv->lock);

	/* Accept the connection */
	async_answer_0(iid, EOK);

	async_sess_t *sess = async_callback_receive(EXCHANGE_SERIALIZE);
	if (sess == NULL)
		return ENOMEM;

	srv->client_sess = sess;

	rc = srv->ops->open(srv);
	if (rc != EOK)
		return rc;

	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		sysarg_t method = IPC_GET_IMETHOD(call);

		if (!method) {
			/* The other side has hung up */
			fibril_mutex_lock(&srv->lock);
			srv->connected = false;
			fibril_mutex_unlock(&srv->lock);
			async_answer_0(callid, EOK);
			break;
		}

		switch (method) {
		case BD_READ_BLOCKS:
			bd_read_blocks_srv(srv, callid, &call);
			break;
		case BD_READ_TOC:
			bd_read_toc_srv(srv, callid, &call);
			break;
		case BD_WRITE_BLOCKS:
			bd_write_blocks_srv(srv, callid, &call);
			break;
		case BD_GET_BLOCK_SIZE:
			bd_get_block_size_srv(srv, callid, &call);
			break;
		case BD_GET_NUM_BLOCKS:
			bd_get_num_blocks_srv(srv, callid, &call);
			break;
		default:
			async_answer_0(callid, EINVAL);
		}
	}

	return srv->ops->close(srv);
}

/** @}
 */
