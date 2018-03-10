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

/** @addtogroup genericipc
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
	cap_handle_t phone_handle;
	kobject_t *phone_obj;
	errno_t rc = phone_alloc(TASK, false, &phone_handle, &phone_obj);
	call->priv = (sysarg_t) phone_obj;
	IPC_SET_ARG5(call->data, (rc == EOK) ? phone_handle : -1);
	return 0;
}

static errno_t answer_cleanup(call_t *answer, ipc_data_t *olddata)
{
	cap_handle_t phone_handle = (cap_handle_t) IPC_GET_ARG5(*olddata);
	kobject_t *phone_obj = (kobject_t *) answer->priv;

	if (phone_handle >= 0) {
		kobject_put(phone_obj);
		cap_free(TASK, phone_handle);
	}

	return EOK;
}

static errno_t answer_preprocess(call_t *answer, ipc_data_t *olddata)
{
	cap_handle_t phone_handle = (cap_handle_t) IPC_GET_ARG5(*olddata);
	kobject_t *phone_obj = (kobject_t *) answer->priv;

	if (IPC_GET_RETVAL(answer->data) != EOK) {
		/* The connection was not accepted */
		answer_cleanup(answer, olddata);
	} else if (phone_handle >= 0) {
		/*
		 * The connection was accepted
		 */

		/*
		 * We need to create another reference as the one we have now
		 * will be consumed by ipc_phone_connect().
		 */
		kobject_add_ref(phone_obj);

		if (ipc_phone_connect(phone_obj->phone,
		    &answer->sender->answerbox)) {
			/* Pass the reference to the capability */
			cap_publish(TASK, phone_handle, phone_obj);
			/* Set 'phone hash' as ARG5 of response */
			IPC_SET_ARG5(answer->data,
			    (sysarg_t) phone_obj->phone);
		} else {
			/* The answerbox is shutting down. */
			IPC_SET_RETVAL(answer->data, ENOENT);
			answer_cleanup(answer, olddata);
		}
	} else {
		IPC_SET_RETVAL(answer->data, ELIMIT);
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
