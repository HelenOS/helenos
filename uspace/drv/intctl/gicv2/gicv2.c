/*
 * Copyright (c) 2018 Petr Pavlu
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

/** @addtogroup uspace_drv_gicv2
 * @{
 */

/** @file
 */

#include <async.h>
#include <bitops.h>
#include <ddi.h>
#include <ddf/log.h>
#include <errno.h>
#include <macros.h>
#include <str_error.h>
#include <ipc/irc.h>
#include <stdint.h>

#include "gicv2.h"

/** GICv2 distributor register map. */
typedef struct {
	/** Distributor control register. */
	ioport32_t ctlr;
#define GICV2D_CTLR_ENABLE_FLAG  0x1

	/** Interrupt controller type register. */
	const ioport32_t typer;
#define GICV2D_TYPER_IT_LINES_NUMBER_SHIFT  0
#define GICV2D_TYPER_IT_LINES_NUMBER_MASK \
	(0x1f << GICV2D_TYPER_IT_LINES_NUMBER_SHIFT)

	/** Distributor implementer identification register. */
	const ioport32_t iidr;
	/** Reserved. */
	PADD32(5);
	/** Implementation defined registers. */
	ioport32_t impl[8];
	/** Reserved. */
	PADD32(16);
	/** Interrupt group registers. */
	ioport32_t igroupr[32];
	/** Interrupt set-enable registers. */
	ioport32_t isenabler[32];
	/** Interrupt clear-enable registers. */
	ioport32_t icenabler[32];
	/** Interrupt set-pending registers. */
	ioport32_t ispendr[32];
	/** Interrupt clear-pending registers. */
	ioport32_t icpendr[32];
	/** GICv2 interrupt set-active registers. */
	ioport32_t isactiver[32];
	/** Interrupt clear-active registers. */
	ioport32_t icactiver[32];
	/** Interrupt priority registers. */
	ioport32_t ipriorityr[255];
	/** Reserved. */
	PADD32(1);
	/** Interrupt processor target registers. First 8 words are read-only.
	 */
	ioport32_t itargetsr[255];
	/** Reserved. */
	PADD32(1);
	/** Interrupt configuration registers. */
	ioport32_t icfgr[64];
	/** Implementation defined registers. */
	ioport32_t impl2[64];
	/** Non-secure access control registers. */
	ioport32_t nsacr[64];
	/** Software generated interrupt register. */
	ioport32_t sgir;
	/** Reserved. */
	PADD32(3);
	/** SGI clear-pending registers. */
	ioport32_t cpendsgir[4];
	/** SGI set-pending registers. */
	ioport32_t spendsgir[4];
	/** Reserved. */
	PADD32(40);
	/** Implementation defined identification registers. */
	const ioport32_t impl3[12];
} gicv2_distr_regs_t;

/* GICv2 CPU interface register map. */
typedef struct {
	/** CPU interface control register. */
	ioport32_t ctlr;
#define GICV2C_CTLR_ENABLE_FLAG  0x1

	/** Interrupt priority mask register. */
	ioport32_t pmr;
	/** Binary point register. */
	ioport32_t bpr;
	/** Interrupt acknowledge register. */
	const ioport32_t iar;
#define GICV2C_IAR_INTERRUPT_ID_SHIFT  0
#define GICV2C_IAR_INTERRUPT_ID_MASK \
	(0x3ff << GICV2C_IAR_INTERRUPT_ID_SHIFT)
#define GICV2C_IAR_CPUID_SHIFT  10
#define GICV2C_IAR_CPUID_MASK \
	(0x7 << GICV2C_IAR_CPUID_SHIFT)

	/** End of interrupt register. */
	ioport32_t eoir;
	/** Running priority register. */
	const ioport32_t rpr;
	/** Highest priority pending interrupt register. */
	const ioport32_t hppir;
	/** Aliased binary point register. */
	ioport32_t abpr;
	/** Aliased interrupt acknowledge register. */
	const ioport32_t aiar;
	/** Aliased end of interrupt register. */
	ioport32_t aeoir;
	/** Aliased highest priority pending interrupt register. */
	const ioport32_t ahppir;
	/** Reserved. */
	PADD32(5);
	/** Implementation defined registers. */
	ioport32_t impl[36];
	/** Active priorities registers. */
	ioport32_t apr[4];
	/** Non-secure active priorities registers. */
	ioport32_t nsapr[4];
	/** Reserved. */
	PADD32(3);
	/** CPU interface identification register. */
	const ioport32_t iidr;
	/** Unallocated. */
	PADD32(960);
	/** Deactivate interrupt register. */
	ioport32_t dir;
} gicv2_cpui_regs_t;

static errno_t gicv2_enable_irq(gicv2_t *gicv2, sysarg_t irq)
{
	if (irq > gicv2->max_irq)
		return EINVAL;

	ddf_msg(LVL_NOTE, "Enable interrupt '%" PRIun "'.", irq);

	gicv2_distr_regs_t *distr = (gicv2_distr_regs_t *) gicv2->distr;
	pio_write_32(&distr->isenabler[irq / 32], BIT_V(uint32_t, irq % 32));
	return EOK;
}

/** Client connection handler.
 *
 * @param icall Call data of the request that opened the connection.
 * @param arg   Local argument.
 */
static void gicv2_connection(ipc_call_t *icall, void *arg)
{
	ipc_call_t call;
	gicv2_t *gicv2;

	/*
	 * Answer the first IPC_M_CONNECT_ME_TO call.
	 */
	async_answer_0(icall, EOK);

	gicv2 = (gicv2_t *)ddf_dev_data_get(ddf_fun_get_dev((ddf_fun_t *)arg));

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
			    gicv2_enable_irq(gicv2, ipc_get_arg1(&call)));
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

/** Add a GICv2 device. */
errno_t gicv2_add(gicv2_t *gicv2, gicv2_res_t *res)
{
	ddf_fun_t *fun_a = NULL;
	errno_t rc;

	rc = pio_enable((void *) res->distr_base, sizeof(gicv2_distr_regs_t),
	    &gicv2->distr);
	if (rc != EOK) {
		ddf_msg(
		    LVL_ERROR, "Error enabling PIO for distributor registers.");
		goto error;
	}

	rc = pio_enable(
	    (void *) res->cpui_base, sizeof(gicv2_cpui_regs_t), &gicv2->cpui);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR,
		    "Error enabling PIO for CPU interface registers.");
		goto error;
	}

	fun_a = ddf_fun_create(gicv2->dev, fun_exposed, "a");
	if (fun_a == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating function 'a'.");
		rc = ENOMEM;
		goto error;
	}

	ddf_fun_set_conn_handler(fun_a, gicv2_connection);

	rc = ddf_fun_bind(fun_a);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function 'a': %s",
		    str_error(rc));
		goto error;
	}

	rc = ddf_fun_add_to_category(fun_a, "irc");
	if (rc != EOK)
		goto error;

	/* Get maximum number of interrupts. */
	gicv2_distr_regs_t *distr = (gicv2_distr_regs_t *) gicv2->distr;
	uint32_t typer = pio_read_32(&distr->typer);
	gicv2->max_irq = (((typer & GICV2D_TYPER_IT_LINES_NUMBER_MASK) >>
	    GICV2D_TYPER_IT_LINES_NUMBER_SHIFT) + 1) * 32;

	return EOK;
error:
	if (fun_a != NULL)
		ddf_fun_destroy(fun_a);
	return rc;
}

/** Remove a GICv2 device. */
errno_t gicv2_remove(gicv2_t *gicv2)
{
	return ENOTSUP;
}

/** A GICv2 device gone. */
errno_t gicv2_gone(gicv2_t *gicv2)
{
	return ENOTSUP;
}

/** @}
 */
