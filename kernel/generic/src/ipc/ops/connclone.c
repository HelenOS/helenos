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
#include <ipc/ipcrsc.h>
#include <synch/mutex.h>
#include <abi/errno.h>
#include <arch.h>

static void phones_lock(phone_t *p1, phone_t *p2)
{
	if (p1 < p2) {
		mutex_lock(&p1->lock);
		mutex_lock(&p2->lock);
	} else if (p1 > p2) {
		mutex_lock(&p2->lock);
		mutex_lock(&p1->lock);
	} else
		mutex_lock(&p1->lock);
}

static void phones_unlock(phone_t *p1, phone_t *p2)
{
	mutex_unlock(&p1->lock);
	if (p1 != p2)
		mutex_unlock(&p2->lock);
}

static int request_preprocess(call_t *call, phone_t *phone)
{
	phone_t *cloned_phone;

	if (phone_get(IPC_GET_ARG1(call->data), &cloned_phone) != EOK)
		return ENOENT;
		
	phones_lock(cloned_phone, phone);
		
	if ((cloned_phone->state != IPC_PHONE_CONNECTED) ||
	    phone->state != IPC_PHONE_CONNECTED) {
		phones_unlock(cloned_phone, phone);
		return EINVAL;
	}
		
	/*
	 * We can be pretty sure now that both tasks exist and we are
	 * connected to them. As we continue to hold the phone locks,
	 * we are effectively preventing them from finishing their
	 * potential cleanup.
	 *
	 */
	int newphid = phone_alloc(phone->callee->task);
	if (newphid < 0) {
		phones_unlock(cloned_phone, phone);
		return ELIMIT;
	}
		
	(void) ipc_phone_connect(&phone->callee->task->phones[newphid],
	    cloned_phone->callee);
	phones_unlock(cloned_phone, phone);
		
	/* Set the new phone for the callee. */
	IPC_SET_ARG1(call->data, newphid);

	return EOK;
}

static int answer_cleanup(call_t *answer, ipc_data_t *olddata)
{
	int phoneid = (int) IPC_GET_ARG1(*olddata);
	phone_t *phone = &TASK->phones[phoneid];

	/*
	 * In this case, the connection was established at the request
	 * time and therefore we need to slam the phone.  We don't
	 * merely hangup as that would result in sending IPC_M_HUNGUP
	 * to the third party on the other side of the cloned phone.
	 */
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
		 * The recipient of the cloned phone rejected the offer.
		 */
		answer_cleanup(answer, olddata);
	}

	return EOK;
}

sysipc_ops_t ipc_m_connection_clone_ops = {
	.request_preprocess = request_preprocess,
	.request_forget = null_request_forget,
	.request_process = null_request_process,
	.answer_cleanup = answer_cleanup,
	.answer_preprocess = answer_preprocess,
	.answer_process = null_answer_process,
};

/** @}
 */
