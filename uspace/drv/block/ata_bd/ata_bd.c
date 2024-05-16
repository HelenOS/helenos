/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup ata_bd
 * @{
 */

/**
 * @file
 * @brief ISA ATA driver
 *
 * The driver services a single IDE channel.
 */

#include <ddi.h>
#include <ddf/interrupt.h>
#include <ddf/log.h>
#include <async.h>
#include <as.h>
#include <fibril_synch.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <str.h>
#include <inttypes.h>
#include <errno.h>

#include "ata_bd.h"
#include "main.h"

#define NAME       "ata_bd"

static errno_t ata_bd_init_io(ata_ctrl_t *ctrl);
static void ata_bd_fini_io(ata_ctrl_t *ctrl);
static errno_t ata_bd_init_irq(ata_ctrl_t *ctrl);
static void ata_bd_fini_irq(ata_ctrl_t *ctrl);
static void ata_irq_handler(ipc_call_t *call, ddf_dev_t *dev);

static void ata_write_data_16(void *, uint16_t *, size_t);
static void ata_read_data_16(void *, uint16_t *, size_t);
static void ata_write_cmd_8(void *, uint16_t, uint8_t);
static uint8_t ata_read_cmd_8(void *, uint16_t);
static void ata_write_ctl_8(void *, uint16_t, uint8_t);
static uint8_t ata_read_ctl_8(void *, uint16_t);
static errno_t ata_irq_enable(void *);
static errno_t ata_irq_disable(void *);
static errno_t ata_add_device(void *, unsigned, void *);
static errno_t ata_remove_device(void *, unsigned);
static void ata_msg_debug(void *, char *);
static void ata_msg_note(void *, char *);
static void ata_msg_warn(void *, char *);
static void ata_msg_error(void *, char *);

static const irq_pio_range_t ata_irq_ranges[] = {
	{
		.base = 0,
		.size = sizeof(ata_cmd_t)
	}
};

/** ATA interrupt pseudo code. */
static const irq_cmd_t ata_irq_cmds[] = {
	{
		.cmd = CMD_PIO_READ_8,
		.addr = NULL,  /* will be patched in run-time */
		.dstarg = 1
	},
	{
		.cmd = CMD_ACCEPT
	}
};

/** Initialize ATA controller. */
errno_t ata_ctrl_init(ata_ctrl_t *ctrl, ata_hwres_t *res)
{
	errno_t rc;
	bool irq_inited = false;
	ata_params_t params;

	ddf_msg(LVL_DEBUG, "ata_ctrl_init()");

	fibril_mutex_initialize(&ctrl->lock);
	ctrl->cmd_physical = res->cmd;
	ctrl->ctl_physical = res->ctl;
	ctrl->irq = res->irq;

	ddf_msg(LVL_NOTE, "I/O address %p/%p", (void *) ctrl->cmd_physical,
	    (void *) ctrl->ctl_physical);

	ddf_msg(LVL_DEBUG, "Init I/O");
	rc = ata_bd_init_io(ctrl);
	if (rc != EOK)
		return rc;

	ddf_msg(LVL_DEBUG, "Init IRQ");
	rc = ata_bd_init_irq(ctrl);
	if (rc != EOK) {
		ddf_msg(LVL_NOTE, "init IRQ failed");
		return rc;
	}

	irq_inited = true;

	ddf_msg(LVL_DEBUG, "ata_ctrl_init(): Initialize ATA channel");

	params.arg = (void *)ctrl;
	params.have_irq = (ctrl->irq >= 0) ? true : false;
	params.write_data_16 = ata_write_data_16;
	params.read_data_16 = ata_read_data_16;
	params.write_cmd_8 = ata_write_cmd_8;
	params.read_cmd_8 = ata_read_cmd_8;
	params.write_ctl_8 = ata_write_ctl_8;
	params.read_ctl_8 = ata_read_ctl_8;
	params.irq_enable = ata_irq_enable;
	params.irq_disable = ata_irq_disable;
	params.add_device = ata_add_device;
	params.remove_device = ata_remove_device;
	params.msg_debug = ata_msg_debug;
	params.msg_note = ata_msg_note;
	params.msg_warn = ata_msg_warn;
	params.msg_error = ata_msg_error;

	rc = ata_channel_create(&params, &ctrl->channel);
	if (rc != EOK)
		goto error;

	rc = ata_channel_initialize(ctrl->channel);
	if (rc != EOK)
		goto error;

	ddf_msg(LVL_DEBUG, "ata_ctrl_init: DONE");
	return EOK;
error:
	if (irq_inited)
		ata_bd_fini_irq(ctrl);
	ata_bd_fini_io(ctrl);
	return rc;
}

/** Remove ATA controller. */
errno_t ata_ctrl_remove(ata_ctrl_t *ctrl)
{
	errno_t rc;

	ddf_msg(LVL_DEBUG, ": ata_ctrl_remove()");

	fibril_mutex_lock(&ctrl->lock);

	rc = ata_channel_destroy(ctrl->channel);
	if (rc != EOK) {
		fibril_mutex_unlock(&ctrl->lock);
		return rc;
	}

	ata_bd_fini_irq(ctrl);
	ata_bd_fini_io(ctrl);
	fibril_mutex_unlock(&ctrl->lock);

	return EOK;
}

/** Surprise removal of ATA controller. */
errno_t ata_ctrl_gone(ata_ctrl_t *ctrl)
{
	errno_t rc;

	ddf_msg(LVL_DEBUG, "ata_ctrl_gone()");

	fibril_mutex_lock(&ctrl->lock);

	rc = ata_channel_destroy(ctrl->channel);
	if (rc != EOK) {
		fibril_mutex_unlock(&ctrl->lock);
		return rc;
	}

	ata_bd_fini_io(ctrl);
	fibril_mutex_unlock(&ctrl->lock);

	return EOK;
}

/** Enable device I/O. */
static errno_t ata_bd_init_io(ata_ctrl_t *ctrl)
{
	errno_t rc;
	void *vaddr;

	rc = pio_enable((void *) ctrl->cmd_physical, sizeof(ata_cmd_t), &vaddr);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Cannot initialize device I/O space.");
		return rc;
	}

	ctrl->cmd = vaddr;

	rc = pio_enable((void *) ctrl->ctl_physical, sizeof(ata_ctl_t), &vaddr);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Cannot initialize device I/O space.");
		return rc;
	}

	ctrl->ctl = vaddr;
	return EOK;
}

/** Clean up device I/O. */
static void ata_bd_fini_io(ata_ctrl_t *ctrl)
{
	(void) ctrl;
	/* XXX TODO */
}

/** Initialize IRQ. */
static errno_t ata_bd_init_irq(ata_ctrl_t *ctrl)
{
	irq_code_t irq_code;
	irq_pio_range_t *ranges;
	irq_cmd_t *cmds;
	errno_t rc;

	if (ctrl->irq < 0)
		return EOK;

	ranges = malloc(sizeof(ata_irq_ranges));
	if (ranges == NULL)
		return ENOMEM;

	cmds = malloc(sizeof(ata_irq_cmds));
	if (cmds == NULL) {
		free(cmds);
		return ENOMEM;
	}

	memcpy(ranges, &ata_irq_ranges, sizeof(ata_irq_ranges));
	ranges[0].base = ctrl->cmd_physical;
	memcpy(cmds, &ata_irq_cmds, sizeof(ata_irq_cmds));
	cmds[0].addr = &ctrl->cmd->status;

	irq_code.rangecount = sizeof(ata_irq_ranges) / sizeof(irq_pio_range_t);
	irq_code.ranges = ranges;
	irq_code.cmdcount = sizeof(ata_irq_cmds) / sizeof(irq_cmd_t);
	irq_code.cmds = cmds;

	ddf_msg(LVL_NOTE, "IRQ %d", ctrl->irq);
	rc = register_interrupt_handler(ctrl->dev, ctrl->irq, ata_irq_handler,
	    &irq_code, &ctrl->ihandle);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error registering IRQ.");
		goto error;
	}

	ddf_msg(LVL_DEBUG, "Interrupt handler registered");
	free(ranges);
	free(cmds);
	return EOK;
error:
	free(ranges);
	free(cmds);
	return rc;
}

/** Clean up IRQ. */
static void ata_bd_fini_irq(ata_ctrl_t *ctrl)
{
	errno_t rc;
	async_sess_t *parent_sess;

	parent_sess = ddf_dev_parent_sess_get(ctrl->dev);

	rc = hw_res_disable_interrupt(parent_sess, ctrl->irq);
	if (rc != EOK)
		ddf_msg(LVL_ERROR, "Error disabling IRQ.");

	(void) unregister_interrupt_handler(ctrl->dev, ctrl->ihandle);
}

/** Interrupt handler.
 *
 * @param call Call data
 * @param dev Device that caused the interrupt
 */
static void ata_irq_handler(ipc_call_t *call, ddf_dev_t *dev)
{
	ata_ctrl_t *ctrl = (ata_ctrl_t *)ddf_dev_data_get(dev);
	uint8_t status;
	async_sess_t *parent_sess;

	status = ipc_get_arg1(call);
	ata_channel_irq(ctrl->channel, status);

	parent_sess = ddf_dev_parent_sess_get(dev);
	hw_res_clear_interrupt(parent_sess, ctrl->irq);
}

/** Write the data register callback handler.
 *
 * @param arg Argument (ata_ctrl_t *)
 * @param data Data
 * @param nwords Number of words to write
 */
static void ata_write_data_16(void *arg, uint16_t *data, size_t nwords)
{
	ata_ctrl_t *ctrl = (ata_ctrl_t *)arg;
	size_t i;

	for (i = 0; i < nwords; i++)
		pio_write_16(&ctrl->cmd->data_port, data[i]);
}

/** Read the data register callback handler.
 *
 * @param arg Argument (ata_ctrl_t *)
 * @param buf Destination buffer
 * @param nwords Number of words to read
 */
static void ata_read_data_16(void *arg, uint16_t *buf, size_t nwords)
{
	ata_ctrl_t *ctrl = (ata_ctrl_t *)arg;
	size_t i;

	for (i = 0; i < nwords; i++)
		buf[i] = pio_read_16(&ctrl->cmd->data_port);
}

/** Write command register callback handler.
 *
 * @param arg Argument (ata_ctrl_t *)
 * @param off Register offset
 * @param value Value to write to command register
 */
static void ata_write_cmd_8(void *arg, uint16_t off, uint8_t value)
{
	ata_ctrl_t *ctrl = (ata_ctrl_t *)arg;

	pio_write_8(((ioport8_t *)ctrl->cmd) + off, value);
}

/** Read command register callback handler.
 *
 * @param arg Argument (ata_ctrl_t *)
 * @param off Register offset
 * @return value Value read from command register
 */
static uint8_t ata_read_cmd_8(void *arg, uint16_t off)
{
	ata_ctrl_t *ctrl = (ata_ctrl_t *)arg;

	return pio_read_8(((ioport8_t *)ctrl->cmd) + off);
}

/** Write control register callback handler.
 *
 * @param arg Argument (ata_ctrl_t *)
 * @param off Register offset
 * @param value Value to write to control register
 */
static void ata_write_ctl_8(void *arg, uint16_t off, uint8_t value)
{
	ata_ctrl_t *ctrl = (ata_ctrl_t *)arg;

	pio_write_8(((ioport8_t *)ctrl->ctl) + off, value);
}

/** Read control register callback handler.
 *
 * @param arg Argument (ata_ctrl_t *)
 * @param off Register offset
 * @return value Value read from control register
 */
static uint8_t ata_read_ctl_8(void *arg, uint16_t off)
{
	ata_ctrl_t *ctrl = (ata_ctrl_t *)arg;

	return pio_read_8(((ioport8_t *)ctrl->ctl) + off);
}

/** Enable IRQ callback handler
 *
 * @param arg Argument (ata_ctrl_t *)
 * @return EOK on success or an error code
 */
static errno_t ata_irq_enable(void *arg)
{
	ata_ctrl_t *ctrl = (ata_ctrl_t *)arg;
	async_sess_t *parent_sess;
	errno_t rc;

	ddf_msg(LVL_DEBUG, "Enable IRQ");

	parent_sess = ddf_dev_parent_sess_get(ctrl->dev);

	rc = hw_res_enable_interrupt(parent_sess, ctrl->irq);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error enabling IRQ.");
		(void) unregister_interrupt_handler(ctrl->dev,
		    ctrl->ihandle);
		return rc;
	}

	return EOK;
}

/** Disable IRQ callback handler
 *
 * @param arg Argument (ata_ctrl_t *)
 * @return EOK on success or an error code
 */
static errno_t ata_irq_disable(void *arg)
{
	ata_ctrl_t *ctrl = (ata_ctrl_t *)arg;
	async_sess_t *parent_sess;
	errno_t rc;

	ddf_msg(LVL_DEBUG, "Disable IRQ");

	parent_sess = ddf_dev_parent_sess_get(ctrl->dev);

	rc = hw_res_disable_interrupt(parent_sess, ctrl->irq);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error enabling IRQ.");
		(void) unregister_interrupt_handler(ctrl->dev,
		    ctrl->ihandle);
		return rc;
	}

	return EOK;
}

/** Add ATA device callback handler.
 *
 * @param arg Argument (ata_ctrl_t *)
 * @param idx Device index
 * $param charg Connection handler argument
 * @return EOK on success or an error code
 */
static errno_t ata_add_device(void *arg, unsigned idx, void *charg)
{
	ata_ctrl_t *ctrl = (ata_ctrl_t *)arg;
	return ata_fun_create(ctrl, idx, charg);
}

/** Remove ATA device callback handler.
 *
 * @param arg Argument (ata_ctrl_t *)
 * @param idx Device index
 * @return EOK on success or an error code
 */
static errno_t ata_remove_device(void *arg, unsigned idx)
{
	ata_ctrl_t *ctrl = (ata_ctrl_t *)arg;
	return ata_fun_remove(ctrl, idx);
}

/** Debug message callback handler.
 *
 * @param arg Argument (ata_ctrl_t *)
 * @param msg Message
 */
static void ata_msg_debug(void *arg, char *msg)
{
	(void)arg;
	ddf_msg(LVL_DEBUG, "%s", msg);
}

/** Notice message callback handler.
 *
 * @param arg Argument (ata_ctrl_t *)
 * @param msg Message
 */
static void ata_msg_note(void *arg, char *msg)
{
	(void)arg;
	ddf_msg(LVL_NOTE, "%s", msg);
}

/** Warning message callback handler.
 *
 * @param arg Argument (ata_ctrl_t *)
 * @param msg Message
 */
static void ata_msg_warn(void *arg, char *msg)
{
	(void)arg;
	ddf_msg(LVL_WARN, "%s", msg);
}

/** Error message callback handler.
 *
 * @param arg Argument (ata_ctrl_t *)
 * @param msg Message
 */
static void ata_msg_error(void *arg, char *msg)
{
	(void)arg;
	ddf_msg(LVL_ERROR, "%s", msg);
}

/**
 * @}
 */
