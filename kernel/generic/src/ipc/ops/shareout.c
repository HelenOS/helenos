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
#include <ipc/ipc.h>
#include <mm/as.h>
#include <synch/spinlock.h>
#include <proc/task.h>
#include <syscall/copy.h>
#include <abi/errno.h>
#include <arch.h>

static errno_t request_preprocess(call_t *call, phone_t *phone)
{
	size_t size = as_area_get_size(IPC_GET_ARG1(call->data));

	if (!size)
		return EPERM;
	IPC_SET_ARG2(call->data, size);

	return EOK;
}

static errno_t answer_preprocess(call_t *answer, ipc_data_t *olddata)
{
	errno_t rc = EOK;

	if (!IPC_GET_RETVAL(answer->data)) {
		/* Accepted, handle as_area receipt */

		irq_spinlock_lock(&answer->sender->lock, true);
		as_t *as = answer->sender->as;
		irq_spinlock_unlock(&answer->sender->lock, true);

		uintptr_t dst_base = (uintptr_t) -1;
		rc = as_area_share(as, IPC_GET_ARG1(*olddata),
		    IPC_GET_ARG2(*olddata), AS, IPC_GET_ARG3(*olddata),
		    &dst_base, IPC_GET_ARG1(answer->data));

		if (rc == EOK) {
			rc = copy_to_uspace((void *) IPC_GET_ARG2(answer->data),
			    &dst_base, sizeof(dst_base));
		}

		IPC_SET_RETVAL(answer->data, rc);
	}

	return rc;
}

sysipc_ops_t ipc_m_share_out_ops = {
	.request_preprocess = request_preprocess,
	.request_forget = null_request_forget,
	.request_process = null_request_process,
	.answer_cleanup = null_answer_cleanup,
	.answer_preprocess = answer_preprocess,
	.answer_process = null_answer_process,
};

/** @}
 */
