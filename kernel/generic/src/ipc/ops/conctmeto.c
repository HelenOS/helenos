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

static int request_preprocess(call_t *call, phone_t *phone)
{
	int newphid = phone_alloc(TASK);

	if (newphid < 0)
		return ELIMIT;
		
	/* Set arg5 for server */
	IPC_SET_ARG5(call->data, (sysarg_t) &TASK->phones[newphid]);
	call->priv = newphid;

	return EOK;
}

static int request_forget(call_t *call)
{
	phone_dealloc(call->priv);
	return EOK;
}

static int answer_preprocess(call_t *answer, ipc_data_t *olddata)
{
	phone_t *phone = (phone_t *) IPC_GET_ARG5(*olddata);

	/* If the user accepted call, connect */
	if (IPC_GET_RETVAL(answer->data) == EOK)
		(void) ipc_phone_connect(phone, &TASK->answerbox);

	return EOK;
}

static int answer_process(call_t *answer)
{
	if (IPC_GET_RETVAL(answer->data))
		phone_dealloc(answer->priv);
	else
		IPC_SET_ARG5(answer->data, answer->priv);
	
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
