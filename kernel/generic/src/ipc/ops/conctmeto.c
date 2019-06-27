/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

/** @addtogroup kernel_generic_ipc
 * @{
 */
/** @file
 */

#include <ipc/sysipc_ops.h>
#include <ipc/ipc.h>
#include <ipc/ipcrsc.h>
#include <abi/errno.h>
#include <arch.h>

static errno_t request_preprocess(call_t *call, phone_t *phone)
{
	/*
	 * Create the new phone and capability, but don't publish them yet.
	 * That will be done once the phone is connected.
	 */
	cap_phone_handle_t phandle;
	kobject_t *pobj;
	errno_t rc = phone_alloc(TASK, false, &phandle, &pobj);
	if (rc != EOK) {
		call->priv = 0;
		return rc;
	}

	/* Move pobj's reference to call->priv */
	call->priv = (sysarg_t) pobj;
	pobj = NULL;

	/* Remember the handle */
	ipc_set_arg5(&call->data, (sysarg_t) phandle);

	return EOK;
}

static errno_t request_forget(call_t *call)
{
	cap_phone_handle_t phandle = (cap_handle_t) ipc_get_arg5(&call->data);

	if (cap_handle_raw(phandle) < 0)
		return EOK;

	/* Move reference from call->priv to pobj */
	kobject_t *pobj = (kobject_t *) call->priv;
	call->priv = 0;
	/* Drop pobj's reference */
	kobject_put(pobj);
	cap_free(TASK, phandle);

	return EOK;
}

static errno_t answer_preprocess(call_t *answer, ipc_data_t *olddata)
{
	/* Get an extra reference for phone */
	kobject_t *pobj = (kobject_t *) answer->priv;
	kobject_add_ref(pobj);

	/* Set the recipient-assigned label */
	pobj->phone->label = ipc_get_arg5(&answer->data);

	/* Restore phone handle in answer's ARG5 */
	ipc_set_arg5(&answer->data, ipc_get_arg5(olddata));

	/* If the user accepted the call, connect */
	if (ipc_get_retval(&answer->data) == EOK) {
		/* Hand over reference from pobj to the answerbox */
		(void) ipc_phone_connect(pobj->phone, &TASK->answerbox);
	} else {
		/* Drop the extra reference */
		kobject_put(pobj);
	}

	return EOK;
}

static errno_t answer_process(call_t *answer)
{
	cap_phone_handle_t phandle = (cap_handle_t) ipc_get_arg5(&answer->data);
	/* Move the reference from answer->priv to pobj */
	kobject_t *pobj = (kobject_t *) answer->priv;
	answer->priv = 0;

	if (ipc_get_retval(&answer->data)) {
		if (cap_handle_raw(phandle) >= 0) {
			/*
			 * Cleanup the unpublished capability and drop
			 * phone->kobject's reference.
			 */
			kobject_put(pobj);
			cap_free(TASK, phandle);
		}
	} else {
		/*
		 * Publish the capability. Publishing the capability this late
		 * is important for ipc_cleanup() where we want to have a
		 * capability for each phone that wasn't hung up by the user.
		 */
		cap_publish(TASK, phandle, pobj);
	}

	return EOK;
}

sysipc_ops_t ipc_m_connect_me_to_ops = {
	.request_preprocess = request_preprocess,
	.request_forget = request_forget,
	.request_process = null_request_process,
	.answer_cleanup = null_answer_cleanup,
	.answer_preprocess = answer_preprocess,
	.answer_process = answer_process,
};

/** @}
 */
