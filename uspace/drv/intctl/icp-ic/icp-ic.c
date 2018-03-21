/*
 * Copyright (c) 2014 Jiri Svoboda
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

/** @addtogroup icp-ic
 * @{
 */

/**
 * @file icp-ic.c
 * @brief IntegratorCP interrupt controller driver
 */

#include <async.h>
#include <bitops.h>
#include <ddi.h>
#include <ddf/log.h>
#include <errno.h>
#include <str_error.h>
#include <ipc/irc.h>
#include <stdint.h>

#include "icp-ic.h"
#include "icp-ic_hw.h"

enum {
	icpic_max_irq = 32
};

static errno_t icpic_enable_irq(icpic_t *icpic, sysarg_t irq)
{
	if (irq > icpic_max_irq)
		return EINVAL;

	ddf_msg(LVL_NOTE, "Enable IRQ %zu", irq);

	pio_write_32(&icpic->regs->irq_enableset, BIT_V(uint32_t, irq));
	return EOK;
}

/** Client connection handler.
 *
 * @param iid   Hash of the request that opened the connection.
 * @param icall Call data of the request that opened the connection.
 * @param arg	Local argument.
 */
static void icpic_connection(cap_call_handle_t icall_handle, ipc_call_t *icall, void *arg)
{
	cap_call_handle_t chandle;
	ipc_call_t call;
	icpic_t *icpic;

	/*
	 * Answer the first IPC_M_CONNECT_ME_TO call.
	 */
	async_answer_0(icall_handle, EOK);

	icpic = (icpic_t *)ddf_dev_data_get(ddf_fun_get_dev((ddf_fun_t *)arg));

	while (true) {
		chandle = async_get_call(&call);

		if (!IPC_GET_IMETHOD(call)) {
			/* The other side has hung up. */
			async_answer_0(chandle, EOK);
			return;
		}

		switch (IPC_GET_IMETHOD(call)) {
		case IRC_ENABLE_INTERRUPT:
			async_answer_0(chandle,
			    icpic_enable_irq(icpic, IPC_GET_ARG1(call)));
			break;
		case IRC_DISABLE_INTERRUPT:
			/* XXX TODO */
			async_answer_0(chandle, EOK);
			break;
		case IRC_CLEAR_INTERRUPT:
			/* Noop */
			async_answer_0(chandle, EOK);
			break;
		default:
			async_answer_0(chandle, EINVAL);
			break;
		}
	}
}

/** Add icp-ic device. */
errno_t icpic_add(icpic_t *icpic, icpic_res_t *res)
{
	ddf_fun_t *fun_a = NULL;
	void *regs;
	errno_t rc;

	rc = pio_enable((void *)res->base, sizeof(icpic_regs_t), &regs);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error enabling PIO");
		goto error;
	}

	icpic->regs = (icpic_regs_t *)regs;

	fun_a = ddf_fun_create(icpic->dev, fun_exposed, "a");
	if (fun_a == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating function 'a'.");
		rc = ENOMEM;
		goto error;
	}

	ddf_fun_set_conn_handler(fun_a, icpic_connection);

	rc = ddf_fun_bind(fun_a);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function 'a': %s", str_error(rc));
		goto error;
	}

	rc = ddf_fun_add_to_category(fun_a, "irc");
	if (rc != EOK)
		goto error;

	return EOK;
error:
	if (fun_a != NULL)
		ddf_fun_destroy(fun_a);
	return rc;
}

/** Remove icp-ic device */
errno_t icpic_remove(icpic_t *icpic)
{
	return ENOTSUP;
}

/** icp-ic device gone */
errno_t icpic_gone(icpic_t *icpic)
{
	return ENOTSUP;
}

/**
 * @}
 */
