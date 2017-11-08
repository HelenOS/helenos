/*
 * Copyright (c) 2005 Jakub Jermar
 * Copyright (c) 2011 Jiri Svoboda
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

/** @file Ski console driver.
 */

#include <ddf/driver.h>
#include <ddf/log.h>
#include <errno.h>
#include <ipc/char.h>
#include <stdint.h>
#include <stdlib.h>
#include <thread.h>
#include <stdbool.h>

#include "ski-con.h"

#define SKI_GETCHAR		21

#define POLL_INTERVAL		10000

static void ski_con_thread_impl(void *arg);
static int32_t ski_con_getchar(void);
static void ski_con_connection(ipc_callid_t, ipc_call_t *, void *);

#include <stdio.h>
/** Initialize Ski port driver. */
int ski_con_add(ski_con_t *con)
{
	thread_id_t tid;
	ddf_fun_t *fun = NULL;
	bool bound = false;
	int rc;

	fun = ddf_fun_create(con->dev, fun_exposed, "a");
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Error creating function 'a'.");
		rc = ENOMEM;
		goto error;
	}

	ddf_fun_set_conn_handler(fun, ski_con_connection);

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error binding function 'a'.");
		goto error;
	}

	bound = true;

	rc = thread_create(ski_con_thread_impl, con, "kbd_poll", &tid);
	if (rc != 0) {
		return rc;
	}

	return EOK;
error:
	if (bound)
		ddf_fun_unbind(fun);
	if (fun != NULL)
		ddf_fun_destroy(fun);

	return rc;
}

/** Remove ski console device */
int ski_con_remove(ski_con_t *con)
{
	return ENOTSUP;
}

/** Ski console device gone */
int ski_con_gone(ski_con_t *con)
{
	return ENOTSUP;
}

/** Thread to poll Ski for keypresses. */
static void ski_con_thread_impl(void *arg)
{
	int32_t c;
	ski_con_t *con = (ski_con_t *) arg;

	while (1) {
		while (1) {
			c = ski_con_getchar();
			if (c == 0)
				break;

			if (con->client_sess != NULL) {
				async_exch_t *exch = async_exchange_begin(con->client_sess);
				async_msg_1(exch, CHAR_NOTIF_BYTE, c);
				async_exchange_end(exch);
			}
		}

		thread_usleep(POLL_INTERVAL);
	}
}

/** Ask Ski if a key was pressed.
 *
 * Use SSC (Simulator System Call) to get character from the debug console.
 * This call is non-blocking.
 *
 * @return ASCII code of pressed key or 0 if no key pressed.
 */
static int32_t ski_con_getchar(void)
{
	uint64_t ch;

#ifdef UARCH_ia64
	asm volatile (
		"mov r15 = %1\n"
		"break 0x80000;;\n"	/* modifies r8 */
		"mov %0 = r8;;\n"

		: "=r" (ch)
		: "i" (SKI_GETCHAR)
		: "r15", "r8"
	);
#else
	ch = 0;
#endif
	return (int32_t) ch;
}

static void ski_con_putchar(ski_con_t *con, char ch)
{
	
}

/** Character device connection handler. */
static void ski_con_connection(ipc_callid_t iid, ipc_call_t *icall,
    void *arg)
{
	ski_con_t *con;

	/* Answer the IPC_M_CONNECT_ME_TO call. */
	async_answer_0(iid, EOK);

	con = (ski_con_t *)ddf_dev_data_get(ddf_fun_get_dev((ddf_fun_t *)arg));

	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		sysarg_t method = IPC_GET_IMETHOD(call);

		if (!method) {
			/* The other side has hung up. */
			async_answer_0(callid, EOK);
			return;
		}

		async_sess_t *sess =
		    async_callback_receive_start(EXCHANGE_SERIALIZE, &call);
		if (sess != NULL) {
			if (con->client_sess == NULL) {
				con->client_sess = sess;
				async_answer_0(callid, EOK);
			} else
				async_answer_0(callid, ELIMIT);
		} else {
			switch (method) {
			case CHAR_WRITE_BYTE:
				ddf_msg(LVL_DEBUG, "Write %" PRIun " to device\n",
				    IPC_GET_ARG1(call));
				ski_con_putchar(con, (uint8_t) IPC_GET_ARG1(call));
				async_answer_0(callid, EOK);
				break;
			default:
				async_answer_0(callid, EINVAL);
			}
		}
	}
}

/** @}
 */
