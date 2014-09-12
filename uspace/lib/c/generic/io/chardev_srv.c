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
#include <sys/types.h>

static chardev_srv_t *chardev_srv_create(chardev_srvs_t *);

static void chardev_read_srv(chardev_srv_t *srv, ipc_callid_t callid,
    ipc_call_t *call)
{
	size_t size = IPC_GET_ARG1(*call);
	int rc;

	if (srv->srvs->ops->read == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	if (size <= 4 * sizeof(sysarg_t)) {
		sysarg_t message[4] = {};

		rc = srv->srvs->ops->read(srv, (char *)message, size);
		async_answer_4(callid, rc, message[0], message[1],
		    message[2], message[3]);
	} else {
		async_answer_0(callid, ELIMIT);
	}
}

static void chardev_write_srv(chardev_srv_t *srv, ipc_callid_t callid,
    ipc_call_t *call)
{
	size_t size = IPC_GET_ARG1(*call);
	int rc;

	if (srv->srvs->ops->write == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	if (size <= 3 * sizeof(sysarg_t)) {
		const sysarg_t message[3] = {
			IPC_GET_ARG2(*call),
			IPC_GET_ARG3(*call),
			IPC_GET_ARG4(*call)
		};

		rc = srv->srvs->ops->write(srv, (char *)message, size);
		async_answer_0(callid, rc);
	} else {
		async_answer_0(callid, ELIMIT);
	}
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

int chardev_conn(ipc_callid_t iid, ipc_call_t *icall, chardev_srvs_t *srvs)
{
	chardev_srv_t *srv;
	int rc;

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
			async_answer_0(callid, EINVAL);
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
