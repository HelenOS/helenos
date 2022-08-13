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
#include <ipc/ipc.h>
#include <mm/as.h>
#include <synch/spinlock.h>
#include <proc/task.h>
#include <syscall/copy.h>
#include <abi/errno.h>
#include <arch.h>

static errno_t request_preprocess(call_t *call, phone_t *phone)
{
	size_t size = as_area_get_size(ipc_get_arg1(&call->data));

	if (!size)
		return EPERM;
	ipc_set_arg2(&call->data, size);

	return EOK;
}

static errno_t answer_preprocess(call_t *answer, ipc_data_t *olddata)
{
	errno_t rc = EOK;

	if (!ipc_get_retval(&answer->data)) {
		/* Accepted, handle as_area receipt */

		irq_spinlock_lock(&answer->sender->lock, true);
		as_t *as = answer->sender->as;
		irq_spinlock_unlock(&answer->sender->lock, true);

		uintptr_t dst_base = (uintptr_t) -1;
		rc = as_area_share(as, ipc_get_arg1(olddata),
		    ipc_get_arg2(olddata), AS, ipc_get_arg3(olddata),
		    &dst_base, ipc_get_arg1(&answer->data));

		if (rc == EOK) {
			rc = copy_to_uspace(ipc_get_arg2(&answer->data),
			    &dst_base, sizeof(dst_base));
		}

		ipc_set_retval(&answer->data, rc);
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
