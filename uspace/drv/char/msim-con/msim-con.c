/*
 * Copyright (c) 2006 Josef Cejka
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

/** @file
 * @brief Msim console driver.
 */

#include <async.h>
#include <ddf/driver.h>
#include <ddf/log.h>
#include <ddi.h>
#include <errno.h>
#include <ipc/char.h>
#include <sysinfo.h>

#include "msim-con.h"

static void msim_con_connection(ipc_callid_t, ipc_call_t *, void *);

static irq_pio_range_t msim_ranges[] = {
	{
		.base = 0,
		.size = 1
	}
};

static irq_cmd_t msim_cmds[] = {
	{
		.cmd = CMD_PIO_READ_8,
		.addr = (void *) 0,	/* will be patched in run-time */
		.dstarg = 2
	},
	{
		.cmd = CMD_ACCEPT
	}
};

static irq_code_t msim_kbd = {
	sizeof(msim_ranges) / sizeof(irq_pio_range_t),
	msim_ranges,
	sizeof(msim_cmds) / sizeof(irq_cmd_t),
	msim_cmds
};
#include <stdio.h>
static void msim_irq_handler(ipc_callid_t iid, ipc_call_t *call, void *arg)
{
	msim_con_t *con = (msim_con_t *) arg;
	uint8_t c;

	c = IPC_GET_ARG2(*call);
	printf("key=%d\n", c);
	if (con->client_sess != NULL) {
		async_exch_t *exch = async_exchange_begin(con->client_sess);
		async_msg_1(exch, CHAR_NOTIF_BYTE, c);
		async_exchange_end(exch);
	}
}

/** Add msim console device. */
int msim_con_add(msim_con_t *con)
{
	ddf_fun_t *fun = NULL;
	bool subscribed = false;
	int rc;

	printf("msim_con_add\n");
	fun = ddf_fun_create(con->dev, fun_exposed, "a");
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Error creating function 'a'.");
		rc = ENOMEM;
		goto error;
	}

	ddf_fun_set_conn_handler(fun, msim_con_connection);

	sysarg_t paddr;
	if (sysinfo_get_value("kbd.address.physical", &paddr) != EOK) {
		rc = ENOENT;
		goto error;
	}

	sysarg_t inr;
	if (sysinfo_get_value("kbd.inr", &inr) != EOK) {
		rc = ENOENT;
		goto error;
	}

	printf("msim_con_add: irq subscribe\n");

	msim_ranges[0].base = paddr;
	msim_cmds[0].addr = (void *) paddr;
	async_irq_subscribe(inr, msim_irq_handler, con, &msim_kbd);
	subscribed = true;

	printf("msim_con_add: bind\n");
	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error binding function 'a'.");
		goto error;
	}

	printf("msim_con_add: DONE\n");
	return EOK;
error:
	if (subscribed)
		async_irq_unsubscribe(inr);
	if (fun != NULL)
		ddf_fun_destroy(fun);

	return rc;
}

/** Remove msim console device */
int msim_con_remove(msim_con_t *con)
{
	return ENOTSUP;
}

/** Msim console device gone */
int msim_con_gone(msim_con_t *con)
{
	return ENOTSUP;
}

static void msim_con_putchar(msim_con_t *con, uint8_t ch)
{
}

/** Character device connection handler. */
static void msim_con_connection(ipc_callid_t iid, ipc_call_t *icall,
    void *arg)
{
	msim_con_t *con;

	/* Answer the IPC_M_CONNECT_ME_TO call. */
	async_answer_0(iid, EOK);

	printf("msim_con_connection\n");

	con = (msim_con_t *)ddf_dev_data_get(ddf_fun_get_dev((ddf_fun_t *)arg));

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
			printf("msim_con_connection: set client_sess\n");
		} else {
			switch (method) {
			case CHAR_WRITE_BYTE:
				ddf_msg(LVL_DEBUG, "Write %" PRIun " to device\n",
				    IPC_GET_ARG1(call));
				msim_con_putchar(con, (uint8_t) IPC_GET_ARG1(call));
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
