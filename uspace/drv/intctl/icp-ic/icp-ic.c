/*
 * SPDX-FileCopyrightText: 2014 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
 * @param icall Call data of the request that opened the connection.
 * @param arg   Local argument.
 *
 */
static void icpic_connection(ipc_call_t *icall, void *arg)
{
	ipc_call_t call;
	icpic_t *icpic;

	/*
	 * Answer the first IPC_M_CONNECT_ME_TO call.
	 */
	async_accept_0(icall);

	icpic = (icpic_t *) ddf_dev_data_get(ddf_fun_get_dev((ddf_fun_t *) arg));

	while (true) {
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			/* The other side has hung up. */
			async_answer_0(&call, EOK);
			return;
		}

		switch (ipc_get_imethod(&call)) {
		case IRC_ENABLE_INTERRUPT:
			async_answer_0(&call,
			    icpic_enable_irq(icpic, ipc_get_arg1(&call)));
			break;
		case IRC_DISABLE_INTERRUPT:
			/* XXX TODO */
			async_answer_0(&call, EOK);
			break;
		case IRC_CLEAR_INTERRUPT:
			/* Noop */
			async_answer_0(&call, EOK);
			break;
		default:
			async_answer_0(&call, EINVAL);
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
	bool bound = false;

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
	if (bound)
		ddf_fun_unbind(fun_a);
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
