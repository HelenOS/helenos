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

	phone_t *sender_phone = phone_from_kobject(kobject_get(TASK,
	    (cap_handle_t) ipc_get_arg5(&call->data), KOBJECT_TYPE_PHONE));
	if (!sender_phone)
		return ENOENT;

	mutex_lock(&sender_phone->lock);
	if (sender_phone->state != IPC_PHONE_CONNECTED) {
		mutex_unlock(&sender_phone->lock);
		kobject_put(&sender_phone->kobject);
		return EINVAL;
	}

	other_task_s = sender_phone->callee->task;

	mutex_unlock(&sender_phone->lock);

	/* Remember the third party task hash. */
	ipc_set_arg5(&call->data, (sysarg_t) other_task_s);

	kobject_put(&sender_phone->kobject);
	return EOK;
}

static errno_t answer_preprocess(call_t *answer, ipc_data_t *olddata)
{
	errno_t rc = EOK;

	if (!ipc_get_retval(&answer->data)) {
		/* The recipient authorized the change of state. */
		task_t *other_task_s;
		task_t *other_task_r;

		phone_t *recipient_phone = phone_from_kobject(kobject_get(TASK,
		    (cap_handle_t) ipc_get_arg1(&answer->data),
		    KOBJECT_TYPE_PHONE));
		if (!recipient_phone) {
			ipc_set_retval(&answer->data, ENOENT);
			return ENOENT;
		}

		mutex_lock(&recipient_phone->lock);
		if (recipient_phone->state != IPC_PHONE_CONNECTED) {
			mutex_unlock(&recipient_phone->lock);
			ipc_set_retval(&answer->data, EINVAL);
			kobject_put(&recipient_phone->kobject);
			return EINVAL;
		}

		other_task_r = recipient_phone->callee->task;
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

		mutex_unlock(&recipient_phone->lock);
		kobject_put(&recipient_phone->kobject);
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
