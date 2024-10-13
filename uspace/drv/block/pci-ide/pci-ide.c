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

/** @addtogroup pci-ide
 * @{
 */

/**
 * @file
 * @brief PCI IDE driver
 */

#include <assert.h>
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

#include "pci-ide.h"
#include "main.h"

static errno_t pci_ide_init_io(pci_ide_channel_t *);
static void pci_ide_fini_io(pci_ide_channel_t *);
static errno_t pci_ide_init_irq(pci_ide_channel_t *);
static void pci_ide_fini_irq(pci_ide_channel_t *);
static void pci_ide_irq_handler(ipc_call_t *, void *);

static void pci_ide_write_data_16(void *, uint16_t *, size_t);
static void pci_ide_read_data_16(void *, uint16_t *, size_t);
static void pci_ide_write_cmd_8(void *, uint16_t, uint8_t);
static uint8_t pci_ide_read_cmd_8(void *, uint16_t);
static void pci_ide_write_ctl_8(void *, uint16_t, uint8_t);
static uint8_t pci_ide_read_ctl_8(void *, uint16_t);
static errno_t pci_ide_irq_enable(void *);
static errno_t pci_ide_irq_disable(void *);
static void pci_ide_dma_chan_setup(void *, void *, size_t, ata_dma_dir_t);
static void pci_ide_dma_chan_teardown(void *);
static errno_t pci_ide_add_device(void *, unsigned, void *);
static errno_t pci_ide_remove_device(void *, unsigned);
static void pci_ide_msg_debug(void *, char *);
static void pci_ide_msg_note(void *, char *);
static void pci_ide_msg_warn(void *, char *);
static void pci_ide_msg_error(void *, char *);

static const irq_pio_range_t pci_ide_irq_ranges[] = {
	{
		.base = 0,
		.size = sizeof(ata_cmd_t)
	},
	{
		.base = 0,
		.size = sizeof(pci_ide_regs_t)
	}
};

/** IDE interrupt pseudo code. */
static const irq_cmd_t pci_ide_irq_cmds[] = {
	/* [0] Read BMISX */
	{
		.cmd = CMD_PIO_READ_8,
		.addr = NULL,  /* will be patched in run-time */
		.dstarg = 1
	},
	/* [1] Test BMISX.IDEINTS */
	{
		.cmd = CMD_AND,
		.value = bmisx_ideints,
		.srcarg = 1,
		.dstarg = 2
	},
	/* [2] if (BMISX.IDEINTS != 0) { */
	{
		.cmd = CMD_PREDICATE,
		.srcarg = 2,
		.value = 3
	},
	/*
	 * [3] Clear BMISX.IDEINTS by writing 1 to it. This clears bits 6,5,
	 * but that should not matter.
	 */
	{
		.cmd = CMD_PIO_WRITE_8,
		.addr = NULL,  /* will be patched in run-time */
		.value = bmisx_ideints
	},
	/* [4] Read IDE status register */
	{
		.cmd = CMD_PIO_READ_8,
		.addr = NULL,  /* will be patched in run-time */
		.dstarg = 1
	},
	/* [5] Accept */
	{
		.cmd = CMD_ACCEPT
	},
	/* [6] } else { Decline */
	{
		.cmd = CMD_DECLINE
	}
	/* } */
};

/** Initialize PCI IDE controller.
 *
 * @param ctrl PCI IDE controller
 * @param res Hardware resources
 *
 * @return EOK on success or an error code
 */
errno_t pci_ide_ctrl_init(pci_ide_ctrl_t *ctrl, pci_ide_hwres_t *res)
{
	errno_t rc;
	void *vaddr;

	ddf_msg(LVL_DEBUG, "pci_ide_ctrl_init()");
	ctrl->bmregs_physical = res->bmregs;

	ddf_msg(LVL_NOTE, "Bus master IDE regs I/O address: 0x%" PRIxPTR,
	    ctrl->bmregs_physical);

	rc = pio_enable((void *)ctrl->bmregs_physical, sizeof(pci_ide_regs_t),
	    &vaddr);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Cannot initialize device I/O space.");
		return rc;
	}

	ctrl->bmregs = vaddr;

	ddf_msg(LVL_DEBUG, "pci_ide_ctrl_init: DONE");
	return EOK;
}

/** Finalize PCI IDE controller.
 *
 * @param ctrl PCI IDE controller
 * @return EOK on success or an error code
 */
errno_t pci_ide_ctrl_fini(pci_ide_ctrl_t *ctrl)
{
	ddf_msg(LVL_DEBUG, ": pci_ide_ctrl_fini()");

	// XXX TODO
	return EOK;
}

/** Initialize PCI IDE channel. */
errno_t pci_ide_channel_init(pci_ide_ctrl_t *ctrl, pci_ide_channel_t *chan,
    unsigned chan_id, pci_ide_hwres_t *res)
{
	errno_t rc;
	bool irq_inited = false;
	ata_params_t params;
	void *buffer = NULL;

	ddf_msg(LVL_DEBUG, "pci_ide_channel_init()");

	chan->ctrl = ctrl;
	chan->chan_id = chan_id;
	fibril_mutex_initialize(&chan->lock);
	if (chan_id == 0) {
		chan->cmd_physical = res->cmd1;
		chan->ctl_physical = res->ctl1;
		chan->irq = res->irq1;
	} else {
		chan->cmd_physical = res->cmd2;
		chan->ctl_physical = res->ctl2;
		chan->irq = res->irq2;
	}

	chan->dma_buf_size = 8192;

	ddf_msg(LVL_NOTE, "I/O address %p/%p", (void *) chan->cmd_physical,
	    (void *) chan->ctl_physical);

	ddf_msg(LVL_DEBUG, "Init I/O");
	rc = pci_ide_init_io(chan);
	if (rc != EOK)
		return rc;

	ddf_msg(LVL_DEBUG, "Init IRQ");
	rc = pci_ide_init_irq(chan);
	if (rc != EOK) {
		ddf_msg(LVL_NOTE, "init IRQ failed");
		goto error;
	}

	irq_inited = true;

	ddf_msg(LVL_DEBUG, "Allocate PRD table");

	buffer = AS_AREA_ANY;
	rc = dmamem_map_anonymous(sizeof (pci_ide_prd_t), DMAMEM_4GiB | 0xffff,
	    AS_AREA_WRITE | AS_AREA_READ, 0, &chan->prdt_pa, &buffer);
	if (rc != EOK) {
		ddf_msg(LVL_NOTE, "Failed allocating PRD table.");
		goto error;
	}

	chan->prdt = (pci_ide_prd_t *)buffer;

	ddf_msg(LVL_DEBUG, "Allocate DMA buffer");

	buffer = AS_AREA_ANY;
	rc = dmamem_map_anonymous(chan->dma_buf_size, DMAMEM_4GiB | 0xffff,
	    AS_AREA_WRITE | AS_AREA_READ, 0, &chan->dma_buf_pa, &buffer);
	if (rc != EOK) {
		ddf_msg(LVL_NOTE, "Failed allocating PRD table.");
		goto error;
	}

	chan->dma_buf = buffer;

	/* Populate PRD with information on our single DMA buffer */
	chan->prdt->pba = host2uint32_t_le(chan->dma_buf_pa);
	chan->prdt->bcnt = host2uint16_t_le(chan->dma_buf_size);
	chan->prdt->eot_res = host2uint16_t_le(pci_ide_prd_eot);

	/* Program PRD table pointer register */
	if (chan_id == 0) {
		pio_write_32(&chan->ctrl->bmregs->bmidtpp,
		    (uint32_t)chan->prdt_pa);
	} else {
		pio_write_32(&chan->ctrl->bmregs->bmidtps,
		    (uint32_t)chan->prdt_pa);
	}

	ddf_msg(LVL_DEBUG, "pci_ide_channel_init(): Initialize IDE channel");

	params.arg = (void *)chan;
	params.have_irq = (chan->irq >= 0) ? true : false;
	params.use_dma = true;
	params.max_dma_xfer = chan->dma_buf_size;
	params.write_data_16 = pci_ide_write_data_16;
	params.read_data_16 = pci_ide_read_data_16;
	params.write_cmd_8 = pci_ide_write_cmd_8;
	params.read_cmd_8 = pci_ide_read_cmd_8;
	params.write_ctl_8 = pci_ide_write_ctl_8;
	params.read_ctl_8 = pci_ide_read_ctl_8;
	params.irq_enable = pci_ide_irq_enable;
	params.irq_disable = pci_ide_irq_disable;
	params.dma_chan_setup = pci_ide_dma_chan_setup;
	params.dma_chan_teardown = pci_ide_dma_chan_teardown;
	params.add_device = pci_ide_add_device;
	params.remove_device = pci_ide_remove_device;
	params.msg_debug = pci_ide_msg_debug;
	params.msg_note = pci_ide_msg_note;
	params.msg_warn = pci_ide_msg_warn;
	params.msg_error = pci_ide_msg_error;

	rc = ata_channel_create(&params, &chan->channel);
	if (rc != EOK)
		goto error;

	rc = ata_channel_initialize(chan->channel);
	if (rc != EOK)
		goto error;

	ddf_msg(LVL_DEBUG, "pci_ide_channel_init: DONE");
	return EOK;
error:
	if (chan->channel != NULL) {
		(void) ata_channel_destroy(chan->channel);
		chan->channel = NULL;
	}
	if (buffer != NULL)
		dmamem_unmap_anonymous(buffer);
	if (irq_inited)
		pci_ide_fini_irq(chan);
	pci_ide_fini_io(chan);
	return rc;
}

/** Finalize PCI IDE channel. */
errno_t pci_ide_channel_fini(pci_ide_channel_t *chan)
{
	errno_t rc;

	ddf_msg(LVL_DEBUG, ": pci_ide_channel_fini()");

	fibril_mutex_lock(&chan->lock);

	rc = ata_channel_destroy(chan->channel);
	if (rc != EOK) {
		fibril_mutex_unlock(&chan->lock);
		return rc;
	}

	dmamem_unmap_anonymous(chan->dma_buf);
	pci_ide_fini_irq(chan);
	pci_ide_fini_io(chan);
	fibril_mutex_unlock(&chan->lock);

	return EOK;
}

/** Enable device I/O. */
static errno_t pci_ide_init_io(pci_ide_channel_t *chan)
{
	errno_t rc;
	void *vaddr;

	rc = pio_enable((void *) chan->cmd_physical, sizeof(ata_cmd_t), &vaddr);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Cannot initialize device I/O space.");
		return rc;
	}

	chan->cmd = vaddr;

	rc = pio_enable((void *) chan->ctl_physical, sizeof(ata_ctl_t), &vaddr);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Cannot initialize device I/O space.");
		return rc;
	}

	chan->ctl = vaddr;

	rc = pio_enable((void *) chan->cmd_physical, sizeof(ata_cmd_t), &vaddr);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Cannot initialize device I/O space.");
		return rc;
	}

	chan->cmd = vaddr;

	return EOK;
}

/** Clean up device I/O. */
static void pci_ide_fini_io(pci_ide_channel_t *chan)
{
	(void) chan;
	/* XXX TODO */
}

/** Initialize IRQ. */
static errno_t pci_ide_init_irq(pci_ide_channel_t *chan)
{
	irq_code_t irq_code;
	irq_pio_range_t *ranges;
	irq_cmd_t *cmds;
	uint8_t *bmisx;
	errno_t rc;

	if (chan->irq < 0)
		return EOK;

	ranges = malloc(sizeof(pci_ide_irq_ranges));
	if (ranges == NULL)
		return ENOMEM;

	cmds = malloc(sizeof(pci_ide_irq_cmds));
	if (cmds == NULL) {
		free(cmds);
		return ENOMEM;
	}

	/* Bus master IDE status register (primary or secondary) */
	bmisx = chan->chan_id == 0 ?
	    &chan->ctrl->bmregs->bmisp :
	    &chan->ctrl->bmregs->bmiss;

	memcpy(ranges, &pci_ide_irq_ranges, sizeof(pci_ide_irq_ranges));
	ranges[0].base = chan->cmd_physical;
	ranges[1].base = chan->ctrl->bmregs_physical;

	memcpy(cmds, &pci_ide_irq_cmds, sizeof(pci_ide_irq_cmds));
	cmds[0].addr = bmisx;
	cmds[3].addr = bmisx;
	cmds[4].addr = &chan->cmd->status;

	irq_code.rangecount = sizeof(pci_ide_irq_ranges) / sizeof(irq_pio_range_t);
	irq_code.ranges = ranges;
	irq_code.cmdcount = sizeof(pci_ide_irq_cmds) / sizeof(irq_cmd_t);
	irq_code.cmds = cmds;

	ddf_msg(LVL_NOTE, "IRQ %d", chan->irq);
	rc = register_interrupt_handler(chan->ctrl->dev, chan->irq,
	    pci_ide_irq_handler, (void *)chan, &irq_code, &chan->ihandle);
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
static void pci_ide_fini_irq(pci_ide_channel_t *chan)
{
	errno_t rc;
	async_sess_t *parent_sess;

	parent_sess = ddf_dev_parent_sess_get(chan->ctrl->dev);

	rc = hw_res_disable_interrupt(parent_sess, chan->irq);
	if (rc != EOK)
		ddf_msg(LVL_ERROR, "Error disabling IRQ.");

	(void) unregister_interrupt_handler(chan->ctrl->dev, chan->ihandle);
}

/** Interrupt handler.
 *
 * @param call Call data
 * @param arg Argument (pci_ide_channel_t *)
 */
static void pci_ide_irq_handler(ipc_call_t *call, void *arg)
{
	pci_ide_channel_t *chan = (pci_ide_channel_t *)arg;
	uint8_t status;
	async_sess_t *parent_sess;

	status = ipc_get_arg1(call);
	ata_channel_irq(chan->channel, status);

	parent_sess = ddf_dev_parent_sess_get(chan->ctrl->dev);
	hw_res_clear_interrupt(parent_sess, chan->irq);
}

/** Write the data register callback handler.
 *
 * @param arg Argument (pci_ide_channel_t *)
 * @param data Data
 * @param nwords Number of words to write
 */
static void pci_ide_write_data_16(void *arg, uint16_t *data, size_t nwords)
{
	pci_ide_channel_t *chan = (pci_ide_channel_t *)arg;
	size_t i;

	for (i = 0; i < nwords; i++)
		pio_write_16(&chan->cmd->data_port, data[i]);
}

/** Read the data register callback handler.
 *
 * @param arg Argument (pci_ide_channel_t *)
 * @param buf Destination buffer
 * @param nwords Number of words to read
 */
static void pci_ide_read_data_16(void *arg, uint16_t *buf, size_t nwords)
{
	pci_ide_channel_t *chan = (pci_ide_channel_t *)arg;
	size_t i;

	for (i = 0; i < nwords; i++)
		buf[i] = pio_read_16(&chan->cmd->data_port);
}

/** Write command register callback handler.
 *
 * @param arg Argument (pci_ide_channel_t *)
 * @param off Register offset
 * @param value Value to write to command register
 */
static void pci_ide_write_cmd_8(void *arg, uint16_t off, uint8_t value)
{
	pci_ide_channel_t *chan = (pci_ide_channel_t *)arg;

	pio_write_8(((ioport8_t *)chan->cmd) + off, value);
}

/** Read command register callback handler.
 *
 * @param arg Argument (pci_ide_channel_t *)
 * @param off Register offset
 * @return value Value read from command register
 */
static uint8_t pci_ide_read_cmd_8(void *arg, uint16_t off)
{
	pci_ide_channel_t *chan = (pci_ide_channel_t *)arg;

	return pio_read_8(((ioport8_t *)chan->cmd) + off);
}

/** Write control register callback handler.
 *
 * @param arg Argument (pci_ide_channel_t *)
 * @param off Register offset
 * @param value Value to write to control register
 */
static void pci_ide_write_ctl_8(void *arg, uint16_t off, uint8_t value)
{
	pci_ide_channel_t *chan = (pci_ide_channel_t *)arg;

	pio_write_8(((ioport8_t *)chan->ctl) + off, value);
}

/** Read control register callback handler.
 *
 * @param arg Argument (pci_ide_channel_t *)
 * @param off Register offset
 * @return value Value read from control register
 */
static uint8_t pci_ide_read_ctl_8(void *arg, uint16_t off)
{
	pci_ide_channel_t *chan = (pci_ide_channel_t *)arg;

	return pio_read_8(((ioport8_t *)chan->ctl) + off);
}

/** Enable IRQ callback handler
 *
 * @param arg Argument (pci_ide_channel_t *)
 * @return EOK on success or an error code
 */
static errno_t pci_ide_irq_enable(void *arg)
{
	pci_ide_channel_t *chan = (pci_ide_channel_t *)arg;
	async_sess_t *parent_sess;
	errno_t rc;

	ddf_msg(LVL_DEBUG, "Enable IRQ %d for channel %u",
	    chan->irq, chan->chan_id);

	parent_sess = ddf_dev_parent_sess_get(chan->ctrl->dev);

	rc = hw_res_enable_interrupt(parent_sess, chan->irq);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error enabling IRQ.");
		return rc;
	}

	return EOK;
}

/** Disable IRQ callback handler
 *
 * @param arg Argument (pci_ide_channel_t *)
 * @return EOK on success or an error code
 */
static errno_t pci_ide_irq_disable(void *arg)
{
	pci_ide_channel_t *chan = (pci_ide_channel_t *)arg;
	async_sess_t *parent_sess;
	errno_t rc;

	ddf_msg(LVL_DEBUG, "Disable IRQ");

	parent_sess = ddf_dev_parent_sess_get(chan->ctrl->dev);

	rc = hw_res_disable_interrupt(parent_sess, chan->irq);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error disabling IRQ.");
		return rc;
	}

	return EOK;
}

/** Set up DMA channel callback handler
 *
 * @param arg Argument (pci_ide_channel_t *)
 * @param buf Buffer
 * @param buf_size Buffer size
 * @param dir DMA transfer direction
 */
static void pci_ide_dma_chan_setup(void *arg, void *buf, size_t buf_size,
    ata_dma_dir_t dir)
{
	pci_ide_channel_t *chan = (pci_ide_channel_t *)arg;
	uint8_t *bmicx;
	uint8_t val;

	/* Needed later in teardown */
	chan->cur_dir = dir;
	chan->cur_buf = buf;
	chan->cur_buf_size = buf_size;

	if (dir == ata_dma_write) {
		assert(buf_size <= chan->dma_buf_size);
		memcpy(chan->dma_buf, buf, buf_size);
	}

	/* Primary or secondary channel control register */
	bmicx = (chan->chan_id == 0) ? &chan->ctrl->bmregs->bmicp :
	    &chan->ctrl->bmregs->bmics;

	/* Set read / write */
	val = (dir == ata_dma_write) ? bmicx_rwcon : 0;
	pio_write_8(bmicx, val);

	/* Start bus master DMA engine */
	val = val | bmicx_ssbm;
	pio_write_8(bmicx, val);
}

/** Tear down DMA channel callback handler
 *
 * @param arg Argument (pci_ide_channel_t *)
 */
static void pci_ide_dma_chan_teardown(void *arg)
{
	pci_ide_channel_t *chan = (pci_ide_channel_t *)arg;
	uint8_t *bmicx;
	uint8_t val;

	/* Primary or secondary channel control register */
	bmicx = (chan->chan_id == 0) ? &chan->ctrl->bmregs->bmicp :
	    &chan->ctrl->bmregs->bmics;

	/* Stop bus master DMA engine clear SSBM but keep RWCON the same */
	val = (chan->cur_dir == ata_dma_write) ? bmicx_rwcon : 0;
	pio_write_8(bmicx, val);

	if (chan->cur_dir == ata_dma_read) {
		assert(chan->cur_buf_size <= chan->dma_buf_size);
		memcpy(chan->cur_buf, chan->dma_buf, chan->cur_buf_size);
	}
}

/** Add ATA device callback handler.
 *
 * @param arg Argument (pci_ide_channel_t *)
 * @param idx Device index
 * $param charg Connection handler argument
 * @return EOK on success or an error code
 */
static errno_t pci_ide_add_device(void *arg, unsigned idx, void *charg)
{
	pci_ide_channel_t *chan = (pci_ide_channel_t *)arg;
	return pci_ide_fun_create(chan, idx, charg);
}

/** Remove ATA device callback handler.
 *
 * @param arg Argument (pci_ide_channel_t *)
 * @param idx Device index
 * @return EOK on success or an error code
 */
static errno_t pci_ide_remove_device(void *arg, unsigned idx)
{
	pci_ide_channel_t *chan = (pci_ide_channel_t *)arg;
	return pci_ide_fun_remove(chan, idx);
}

/** Debug message callback handler.
 *
 * @param arg Argument (pci_ide_channel_t *)
 * @param msg Message
 */
static void pci_ide_msg_debug(void *arg, char *msg)
{
	(void)arg;
	ddf_msg(LVL_DEBUG, "%s", msg);
}

/** Notice message callback handler.
 *
 * @param arg Argument (pci_ide_channel_t *)
 * @param msg Message
 */
static void pci_ide_msg_note(void *arg, char *msg)
{
	(void)arg;
	ddf_msg(LVL_NOTE, "%s", msg);
}

/** Warning message callback handler.
 *
 * @param arg Argument (pci_ide_channel_t *)
 * @param msg Message
 */
static void pci_ide_msg_warn(void *arg, char *msg)
{
	(void)arg;
	ddf_msg(LVL_WARN, "%s", msg);
}

/** Error message callback handler.
 *
 * @param arg Argument (pci_ide_channel_t *)
 * @param msg Message
 */
static void pci_ide_msg_error(void *arg, char *msg)
{
	(void)arg;
	ddf_msg(LVL_ERROR, "%s", msg);
}

/**
 * @}
 */
