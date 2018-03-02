/*
 * Copyright (c) 2013 Jakub Jermar
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

#include <async.h>
#include <errno.h>
#include <macros.h>

#include "ops/pio_window.h"
#include "ddf/driver.h"

static void remote_pio_window_get(ddf_fun_t *, void *, ipc_callid_t,
    ipc_call_t *);

static const remote_iface_func_ptr_t remote_pio_window_iface_ops [] = {
	[PIO_WINDOW_GET] = &remote_pio_window_get
};

const remote_iface_t remote_pio_window_iface = {
	.method_count = ARRAY_SIZE(remote_pio_window_iface_ops),
	.methods = remote_pio_window_iface_ops
};

static void remote_pio_window_get(ddf_fun_t *fun, void *ops,
    ipc_callid_t callid, ipc_call_t *call)
{
	pio_window_ops_t *pio_win_ops = (pio_window_ops_t *) ops;
	size_t len;

	if (!pio_win_ops->get_pio_window) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	pio_window_t *pio_window = pio_win_ops->get_pio_window(fun);
	if (!pio_window) {
		async_answer_0(callid, ENOENT);
		return;
	}

	async_answer_0(callid, EOK);

	if (!async_data_read_receive(&callid, &len)) {
		/* Protocol error - the recipient is not accepting data */
		return;
	}

	async_data_read_finalize(callid, pio_window, len);
}

/**
 * @}
 */
