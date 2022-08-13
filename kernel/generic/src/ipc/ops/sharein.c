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
#include <abi/errno.h>
#include <arch.h>

static errno_t answer_preprocess(call_t *answer, ipc_data_t *olddata)
{
	if (!ipc_get_retval(&answer->data)) {
		irq_spinlock_lock(&answer->sender->lock, true);
		as_t *as = answer->sender->as;
		irq_spinlock_unlock(&answer->sender->lock, true);

		uintptr_t dst_base = (uintptr_t) -1;
		errno_t rc = as_area_share(AS, ipc_get_arg1(&answer->data),
		    ipc_get_arg1(olddata), as, ipc_get_arg2(&answer->data),
		    &dst_base, ipc_get_arg2(olddata));
		ipc_set_arg5(&answer->data, dst_base);
		ipc_set_retval(&answer->data, rc);
	}

	return EOK;
}

sysipc_ops_t ipc_m_share_in_ops = {
	.request_preprocess = null_request_preprocess,
	.request_forget = null_request_forget,
	.request_process = null_request_process,
	.answer_cleanup = null_answer_cleanup,
	.answer_preprocess = answer_preprocess,
	.answer_process = null_answer_process,
};

/** @}
 */
