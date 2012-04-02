/*
 * Copyright (c) 2011 Martin Decky
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

/** @addtogroup libnet
 * @{
 */

/** @file
 * Transport layer module skeleton implementation.
 * @see tl_skel.h
 */

#include <bool.h>
#include <errno.h>
#include <ns.h>
#include <tl_skel.h>
#include <net_interface.h>
#include <net/modules.h>

/** Default thread for new connections.
 *
 * @param[in] iid   The initial message identifier.
 * @param[in] icall The initial message call structure.
 * @param[in] arg   Local argument.
 *
 */
static void tl_client_connection(ipc_callid_t iid, ipc_call_t *icall,
    void *arg)
{
	/*
	 * Accept the connection by answering
	 * the initial IPC_M_CONNECT_ME_TO call.
	 */
	async_answer_0(iid, EOK);
	
	/* Per-connection initialization */
	tl_connection();
	
	while (true) {
		ipc_call_t answer;
		size_t count;
		
		/* Clear the answer structure */
		refresh_answer(&answer, &count);
		
		/* Fetch the next message */
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		/* Process the message */
		int res = tl_message(callid, &call, &answer, &count);
		
		/*
		 * End if told to either by the message or the processing
		 * result.
		 */
		if ((!IPC_GET_IMETHOD(call)) || (res == EHANGUP))
			return;
		
		/* Answer the message */
		answer_call(callid, res, &answer, count);
	}
}

/** Start the trasport layer module.
 *
 * Initialize the client connection serving function, initialize
 * the module, register the module service and start the async
 * manager, processing IPC messages in an infinite loop.
 *
 * @param[in] service Service identification.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the pm_init() function.
 * @return Other error codes as defined for the il_initialize()
 *         function.
 * @return Other error codes as defined for the REGISTER_ME() macro
 *         function.
 *
 */
int tl_module_start(sysarg_t service)
{
	async_set_client_connection(tl_client_connection);
	async_sess_t *sess = net_connect_module();
	if (!sess)
		return ENOENT;
	
	int rc = pm_init();
	if (rc != EOK)
		return rc;
	
	rc = tl_initialize(sess);
	if (rc != EOK)
		goto out;
	
	rc = service_register(service);
	if (rc != EOK)
		goto out;
	
	task_retval(0);
	async_manager();
	
out:
	pm_destroy();
	return rc;
}

/** @}
 */
