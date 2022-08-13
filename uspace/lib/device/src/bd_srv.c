/*
 * SPDX-FileCopyrightText: 2012 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
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
#include <stddef.h>
#include <stdint.h>

#include <bd_srv.h>

static void bd_read_blocks_srv(bd_srv_t *srv, ipc_call_t *call)
{
	aoff64_t ba;
	size_t cnt;
	void *buf;
	size_t size;
	errno_t rc;

	ba = MERGE_LOUP32(ipc_get_arg1(call), ipc_get_arg2(call));
	cnt = ipc_get_arg3(call);

	ipc_call_t rcall;
	if (!async_data_read_receive(&rcall, &size)) {
		async_answer_0(call, EINVAL);
		return;
	}

	buf = malloc(size);
	if (buf == NULL) {
		async_answer_0(&rcall, ENOMEM);
		async_answer_0(call, ENOMEM);
		return;
	}

	if (srv->srvs->ops->read_blocks == NULL) {
		async_answer_0(&rcall, ENOTSUP);
		async_answer_0(call, ENOTSUP);
		free(buf);
		return;
	}

	rc = srv->srvs->ops->read_blocks(srv, ba, cnt, buf, size);
	if (rc != EOK) {
		async_answer_0(&rcall, ENOMEM);
		async_answer_0(call, ENOMEM);
		free(buf);
		return;
	}

	async_data_read_finalize(&rcall, buf, size);

	free(buf);
	async_answer_0(call, EOK);
}

static void bd_read_toc_srv(bd_srv_t *srv, ipc_call_t *call)
{
	uint8_t session;
	void *buf;
	size_t size;
	errno_t rc;

	session = ipc_get_arg1(call);

	ipc_call_t rcall;
	if (!async_data_read_receive(&rcall, &size)) {
		async_answer_0(call, EINVAL);
		return;
	}

	buf = malloc(size);
	if (buf == NULL) {
		async_answer_0(&rcall, ENOMEM);
		async_answer_0(call, ENOMEM);
		return;
	}

	if (srv->srvs->ops->read_toc == NULL) {
		async_answer_0(&rcall, ENOTSUP);
		async_answer_0(call, ENOTSUP);
		free(buf);
		return;
	}

	rc = srv->srvs->ops->read_toc(srv, session, buf, size);
	if (rc != EOK) {
		async_answer_0(&rcall, ENOMEM);
		async_answer_0(call, ENOMEM);
		free(buf);
		return;
	}

	async_data_read_finalize(&rcall, buf, size);

	free(buf);
	async_answer_0(call, EOK);
}

static void bd_sync_cache_srv(bd_srv_t *srv, ipc_call_t *call)
{
	aoff64_t ba;
	size_t cnt;
	errno_t rc;

	ba = MERGE_LOUP32(ipc_get_arg1(call), ipc_get_arg2(call));
	cnt = ipc_get_arg3(call);

	if (srv->srvs->ops->sync_cache == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	rc = srv->srvs->ops->sync_cache(srv, ba, cnt);
	async_answer_0(call, rc);
}

static void bd_write_blocks_srv(bd_srv_t *srv, ipc_call_t *call)
{
	aoff64_t ba;
	size_t cnt;
	void *data;
	size_t size;
	errno_t rc;

	ba = MERGE_LOUP32(ipc_get_arg1(call), ipc_get_arg2(call));
	cnt = ipc_get_arg3(call);

	rc = async_data_write_accept(&data, false, 0, 0, 0, &size);
	if (rc != EOK) {
		async_answer_0(call, rc);
		return;
	}

	if (srv->srvs->ops->write_blocks == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	rc = srv->srvs->ops->write_blocks(srv, ba, cnt, data, size);
	free(data);
	async_answer_0(call, rc);
}

static void bd_get_block_size_srv(bd_srv_t *srv, ipc_call_t *call)
{
	errno_t rc;
	size_t block_size;

	if (srv->srvs->ops->get_block_size == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	rc = srv->srvs->ops->get_block_size(srv, &block_size);
	async_answer_1(call, rc, block_size);
}

static void bd_get_num_blocks_srv(bd_srv_t *srv, ipc_call_t *call)
{
	errno_t rc;
	aoff64_t num_blocks;

	if (srv->srvs->ops->get_num_blocks == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	rc = srv->srvs->ops->get_num_blocks(srv, &num_blocks);
	async_answer_2(call, rc, LOWER32(num_blocks), UPPER32(num_blocks));
}

static bd_srv_t *bd_srv_create(bd_srvs_t *srvs)
{
	bd_srv_t *srv;

	srv = calloc(1, sizeof(bd_srv_t));
	if (srv == NULL)
		return NULL;

	srv->srvs = srvs;
	return srv;
}

void bd_srvs_init(bd_srvs_t *srvs)
{
	srvs->ops = NULL;
	srvs->sarg = NULL;
}

errno_t bd_conn(ipc_call_t *icall, bd_srvs_t *srvs)
{
	bd_srv_t *srv;
	errno_t rc;

	/* Accept the connection */
	async_accept_0(icall);

	srv = bd_srv_create(srvs);
	if (srv == NULL)
		return ENOMEM;

	async_sess_t *sess = async_callback_receive(EXCHANGE_SERIALIZE);
	if (sess == NULL)
		return ENOMEM;

	srv->client_sess = sess;

	rc = srvs->ops->open(srvs, srv);
	if (rc != EOK)
		return rc;

	while (true) {
		ipc_call_t call;
		async_get_call(&call);
		sysarg_t method = ipc_get_imethod(&call);

		if (!method) {
			/* The other side has hung up */
			async_answer_0(&call, EOK);
			break;
		}

		switch (method) {
		case BD_READ_BLOCKS:
			bd_read_blocks_srv(srv, &call);
			break;
		case BD_READ_TOC:
			bd_read_toc_srv(srv, &call);
			break;
		case BD_SYNC_CACHE:
			bd_sync_cache_srv(srv, &call);
			break;
		case BD_WRITE_BLOCKS:
			bd_write_blocks_srv(srv, &call);
			break;
		case BD_GET_BLOCK_SIZE:
			bd_get_block_size_srv(srv, &call);
			break;
		case BD_GET_NUM_BLOCKS:
			bd_get_num_blocks_srv(srv, &call);
			break;
		default:
			async_answer_0(&call, EINVAL);
		}
	}

	rc = srvs->ops->close(srv);
	free(srv);

	return rc;
}

/** @}
 */
