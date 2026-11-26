/*
 * Copyright (c) 2026 Jakub Kodym
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

/** @addtogroup uspace_drv_gicv3
 * @{
 */
/** @file GICv3 driver.
 * @brief ARM Generic Interrupt Controller, Architecture version 3.0, 64-bit.
 */

#include <errno.h>
#include <macros.h>
#include <ipc/irc.h>
#include <ddf/log.h>
#include <str_error.h>

#include "gicv3.h"
#include "tfa/gicv3.h"
#include "tfa/gicv3_private.h"

#define GICV3_DISTR_SIZE 0x00010000U
#define GICV3_REDISTR_SIZE 0x00080000U

/** Enable a GICv3 interrupt. */
static errno_t gicv3_enable_irq(gicv3_t *gicv3, sysarg_t irq)
{
	if (irq > gicv3->max_irq)
		return EINVAL;

	ddf_msg(LVL_NOTE, "Enable interrupt '%" PRIun "'.", irq);

	gicv3_enable_interrupt(irq, (uintptr_t)gicv3->distr,
	    (uintptr_t)gicv3->redistr);

	return EOK;
}

/** Client connection handler.
 *
 * @param icall Call data of the request that opened the connection.
 * @param arg   Local argument.
 */
static void gicv3_connection(ipc_call_t *icall, void *arg)
{
	ipc_call_t call;
	gicv3_t *gicv3;

	ddf_msg(LVL_NOTE, "GICv3 connection handler called.");
	/*
	 * Answer the first IPC_M_CONNECT_ME_TO call.
	 */
	async_accept_0(icall);

	gicv3 = (gicv3_t *)ddf_dev_data_get(ddf_fun_get_dev((ddf_fun_t *)arg));

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
			    gicv3_enable_irq(gicv3, ipc_get_arg1(&call)));
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

/** Add a GICv3 device. */
errno_t gicv3_add(gicv3_t *gicv3, gicv3_res_t *res)
{
	ddf_msg(LVL_NOTE, "Adding GICv3 device.");

	ddf_fun_t *fun_a = NULL;
	errno_t rc;

	rc = pio_enable((void *) res->distr_base, GICV3_DISTR_SIZE, (void **)
	    &gicv3->distr);
	if (rc != EOK) {
		ddf_msg(
		    LVL_ERROR, "Error enabling PIO for distributor registers.");
		goto error;
	}

	rc = pio_enable((void *) res->redistr_base, GICV3_REDISTR_SIZE,
	    (void **)&gicv3->redistr);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR,
		    "Error enabling PIO for CPU interface registers.");
		goto error;
	}

	fun_a = ddf_fun_create(gicv3->dev, fun_exposed, "a");
	if (fun_a == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating function 'a'.");
		rc = ENOMEM;
		goto error;
	}

	ddf_fun_set_conn_handler(fun_a, gicv3_connection);

	rc = ddf_fun_bind(fun_a);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function 'a': %s",
		    str_error(rc));
		goto error;
	}

	rc = ddf_fun_add_to_category(fun_a, "irc");
	if (rc != EOK)
		goto error;

	gicv3->max_irq = gicv3_get_spi_limit((uintptr_t)gicv3->distr) - 1;

	return EOK;
error:
	if (fun_a != NULL)
		ddf_fun_destroy(fun_a);
	return rc;
}

/** Remove a GICv3 device. */
errno_t gicv3_remove(gicv3_t *gicv3)
{
	return ENOTSUP;
}

/** A GICv3 device gone. */
errno_t gicv3_gone(gicv3_t *gicv3)
{
	return ENOTSUP;
}

/** @}
 */
