/*
 * SPDX-FileCopyrightText: 2012 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_ipc
 * @{
 */
/** @file
 */

#include <ipc/sysipc_ops.h>
#include <abi/ipc/methods.h>
#include <abi/errno.h>

/* Forward declarations. */
extern sysipc_ops_t ipc_m_connect_to_me_ops;
extern sysipc_ops_t ipc_m_connect_me_to_ops;
extern sysipc_ops_t ipc_m_page_in_ops;
extern sysipc_ops_t ipc_m_share_out_ops;
extern sysipc_ops_t ipc_m_share_in_ops;
extern sysipc_ops_t ipc_m_data_write_ops;
extern sysipc_ops_t ipc_m_data_read_ops;
extern sysipc_ops_t ipc_m_state_change_authorize_ops;
extern sysipc_ops_t ipc_m_debug_ops;

static sysipc_ops_t *sysipc_ops[] = {
	[IPC_M_CONNECT_TO_ME] = &ipc_m_connect_to_me_ops,
	[IPC_M_CONNECT_ME_TO] = &ipc_m_connect_me_to_ops,
	[IPC_M_PAGE_IN] = &ipc_m_page_in_ops,
	[IPC_M_SHARE_OUT] = &ipc_m_share_out_ops,
	[IPC_M_SHARE_IN] = &ipc_m_share_in_ops,
	[IPC_M_DATA_WRITE] = &ipc_m_data_write_ops,
	[IPC_M_DATA_READ] = &ipc_m_data_read_ops,
	[IPC_M_STATE_CHANGE_AUTHORIZE] = &ipc_m_state_change_authorize_ops,
	[IPC_M_DEBUG] = &ipc_m_debug_ops
};

static sysipc_ops_t null_ops = {
	.request_preprocess = null_request_preprocess,
	.request_forget = null_request_forget,
	.request_process = null_request_process,
	.answer_cleanup = null_answer_cleanup,
	.answer_preprocess = null_answer_preprocess,
	.answer_process = null_answer_process,
};

errno_t null_request_preprocess(call_t *call, phone_t *phone)
{
	return EOK;
}

errno_t null_request_forget(call_t *call)
{
	return EOK;
}

int null_request_process(call_t *call, answerbox_t *box)
{
	return 0;
}

errno_t null_answer_cleanup(call_t *call, ipc_data_t *data)
{
	return EOK;
}

errno_t null_answer_preprocess(call_t *call, ipc_data_t *data)
{
	return EOK;
}

errno_t null_answer_process(call_t *call)
{
	return EOK;
}

sysipc_ops_t *sysipc_ops_get(sysarg_t imethod)
{
	if (imethod < sizeof(sysipc_ops) / (sizeof(sysipc_ops_t *)))
		return sysipc_ops[imethod] ? sysipc_ops[imethod] : &null_ops;

	return &null_ops;
}

/** @}
 */
