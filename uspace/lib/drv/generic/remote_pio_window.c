/*
 * SPDX-FileCopyrightText: 2013 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

static void remote_pio_window_get(ddf_fun_t *, void *, ipc_call_t *);

static const remote_iface_func_ptr_t remote_pio_window_iface_ops [] = {
	[PIO_WINDOW_GET] = &remote_pio_window_get
};

const remote_iface_t remote_pio_window_iface = {
	.method_count = ARRAY_SIZE(remote_pio_window_iface_ops),
	.methods = remote_pio_window_iface_ops
};

static void remote_pio_window_get(ddf_fun_t *fun, void *ops,
    ipc_call_t *call)
{
	pio_window_ops_t *pio_win_ops = (pio_window_ops_t *) ops;

	if (!pio_win_ops->get_pio_window) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	pio_window_t *pio_window = pio_win_ops->get_pio_window(fun);
	if (!pio_window) {
		async_answer_0(call, ENOENT);
		return;
	}

	async_answer_0(call, EOK);

	ipc_call_t data;
	size_t len;
	if (!async_data_read_receive(&data, &len)) {
		/* Protocol error - the recipient is not accepting data */
		return;
	}

	async_data_read_finalize(&data, pio_window, len);
}

/**
 * @}
 */
