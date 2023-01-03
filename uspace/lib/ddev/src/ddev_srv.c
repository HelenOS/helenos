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

/** @addtogroup libddev
 * @{
 */
/**
 * @file
 * @brief Display protocol server stub
 */

#include <ddev_srv.h>
#include <errno.h>
#include <io/log.h>
#include <ipc/ddev.h>
#include <mem.h>
#include <stdlib.h>
#include <stddef.h>

/** Connect to a GC.
 *
 * XXX As a workaround here we tell the client the values of arg2 and arg3
 * needed to connect to the GC using async_connect_me_to(), these need
 * to be provided by the ddev_ops_t.get_gc. Different values are needed
 * for a DDF driver or a regular server. This would not be needed if we
 * had a proper way of creating an endpoint and passing it to our client.
 */
static void ddev_get_gc_srv(ddev_srv_t *srv, ipc_call_t *icall)
{
	sysarg_t arg2;
	sysarg_t arg3;
	errno_t rc;

	if (srv->ops->get_gc == NULL) {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->get_gc(srv->arg, &arg2, &arg3);
	async_answer_2(icall, rc, arg2, arg3);
}

/** Get display device information */
static void ddev_get_info_srv(ddev_srv_t *srv, ipc_call_t *icall)
{
	ddev_info_t info;
	errno_t rc;

	ipc_call_t call;
	size_t size;
	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(ddev_info_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	if (srv->ops->get_info == NULL) {
		async_answer_0(&call, ENOTSUP);
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = srv->ops->get_info(srv->arg, &info);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	rc = async_data_read_finalize(&call, &info, sizeof(ddev_info_t));
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	async_answer_0(icall, EOK);
}

void ddev_conn(ipc_call_t *icall, ddev_srv_t *srv)
{
	/* Accept the connection */
	async_accept_0(icall);

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
		case DDEV_GET_GC:
			ddev_get_gc_srv(srv, &call);
			break;
		case DDEV_GET_INFO:
			ddev_get_info_srv(srv, &call);
			break;
		default:
			async_answer_0(&call, ENOTSUP);
		}
	}

	/* Hang up callback session */
	if (srv->client_sess != NULL) {
		async_hangup(srv->client_sess);
		srv->client_sess = NULL;
	}
}

/** Initialize display device server structure
 *
 * @param srv Display device server structure to initialize
 */
void ddev_srv_initialize(ddev_srv_t *srv)
{
	memset(srv, 0, sizeof(*srv));
}

/** @}
 */
