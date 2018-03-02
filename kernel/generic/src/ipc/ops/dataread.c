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
	size_t size = IPC_GET_ARG2(call->data);

	if (size > DATA_XFER_LIMIT) {
		int flags = IPC_GET_ARG3(call->data);

		if (flags & IPC_XF_RESTRICT)
			IPC_SET_ARG2(call->data, DATA_XFER_LIMIT);
		else
			return ELIMIT;
	}

	return EOK;
}

static errno_t answer_preprocess(call_t *answer, ipc_data_t *olddata)
{
	assert(!answer->buffer);

	if (!IPC_GET_RETVAL(answer->data)) {
		/* The recipient agreed to send data. */
		uintptr_t src = IPC_GET_ARG1(answer->data);
		uintptr_t dst = IPC_GET_ARG1(*olddata);
		size_t max_size = IPC_GET_ARG2(*olddata);
		size_t size = IPC_GET_ARG2(answer->data);

		if (size && size <= max_size) {
			/*
			 * Copy the destination VA so that this piece of
			 * information is not lost.
			 */
			IPC_SET_ARG1(answer->data, dst);

			answer->buffer = malloc(size, 0);
			errno_t rc = copy_from_uspace(answer->buffer,
			    (void *) src, size);
			if (rc) {
				IPC_SET_RETVAL(answer->data, rc);
				/*
				 * answer->buffer will be cleaned up in
				 * ipc_call_free().
				 */
			}
		} else if (!size) {
			IPC_SET_RETVAL(answer->data, EOK);
		} else {
			IPC_SET_RETVAL(answer->data, ELIMIT);
		}
	}

	return EOK;
}

static errno_t answer_process(call_t *answer)
{
	if (answer->buffer) {
		uintptr_t dst = IPC_GET_ARG1(answer->data);
		size_t size = IPC_GET_ARG2(answer->data);
		errno_t rc;

		rc = copy_to_uspace((void *) dst, answer->buffer, size);
		if (rc)
			IPC_SET_RETVAL(answer->data, rc);
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
