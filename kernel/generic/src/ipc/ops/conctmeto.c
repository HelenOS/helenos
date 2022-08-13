/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
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
