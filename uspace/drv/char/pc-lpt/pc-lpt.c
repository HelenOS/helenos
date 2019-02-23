/*
 * Copyright (c) 2018 Jiri Svoboda
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
 * @brief PC parallel port driver.
 */

#include <async.h>
#include <bitops.h>
#include <ddf/driver.h>
#include <ddf/log.h>
#include <ddi.h>
#include <errno.h>
#include <fibril_synch.h>
#include <io/chardev_srv.h>

#include "pc-lpt.h"
#include "pc-lpt_hw.h"

static void pc_lpt_connection(ipc_call_t *, void *);

static errno_t pc_lpt_read(chardev_srv_t *, void *, size_t, size_t *,
    chardev_flags_t);
static errno_t pc_lpt_write(chardev_srv_t *, const void *, size_t, size_t *);

static chardev_ops_t pc_lpt_chardev_ops = {
	.read = pc_lpt_read,
	.write = pc_lpt_write
};

static irq_cmd_t pc_lpt_cmds_proto[] = {
	{
		.cmd = CMD_DECLINE
	}
};

/** PC LPT IRQ handler.
 *
 * Note that while the standard PC parallel port supports IRQ, it seems
 * drivers tend to avoid using them (for a reason?) These IRQs tend
 * to be used by other HW as well (Sound Blaster) so caution is in order.
 * Also not sure, if/how the IRQ needs to be cleared.
 *
 * Currently we don't enable IRQ and don't handle it in any way.
 */
static void pc_lpt_irq_handler(ipc_call_t *call, void *arg)
{
	pc_lpt_t *lpt = (pc_lpt_t *) arg;

	(void) lpt;
}

/** Add pc-lpt device. */
errno_t pc_lpt_add(pc_lpt_t *lpt, pc_lpt_res_t *res)
{
	ddf_fun_t *fun = NULL;
	bool bound = false;
	irq_cmd_t *pc_lpt_cmds = NULL;
	uint8_t control;
	uint8_t r;
	errno_t rc;

	lpt->irq_handle = CAP_NIL;
	fibril_mutex_initialize(&lpt->hw_lock);

	pc_lpt_cmds = malloc(sizeof(pc_lpt_cmds_proto));
	if (pc_lpt_cmds == NULL) {
		rc = ENOMEM;
		goto error;
	}

	lpt->res = *res;

	fun = ddf_fun_create(lpt->dev, fun_exposed, "a");
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Error creating function 'a'.");
		rc = ENOMEM;
		goto error;
	}

	rc = pio_enable((void *)res->base, sizeof(pc_lpt_regs_t),
	    (void **) &lpt->regs);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error enabling I/O");
		goto error;
	}

	ddf_fun_set_conn_handler(fun, pc_lpt_connection);

	lpt->irq_range[0].base = res->base;
	lpt->irq_range[0].size = 1;

	memcpy(pc_lpt_cmds, pc_lpt_cmds_proto, sizeof(pc_lpt_cmds_proto));
	pc_lpt_cmds[0].addr = (void *) res->base;

	lpt->irq_code.rangecount = 1;
	lpt->irq_code.ranges = lpt->irq_range;
	lpt->irq_code.cmdcount = sizeof(pc_lpt_cmds_proto) / sizeof(irq_cmd_t);
	lpt->irq_code.cmds = pc_lpt_cmds;

	rc = async_irq_subscribe(res->irq, pc_lpt_irq_handler, lpt,
	    &lpt->irq_code, &lpt->irq_handle);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error registering IRQ code.");
		goto error;
	}

	control = BIT_V(uint8_t, ctl_select) | BIT_V(uint8_t, ctl_ninit);
	pio_write_8(&lpt->regs->control, control);
	r = pio_read_8(&lpt->regs->control);
	if ((r & 0xf) != control) {
		/* Device not present */
		rc = EIO;
		goto error;
	}

	control |= BIT_V(uint8_t, ctl_autofd);
	pio_write_8(&lpt->regs->control, control);
	r = pio_read_8(&lpt->regs->control);
	if ((r & 0xf) != control) {
		/* Device not present */
		rc = EIO;
		goto error;
	}

	control &= ~BIT_V(uint8_t, ctl_autofd);
	pio_write_8(&lpt->regs->control, control);

	chardev_srvs_init(&lpt->cds);
	lpt->cds.ops = &pc_lpt_chardev_ops;
	lpt->cds.sarg = lpt;

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error binding function 'a'.");
		goto error;
	}

	bound = true;

	rc = ddf_fun_add_to_category(fun, "printer-port");
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error adding function 'a' to category "
		    "'printer-port'.");
		goto error;
	}

	return EOK;
error:
	if (cap_handle_valid(lpt->irq_handle))
		async_irq_unsubscribe(lpt->irq_handle);
	if (bound)
		ddf_fun_unbind(fun);
	if (fun != NULL)
		ddf_fun_destroy(fun);
	free(pc_lpt_cmds);

	return rc;
}

/** Remove pc-lpt device */
errno_t pc_lpt_remove(pc_lpt_t *lpt)
{
	return ENOTSUP;
}

/** Pc-lpt device gone */
errno_t pc_lpt_gone(pc_lpt_t *lpt)
{
	return ENOTSUP;
}

/** Write a single byte to the parallel port.
 *
 * @param lpt Parallel port (locked)
 * @param ch Byte
 */
static void pc_lpt_putchar(pc_lpt_t *lpt, uint8_t ch)
{
	uint8_t status;
	uint8_t control;

	assert(fibril_mutex_is_locked(&lpt->hw_lock));

	/* Write data */
	pio_write_8(&lpt->regs->data, ch);

	/* Wait for S7/nbusy to become 1 */
	do {
		status = pio_read_8(&lpt->regs->status);
		// FIXME Need to time out with an error after a while
	} while ((status & BIT_V(uint8_t, sts_nbusy)) == 0);

	control = pio_read_8(&lpt->regs->control);
	pio_write_8(&lpt->regs->control, control | BIT_V(uint8_t, ctl_strobe));
	fibril_usleep(5);
	pio_write_8(&lpt->regs->control, control & ~BIT_V(uint8_t, ctl_strobe));
}

/** Read from pc-lpt device */
static errno_t pc_lpt_read(chardev_srv_t *srv, void *buf, size_t size,
    size_t *nread, chardev_flags_t flags)
{
	pc_lpt_t *lpt = (pc_lpt_t *) srv->srvs->sarg;
	(void) lpt;
	return ENOTSUP;
}

/** Write to pc-lpt device */
static errno_t pc_lpt_write(chardev_srv_t *srv, const void *data, size_t size,
    size_t *nwr)
{
	pc_lpt_t *lpt = (pc_lpt_t *) srv->srvs->sarg;
	size_t i;
	uint8_t *dp = (uint8_t *) data;

	fibril_mutex_lock(&lpt->hw_lock);

	for (i = 0; i < size; i++)
		pc_lpt_putchar(lpt, dp[i]);

	fibril_mutex_unlock(&lpt->hw_lock);

	*nwr = size;
	return EOK;
}

/** Character device connection handler. */
static void pc_lpt_connection(ipc_call_t *icall, void *arg)
{
	pc_lpt_t *lpt = (pc_lpt_t *) ddf_dev_data_get(
	    ddf_fun_get_dev((ddf_fun_t *) arg));

	chardev_conn(icall, &lpt->cds);
}

/** @}
 */
