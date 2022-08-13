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
#include <ipc/ipcrsc.h>
#include <synch/mutex.h>
#include <proc/task.h>
#include <abi/errno.h>
#include <macros.h>

static errno_t request_preprocess(call_t *call, phone_t *phone)
{
	task_t *other_task_s;

	kobject_t *sender_obj = kobject_get(TASK,
	    (cap_handle_t) ipc_get_arg5(&call->data), KOBJECT_TYPE_PHONE);
	if (!sender_obj)
		return ENOENT;

	mutex_lock(&sender_obj->phone->lock);
	if (sender_obj->phone->state != IPC_PHONE_CONNECTED) {
		mutex_unlock(&sender_obj->phone->lock);
		kobject_put(sender_obj);
		return EINVAL;
	}

	other_task_s = sender_obj->phone->callee->task;

	mutex_unlock(&sender_obj->phone->lock);

	/* Remember the third party task hash. */
	ipc_set_arg5(&call->data, (sysarg_t) other_task_s);

	kobject_put(sender_obj);
	return EOK;
}

static errno_t answer_preprocess(call_t *answer, ipc_data_t *olddata)
{
	errno_t rc = EOK;

	if (!ipc_get_retval(&answer->data)) {
		/* The recipient authorized the change of state. */
		task_t *other_task_s;
		task_t *other_task_r;

		kobject_t *recipient_obj = kobject_get(TASK,
		    (cap_handle_t) ipc_get_arg1(&answer->data),
		    KOBJECT_TYPE_PHONE);
		if (!recipient_obj) {
			ipc_set_retval(&answer->data, ENOENT);
			return ENOENT;
		}

		mutex_lock(&recipient_obj->phone->lock);
		if (recipient_obj->phone->state != IPC_PHONE_CONNECTED) {
			mutex_unlock(&recipient_obj->phone->lock);
			ipc_set_retval(&answer->data, EINVAL);
			kobject_put(recipient_obj);
			return EINVAL;
		}

		other_task_r = recipient_obj->phone->callee->task;
		other_task_s = (task_t *) ipc_get_arg5(olddata);

		/*
		 * See if both the sender and the recipient meant the
		 * same third party task.
		 */
		if (other_task_r != other_task_s) {
			ipc_set_retval(&answer->data, EINVAL);
			rc = EINVAL;
		} else {
			rc = event_task_notify_5(other_task_r,
			    EVENT_TASK_STATE_CHANGE, false,
			    ipc_get_arg1(olddata),
			    ipc_get_arg2(olddata),
			    ipc_get_arg3(olddata),
			    LOWER32(olddata->task_id),
			    UPPER32(olddata->task_id));
			ipc_set_retval(&answer->data, rc);
		}

		mutex_unlock(&recipient_obj->phone->lock);
		kobject_put(recipient_obj);
	}

	return rc;
}

sysipc_ops_t ipc_m_state_change_authorize_ops = {
	.request_preprocess = request_preprocess,
	.request_forget = null_request_forget,
	.request_process = null_request_process,
	.answer_cleanup = null_answer_cleanup,
	.answer_preprocess = answer_preprocess,
	.answer_process = null_answer_process,
};

/** @}
 */
