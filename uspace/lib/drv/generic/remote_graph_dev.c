/*
 * Copyright (c) 2011 Petr Koupy
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

/** @addtogroup libdrv
 * @{
 */
/** @file
 */

#include <errno.h>
#include <async.h>
#include <macros.h>

#include "ops/graph_dev.h"
#include "graph_iface.h"
#include "ddf/driver.h"

typedef enum {
	GRAPH_DEV_CONNECT = 0
} graph_dev_method_t;

int graph_dev_connect(async_sess_t *sess)
{
	async_exch_t *exch = async_exchange_begin(sess);
	int ret = async_req_1_0(exch, DEV_IFACE_ID(GRAPH_DEV_IFACE), GRAPH_DEV_CONNECT);
	async_exchange_end(exch);

	return ret;
}

static void remote_graph_connect(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);

static const remote_iface_func_ptr_t remote_graph_dev_iface_ops[] = {
	[GRAPH_DEV_CONNECT] = remote_graph_connect
};

const remote_iface_t remote_graph_dev_iface = {
	.method_count = ARRAY_SIZE(remote_graph_dev_iface_ops),
	.methods = remote_graph_dev_iface_ops
};

static void remote_graph_connect(ddf_fun_t *fun, void *ops, ipc_callid_t callid,
    ipc_call_t *call)
{
	graph_dev_ops_t *graph_dev_ops = (graph_dev_ops_t *) ops;

	if (!graph_dev_ops->connect || !ddf_fun_data_get(fun)) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	(*graph_dev_ops->connect)(ddf_fun_data_get(fun), callid, call, NULL);
}

/**
 * @}
 */
