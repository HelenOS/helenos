/*
 * SPDX-FileCopyrightText: 2014 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
 * @{
 */
/**
 * @file
 * @brief Character device server stub
 */
#include <errno.h>
#include <io/chardev_srv.h>
#include <ipc/chardev.h>
#include <macros.h>
#include <stdlib.h>
#include <stddef.h>

static chardev_srv_t *chardev_srv_create(chardev_srvs_t *);

static void chardev_read_srv(chardev_srv_t *srv, ipc_call_t *icall)
{
	void *buf;
	size_t size;
	size_t nread;
	chardev_flags_t flags;
	errno_t rc;

	flags = ipc_get_arg1(icall);

	ipc_call_t call;
	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(icall, EINVAL);
		return;
	}

	buf = malloc(size);
	if (buf == NULL) {
		async_answer_0(&call, ENOMEM);
		async_answer_0(icall, ENOMEM);
		return;
	}

	if (srv->srvs->ops->read == NULL) {
		async_answer_0(&call, ENOTSUP);
		async_answer_0(icall, ENOTSUP);
		free(buf);
		return;
	}

	rc = srv->srvs->ops->read(srv, buf, size, &nread, flags);
	if (rc != EOK && nread == 0) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		free(buf);
		return;
	}

	async_data_read_finalize(&call, buf, nread);

	free(buf);
	async_answer_2(icall, EOK, (sysarg_t) rc, nread);
}

static void chardev_write_srv(chardev_srv_t *srv, ipc_call_t *icall)
{
	void *data;
	size_t size;
	size_t nwr;
	errno_t rc;

	rc = async_data_write_accept(&data, false, 0, 0, 0, &size);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	if (srv->srvs->ops->write == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->srvs->ops->write(srv, data, size, &nwr);
	free(data);
	if (rc != EOK && nwr == 0) {
		async_answer_0(icall, rc);
		return;
	}

	async_answer_2(icall, EOK, (sysarg_t) rc, nwr);
}

static chardev_srv_t *chardev_srv_create(chardev_srvs_t *srvs)
{
	chardev_srv_t *srv;

	srv = calloc(1, sizeof(chardev_srv_t));
	if (srv == NULL)
		return NULL;

	srv->srvs = srvs;
	return srv;
}

void chardev_srvs_init(chardev_srvs_t *srvs)
{
	srvs->ops = NULL;
	srvs->sarg = NULL;
}

errno_t chardev_conn(ipc_call_t *icall, chardev_srvs_t *srvs)
{
	chardev_srv_t *srv;
	errno_t rc;

	/* Accept the connection */
	async_accept_0(icall);

	srv = chardev_srv_create(srvs);
	if (srv == NULL)
		return ENOMEM;

	if (srvs->ops->open != NULL) {
		rc = srvs->ops->open(srvs, srv);
		if (rc != EOK)
			return rc;
	}

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
		case CHARDEV_READ:
			chardev_read_srv(srv, &call);
			break;
		case CHARDEV_WRITE:
			chardev_write_srv(srv, &call);
			break;
		default:
			if (srv->srvs->ops->def_handler != NULL)
				srv->srvs->ops->def_handler(srv, &call);
			else
				async_answer_0(&call, ENOTSUP);
		}
	}

	if (srvs->ops->close != NULL)
		rc = srvs->ops->close(srv);
	else
		rc = EOK;

	free(srv);

	return rc;
}

/** @}
 */
