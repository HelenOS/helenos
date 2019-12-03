/*
 * Copyright (c) 2019 Jiri Svoboda
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

#include <stdio.h>

void ddev_conn(ipc_call_t *icall, ddev_srv_t *srv)
{
	/* Accept the connection */
	async_accept_0(icall);
	printf("display_conn\n");

	while (true) {
		ipc_call_t call;

		async_get_call(&call);
		sysarg_t method = ipc_get_imethod(&call);

		if (!method) {
			/* The other side has hung up */
			async_answer_0(&call, EOK);
			break;
		}

		printf("display_conn method=%u\n", (unsigned) method);
		switch (method) {
		case DDEV_GET_GC:
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
