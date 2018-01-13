/*
 * Copyright (c) 2012 Jakub Jermar 
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

/** @addtogroup genericipc
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
