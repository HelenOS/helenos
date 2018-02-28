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

#include <assert.h>
#include <ipc/sysipc_ops.h>
#include <ipc/ipc.h>
#include <mm/slab.h>
#include <abi/errno.h>
#include <syscall/copy.h>
#include <config.h>

static errno_t request_preprocess(call_t *call, phone_t *phone)
{
	uintptr_t src = IPC_GET_ARG1(call->data);
	size_t size = IPC_GET_ARG2(call->data);

	if (size > DATA_XFER_LIMIT) {
		int flags = IPC_GET_ARG3(call->data);

		if (flags & IPC_XF_RESTRICT) {
			size = DATA_XFER_LIMIT;
			IPC_SET_ARG2(call->data, size);
		} else
			return ELIMIT;
	}

	call->buffer = (uint8_t *) malloc(size, 0);
	errno_t rc = copy_from_uspace(call->buffer, (void *) src, size);
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

	if (!IPC_GET_RETVAL(answer->data)) {
		/* The recipient agreed to receive data. */
		uintptr_t dst = (uintptr_t)IPC_GET_ARG1(answer->data);
		size_t size = (size_t)IPC_GET_ARG2(answer->data);
		size_t max_size = (size_t)IPC_GET_ARG2(*olddata);

		if (size <= max_size) {
			errno_t rc = copy_to_uspace((void *) dst,
			    answer->buffer, size);
			if (rc)
				IPC_SET_RETVAL(answer->data, rc);
		} else {
			IPC_SET_RETVAL(answer->data, ELIMIT);
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
