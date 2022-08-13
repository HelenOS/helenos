/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
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
#include <udebug/udebug_ipc.h>
#include <syscall/copy.h>
#include <abi/errno.h>

static int request_process(call_t *call, answerbox_t *box)
{
	return -1;
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

sysipc_ops_t ipc_m_debug_ops = {
#ifdef CONFIG_UDEBUG
	.request_preprocess = udebug_request_preprocess,
#else
	.request_preprocess = null_request_preprocess,
#endif
	.request_forget = null_request_forget,
	.request_process = request_process,
	.answer_cleanup = null_answer_cleanup,
	.answer_preprocess = null_answer_preprocess,
	.answer_process = answer_process,
};

/** @}
 */
