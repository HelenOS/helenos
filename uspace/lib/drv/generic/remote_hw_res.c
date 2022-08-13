/*
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 * SPDX-FileCopyrightText: 2011 Jan Vesely
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

#include "ops/hw_res.h"
#include "ddf/driver.h"

static void remote_hw_res_get_resource_list(ddf_fun_t *, void *, ipc_call_t *);
static void remote_hw_res_enable_interrupt(ddf_fun_t *, void *, ipc_call_t *);
static void remote_hw_res_disable_interrupt(ddf_fun_t *, void *, ipc_call_t *);
static void remote_hw_res_clear_interrupt(ddf_fun_t *, void *, ipc_call_t *);
static void remote_hw_res_dma_channel_setup(ddf_fun_t *, void *, ipc_call_t *);
static void remote_hw_res_dma_channel_remain(ddf_fun_t *, void *, ipc_call_t *);

static const remote_iface_func_ptr_t remote_hw_res_iface_ops [] = {
	[HW_RES_GET_RESOURCE_LIST] = &remote_hw_res_get_resource_list,
	[HW_RES_ENABLE_INTERRUPT] = &remote_hw_res_enable_interrupt,
	[HW_RES_DISABLE_INTERRUPT] = &remote_hw_res_disable_interrupt,
	[HW_RES_CLEAR_INTERRUPT] = &remote_hw_res_clear_interrupt,
	[HW_RES_DMA_CHANNEL_SETUP] = &remote_hw_res_dma_channel_setup,
	[HW_RES_DMA_CHANNEL_REMAIN] = &remote_hw_res_dma_channel_remain,
};

const remote_iface_t remote_hw_res_iface = {
	.method_count = ARRAY_SIZE(remote_hw_res_iface_ops),
	.methods = remote_hw_res_iface_ops
};

static void remote_hw_res_enable_interrupt(ddf_fun_t *fun, void *ops,
    ipc_call_t *call)
{
	hw_res_ops_t *hw_res_ops = (hw_res_ops_t *) ops;

	if (hw_res_ops->enable_interrupt == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	const int irq = DEV_IPC_GET_ARG1(*call);
	const errno_t ret = hw_res_ops->enable_interrupt(fun, irq);
	async_answer_0(call, ret);
}

static void remote_hw_res_disable_interrupt(ddf_fun_t *fun, void *ops,
    ipc_call_t *call)
{
	hw_res_ops_t *hw_res_ops = (hw_res_ops_t *) ops;

	if (hw_res_ops->disable_interrupt == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	const int irq = DEV_IPC_GET_ARG1(*call);
	const errno_t ret = hw_res_ops->disable_interrupt(fun, irq);
	async_answer_0(call, ret);
}

static void remote_hw_res_clear_interrupt(ddf_fun_t *fun, void *ops,
    ipc_call_t *call)
{
	hw_res_ops_t *hw_res_ops = (hw_res_ops_t *) ops;

	if (hw_res_ops->clear_interrupt == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	const int irq = DEV_IPC_GET_ARG1(*call);
	const errno_t ret = hw_res_ops->clear_interrupt(fun, irq);
	async_answer_0(call, ret);
}

static void remote_hw_res_get_resource_list(ddf_fun_t *fun, void *ops,
    ipc_call_t *call)
{
	hw_res_ops_t *hw_res_ops = (hw_res_ops_t *) ops;

	if (hw_res_ops->get_resource_list == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	hw_resource_list_t *hw_resources = hw_res_ops->get_resource_list(fun);
	if (hw_resources == NULL) {
		async_answer_0(call, ENOENT);
		return;
	}

	async_answer_1(call, EOK, hw_resources->count);

	ipc_call_t data;
	size_t len;
	if (!async_data_read_receive(&data, &len)) {
		/* Protocol error - the recipient is not accepting data */
		return;
	}

	async_data_read_finalize(&data, hw_resources->resources, len);
}

static void remote_hw_res_dma_channel_setup(ddf_fun_t *fun, void *ops,
    ipc_call_t *call)
{
	hw_res_ops_t *hw_res_ops = ops;

	if (hw_res_ops->dma_channel_setup == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	const unsigned channel = DEV_IPC_GET_ARG1(*call) & 0xffff;
	const uint8_t  mode = DEV_IPC_GET_ARG1(*call) >> 16;
	const uint32_t address = DEV_IPC_GET_ARG2(*call);
	const uint32_t size = DEV_IPC_GET_ARG3(*call);

	const errno_t ret = hw_res_ops->dma_channel_setup(
	    fun, channel, address, size, mode);
	async_answer_0(call, ret);
}

static void remote_hw_res_dma_channel_remain(ddf_fun_t *fun, void *ops,
    ipc_call_t *call)
{
	hw_res_ops_t *hw_res_ops = ops;

	if (hw_res_ops->dma_channel_setup == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	const unsigned channel = DEV_IPC_GET_ARG1(*call);
	size_t remain = 0;
	const errno_t ret = hw_res_ops->dma_channel_remain(fun, channel, &remain);
	async_answer_1(call, ret, remain);
}

/**
 * @}
 */
