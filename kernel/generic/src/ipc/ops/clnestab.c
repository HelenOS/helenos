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

/** @addtogroup genericipc
 * @{
 */
/** @file
 */

#include <ipc/sysipc_ops.h>
#include <ipc/ipc.h>
#include <synch/mutex.h>
#include <abi/errno.h>

static int request_preprocess(call_t *call, phone_t *phone)
{
	IPC_SET_ARG5(call->data, (sysarg_t) phone);

	return EOK;	
}

static int answer_cleanup(call_t *answer, ipc_data_t *olddata)
{
	phone_t *phone = (phone_t *) IPC_GET_ARG5(*olddata);

	mutex_lock(&phone->lock);
	if (phone->state == IPC_PHONE_CONNECTED) {
		irq_spinlock_lock(&phone->callee->lock, true);
		list_remove(&phone->link);
		phone->state = IPC_PHONE_SLAMMED;
		irq_spinlock_unlock(&phone->callee->lock, true);
	}
	mutex_unlock(&phone->lock);

	return EOK;
}

static int answer_preprocess(call_t *answer, ipc_data_t *olddata)
{

	if (IPC_GET_RETVAL(answer->data) != EOK) {
		/*
		 * The other party on the cloned phone rejected our request
		 * for connection on the protocol level.  We need to break the
		 * connection without sending IPC_M_HUNGUP back.
		 */
		answer_cleanup(answer, olddata);
	}
	
	return EOK;
}

sysipc_ops_t ipc_m_clone_establish_ops = {
	.request_preprocess = request_preprocess,
	.request_forget = null_request_forget,
	.request_process = null_request_process,
	.answer_cleanup = answer_cleanup,
	.answer_preprocess = answer_preprocess,
	.answer_process = null_answer_process,
};

/** @}
 */
