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

#include <assert.h>
#include <ipc/sysipc_ops.h>
#include <ipc/ipc.h>
#include <stdlib.h>
#include <abi/errno.h>
#include <syscall/copy.h>
#include <config.h>

static errno_t request_preprocess(call_t *call, phone_t *phone)
{
	uspace_addr_t src = ipc_get_arg1(&call->data);
	size_t size = ipc_get_arg2(&call->data);

	if (size > DATA_XFER_LIMIT) {
		int flags = ipc_get_arg3(&call->data);

		if (flags & IPC_XF_RESTRICT) {
			size = DATA_XFER_LIMIT;
			ipc_set_arg2(&call->data, size);
		} else
			return ELIMIT;
	}

	call->buffer = (uint8_t *) malloc(size);
	if (!call->buffer)
		return ENOMEM;
	errno_t rc = copy_from_uspace(call->buffer, src, size);
	if (rc != EOK) {
		/*
		 * call->buffer will be cleaned up in ipc_call_free() at the
		 * latest.
		 */
		return rc;
	}

	return EOK;
}

static errno_t answer_preprocess(call_t *answer, ipc_data_t *olddata)
{
	assert(answer->buffer);

	if (!ipc_get_retval(&answer->data)) {
		/* The recipient agreed to receive data. */
		uspace_addr_t dst = ipc_get_arg1(&answer->data);
		size_t size = ipc_get_arg2(&answer->data);
		size_t max_size = ipc_get_arg2(olddata);

		if (size <= max_size) {
			errno_t rc = copy_to_uspace(dst,
			    answer->buffer, size);
			if (rc)
				ipc_set_retval(&answer->data, rc);
		} else {
			ipc_set_retval(&answer->data, ELIMIT);
		}
	}

	return EOK;
}

sysipc_ops_t ipc_m_data_write_ops = {
	.request_preprocess = request_preprocess,
	.request_forget = null_request_forget,
	.request_process = null_request_process,
	.answer_cleanup = null_answer_cleanup,
	.answer_preprocess = answer_preprocess,
	.answer_process = null_answer_process,
};

/** @}
 */
