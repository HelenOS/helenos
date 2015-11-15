/*
 * Copyright (c) 2015 Michal Koutny
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#define LIBC_ASYNC_C_
#include <ipc/ipc.h>
#include "private/async.h"
#include "private/taskman.h"
#undef LIBC_ASYNC_C_

#include <errno.h>
#include <ipc/taskman.h>
#include <taskman_noasync.h>




/** Tell taskman we are his NS
 *
 * @return EOK on success, otherwise propagated error code
 */
int taskman_intro_ns_noasync(void)
{
	assert(session_taskman);
	int phone = async_session_phone(session_taskman);

	ipc_call_async_0(phone, TASKMAN_I_AM_NS, NULL, NULL, false);

	ipc_call_async_3(phone, IPC_M_CONNECT_TO_ME, 0, 0, 0, NULL, NULL, false);

	/*
	 * Since this is a workaround for NS's low-level implementation, we can
	 * assume positive answer and return EOK.
	 */
	return EOK;
}


void task_retval_noasync(int retval)
{
	assert(session_taskman);
	int phone = async_session_phone(session_taskman);

	/* Just send it and don't wait for an answer. */
	ipc_call_async_2(phone, TASKMAN_RETVAL, retval, false,
	    NULL, NULL, false);
}

/** @}
 */
