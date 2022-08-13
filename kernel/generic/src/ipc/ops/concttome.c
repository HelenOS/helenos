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

static int request_process(call_t *call, answerbox_t *box)
{
	cap_phone_handle_t phandle = CAP_NIL;
	kobject_t *pobj = NULL;
	errno_t rc = phone_alloc(TASK, false, &phandle, &pobj);
	if (rc == EOK) {
		/*
		 * Set the sender-assigned label to the new phone.
		 */
		pobj->phone->label = ipc_get_arg5(&call->data);
	}
	call->priv = (sysarg_t) pobj;
	ipc_set_arg5(&call->data, cap_handle_raw(phandle));
	return 0;
}

static errno_t answer_cleanup(call_t *answer, ipc_data_t *olddata)
{
	cap_phone_handle_t phandle = (cap_handle_t) ipc_get_arg5(olddata);
	kobject_t *pobj = (kobject_t *) answer->priv;

	if (cap_handle_valid(phandle)) {
		kobject_put(pobj);
		cap_free(TASK, phandle);
	}

	return EOK;
}

static errno_t answer_preprocess(call_t *answer, ipc_data_t *olddata)
{
	cap_phone_handle_t phandle = (cap_handle_t) ipc_get_arg5(olddata);
	kobject_t *pobj = (kobject_t *) answer->priv;

	if (ipc_get_retval(&answer->data) != EOK) {
		/* The connection was not accepted */
		answer_cleanup(answer, olddata);
	} else if (cap_handle_valid(phandle)) {
		/*
		 * The connection was accepted
		 */

		/*
		 * We need to create another reference as the one we have now
		 * will be consumed by ipc_phone_connect().
		 */
		kobject_add_ref(pobj);

		if (ipc_phone_connect(pobj->phone,
		    &answer->sender->answerbox)) {
			/* Pass the reference to the capability */
			cap_publish(TASK, phandle, pobj);
		} else {
			/* The answerbox is shutting down. */
			ipc_set_retval(&answer->data, ENOENT);
			answer_cleanup(answer, olddata);
		}
	} else {
		ipc_set_retval(&answer->data, ELIMIT);
	}

	return EOK;
}

sysipc_ops_t ipc_m_connect_to_me_ops = {
	.request_preprocess = null_request_preprocess,
	.request_forget = null_request_forget,
	.request_process = request_process,
	.answer_cleanup = answer_cleanup,
	.answer_preprocess = answer_preprocess,
	.answer_process = null_answer_process,
};

/** @}
 */
