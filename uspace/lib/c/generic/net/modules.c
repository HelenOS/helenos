/*
 * Copyright (c) 2009 Lukas Mejdrech
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
 * Generic module functions implementation.
 *
 * @todo MAKE IT POSSIBLE TO REMOVE THIS FILE VIA EITHER REPLACING PART OF ITS
 * FUNCTIONALITY OR VIA INTEGRATING ITS FUNCTIONALITY MORE TIGHTLY WITH THE REST
 * OF THE SYSTEM.
 */

#include <async.h>
#include <malloc.h>
#include <errno.h>
#include <sys/time.h>
#include <ipc/services.h>
#include <net/modules.h>
#include <ns.h>

/** Answer a call.
 *
 * @param[in] callid Call identifier.
 * @param[in] result Message processing result.
 * @param[in] answer Message processing answer.
 * @param[in] count  Number of answer parameters.
 *
 */
void answer_call(ipc_callid_t callid, int result, ipc_call_t *answer,
    size_t count)
{
	/* Choose the most efficient function */
	if ((answer != NULL) || (count == 0)) {
		switch (count) {
		case 0:
			async_answer_0(callid, (sysarg_t) result);
			break;
		case 1:
			async_answer_1(callid, (sysarg_t) result,
			    IPC_GET_ARG1(*answer));
			break;
		case 2:
			async_answer_2(callid, (sysarg_t) result,
			    IPC_GET_ARG1(*answer), IPC_GET_ARG2(*answer));
			break;
		case 3:
			async_answer_3(callid, (sysarg_t) result,
			    IPC_GET_ARG1(*answer), IPC_GET_ARG2(*answer),
			    IPC_GET_ARG3(*answer));
			break;
		case 4:
			async_answer_4(callid, (sysarg_t) result,
			    IPC_GET_ARG1(*answer), IPC_GET_ARG2(*answer),
			    IPC_GET_ARG3(*answer), IPC_GET_ARG4(*answer));
			break;
		case 5:
		default:
			async_answer_5(callid, (sysarg_t) result,
			    IPC_GET_ARG1(*answer), IPC_GET_ARG2(*answer),
			    IPC_GET_ARG3(*answer), IPC_GET_ARG4(*answer),
			    IPC_GET_ARG5(*answer));
			break;
		}
	}
}

/** Connect to the needed module.
 *
 * @param[in] need Needed module service.
 *
 * @return Session to the needed service.
 * @return NULL if the connection timeouted.
 *
 */
static async_sess_t *connect_to_service(services_t need)
{
	return service_connect_blocking(EXCHANGE_SERIALIZE, need, 0, 0);
}

/** Create bidirectional connection with the needed module service and register
 * the message receiver.
 *
 * @param[in] need            Needed module service.
 * @param[in] arg1            First parameter.
 * @param[in] arg2            Second parameter.
 * @param[in] arg3            Third parameter.
 * @param[in] client_receiver Message receiver.
 *
 * @return Session to the needed service.
 * @return Other error codes as defined for the async_connect_to_me()
 *         function.
 *
 */
async_sess_t *bind_service(services_t need, sysarg_t arg1, sysarg_t arg2,
    sysarg_t arg3, async_client_conn_t client_receiver)
{
	/* Connect to the needed service */
	async_sess_t *sess = connect_to_service(need);
	if (sess != NULL) {
		/* Request the bidirectional connection */
		async_exch_t *exch = async_exchange_begin(sess);
		int rc = async_connect_to_me(exch, arg1, arg2, arg3,
		    client_receiver, NULL);
		async_exchange_end(exch);
		
		if (rc != EOK) {
			async_hangup(sess);
			errno = rc;
			return NULL;
		}
	}
	
	return sess;
}

/** Refresh answer structure and argument count.
 *
 * Erase all arguments.
 *
 * @param[in,out] answer Message processing answer structure.
 * @param[in,out] count  Number of answer arguments.
 *
 */
void refresh_answer(ipc_call_t *answer, size_t *count)
{
	if (count != NULL)
		*count = 0;
	
	if (answer != NULL) {
		IPC_SET_RETVAL(*answer, 0);
		IPC_SET_IMETHOD(*answer, 0);
		IPC_SET_ARG1(*answer, 0);
		IPC_SET_ARG2(*answer, 0);
		IPC_SET_ARG3(*answer, 0);
		IPC_SET_ARG4(*answer, 0);
		IPC_SET_ARG5(*answer, 0);
	}
}

/** @}
 */
