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
	size_t size = ipc_get_arg2(&call->data);

	if (size > DATA_XFER_LIMIT) {
		int flags = ipc_get_arg3(&call->data);

		if (flags & IPC_XF_RESTRICT)
			ipc_set_arg2(&call->data, DATA_XFER_LIMIT);
		else
			return ELIMIT;
	}

	return EOK;
}

static errno_t answer_preprocess(call_t *answer, ipc_data_t *olddata)
{
	assert(!answer->buffer);

	if (!ipc_get_retval(&answer->data)) {
		/* The recipient agreed to send data. */
		uspace_addr_t src = ipc_get_arg1(&answer->data);
		uspace_addr_t dst = ipc_get_arg1(olddata);
		size_t max_size = ipc_get_arg2(olddata);
		size_t size = ipc_get_arg2(&answer->data);

		if (size && size <= max_size) {
			/*
			 * Copy the destination VA so that this piece of
			 * information is not lost.
			 */
			ipc_set_arg1(&answer->data, dst);

			answer->buffer = malloc(size);
			if (!answer->buffer) {
				ipc_set_retval(&answer->data, ENOMEM);
				return EOK;
			}
			errno_t rc = copy_from_uspace(answer->buffer,
			    src, size);
			if (rc) {
				ipc_set_retval(&answer->data, rc);
				/*
				 * answer->buffer will be cleaned up in
				 * ipc_call_free().
				 */
				return EOK;
			}
		} else if (!size) {
			ipc_set_retval(&answer->data, EOK);
		} else {
			ipc_set_retval(&answer->data, ELIMIT);
		}
	}

	return EOK;
}

static errno_t answer_process(call_t *answer)
{
	if (answer->buffer) {
		uspace_addr_t dst = ipc_get_arg1(&answer->data);
		size_t size = ipc_get_arg2(&answer->data);
		errno_t rc;

		rc = copy_to_uspace(dst, answer->buffer, size);
		if (rc)
			ipc_set_retval(&answer->data, rc);
	}

	return EOK;
}

sysipc_ops_t ipc_m_data_read_ops = {
	.request_preprocess = request_preprocess,
	.request_forget = null_request_forget,
	.request_process = null_request_process,
	.answer_cleanup = null_answer_cleanup,
	.answer_preprocess = answer_preprocess,
	.answer_process = answer_process,
};

/** @}
 */
