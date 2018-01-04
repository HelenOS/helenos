/*
 * Copyright (c) 2014 Jiri Svoboda
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
 * @brief Character device server stub
 */
#include <errno.h>
#include <io/chardev_srv.h>
#include <ipc/chardev.h>
#include <macros.h>
#include <stdlib.h>
#include <stddef.h>

static chardev_srv_t *chardev_srv_create(chardev_srvs_t *);

static void chardev_read_srv(chardev_srv_t *srv, ipc_callid_t callid,
    ipc_call_t *call)
{
	void *buf;
	size_t size;
	size_t nread;
	errno_t rc;
	ipc_callid_t rcallid;

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

	if (srv->srvs->ops->read == NULL) {
		async_answer_0(rcallid, ENOTSUP);
		async_answer_0(callid, ENOTSUP);
		free(buf);
		return;
	}

	rc = srv->srvs->ops->read(srv, buf, size, &nread);
	if (rc != EOK && nread == 0) {
		async_answer_0(rcallid, rc);
		async_answer_0(callid, rc);
		free(buf);
		return;
	}

	async_data_read_finalize(rcallid, buf, nread);

	free(buf);
	async_answer_2(callid, EOK, (sysarg_t) rc, nread);
}

static void chardev_write_srv(chardev_srv_t *srv, ipc_callid_t callid,
    ipc_call_t *call)
{
	void *data;
	size_t size;
	size_t nwr;
	errno_t rc;

	rc = async_data_write_accept(&data, false, 0, 0, 0, &size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}

	if (srv->srvs->ops->write == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	rc = srv->srvs->ops->write(srv, data, size, &nwr);
	free(data);
	if (rc != EOK && nwr == 0) {
		async_answer_0(callid, rc);
		return;
	}

	async_answer_2(callid, EOK, (sysarg_t) rc, nwr);
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

errno_t chardev_conn(ipc_callid_t iid, ipc_call_t *icall, chardev_srvs_t *srvs)
{
	chardev_srv_t *srv;
	errno_t rc;

	/* Accept the connection */
	async_answer_0(iid, EOK);

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
		ipc_callid_t callid = async_get_call(&call);
		sysarg_t method = IPC_GET_IMETHOD(call);

		if (!method) {
			/* The other side has hung up */
			async_answer_0(callid, EOK);
			break;
		}

		switch (method) {
		case CHARDEV_READ:
			chardev_read_srv(srv, callid, &call);
			break;
		case CHARDEV_WRITE:
			chardev_write_srv(srv, callid, &call);
			break;
		default:
			if (srv->srvs->ops->def_handler != NULL)
				srv->srvs->ops->def_handler(srv, callid, &call);
			else
				async_answer_0(callid, ENOTSUP);
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
