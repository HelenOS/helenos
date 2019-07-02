/*
 * Copyright (c) 2008 Jiri Svoboda
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
