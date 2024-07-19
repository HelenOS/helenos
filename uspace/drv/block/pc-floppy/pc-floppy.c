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

/** @addtogroup pc-floppy
 * @{
 */

/**
 * @file
 * @brief PC floppy disk driver
 */

#include <bd_srv.h>
#include <ddi.h>
#include <ddf/interrupt.h>
#include <ddf/log.h>
#include <async.h>
#include <as.h>
#include <errno.h>
#include <fibril_synch.h>
#include <macros.h>
#include <perf.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <str.h>
#include <str_error.h>
#include <inttypes.h>
#include <errno.h>

#include "pc-floppy.h"

static errno_t pc_fdc_init_io(pc_fdc_t *);
static void pc_fdc_fini_io(pc_fdc_t *);
static errno_t pc_fdc_init_irq(pc_fdc_t *);
static void pc_fdc_fini_irq(pc_fdc_t *);
static void pc_fdc_irq_handler(ipc_call_t *, void *);

static errno_t pc_fdc_drive_create(pc_fdc_t *, unsigned, pc_fdc_drive_t **);

static errno_t pc_fdc_reset(pc_fdc_t *);
static errno_t pc_fdc_read_id(pc_fdc_t *, bool, uint8_t, uint8_t);
static errno_t pc_fdc_sense_int_sts(pc_fdc_t *);

static errno_t pc_fdc_bd_open(bd_srvs_t *, bd_srv_t *);
static errno_t pc_fdc_bd_close(bd_srv_t *);
static errno_t pc_fdc_bd_read_blocks(bd_srv_t *, uint64_t, size_t, void *, size_t);
static errno_t pc_fdc_bd_read_toc(bd_srv_t *, uint8_t, void *, size_t);
static errno_t pc_fdc_bd_write_blocks(bd_srv_t *, uint64_t, size_t, const void *,
    size_t);
static errno_t pc_fdc_bd_get_block_size(bd_srv_t *, size_t *);
static errno_t pc_fdc_bd_get_num_blocks(bd_srv_t *, aoff64_t *);
static errno_t pc_fdc_bd_sync_cache(bd_srv_t *, aoff64_t, size_t);

static bd_ops_t pc_fdc_bd_ops = {
	.open = pc_fdc_bd_open,
	.close = pc_fdc_bd_close,
	.read_blocks = pc_fdc_bd_read_blocks,
	.read_toc = pc_fdc_bd_read_toc,
	.write_blocks = pc_fdc_bd_write_blocks,
	.get_block_size = pc_fdc_bd_get_block_size,
	.get_num_blocks = pc_fdc_bd_get_num_blocks,
	.sync_cache = pc_fdc_bd_sync_cache
};

enum {
	msr_read_cycles = 100,
	fdc_def_dma_buf_size = 4096
};

static const irq_pio_range_t pc_fdc_irq_ranges[] = {
	{
		.base = 0,
		.size = sizeof(pc_fdc_regs_t)
	}
};

/** PC floppy interrupt pseudo code.
 *
 * Floppy interrupts are complex. Since we don't want to change the handler
 * all the time, need to handle both status-ful and status-less IRQs.
 *
 * XXX Do we need to wait for MSR.RQM == 1? That's really difficult
 * in pseudocode.
 *
 * if (MSR.DIO == 0)
 *    send SENSE INT STATUS;
 *
 * st[0] = read DATA;
 * [repeat 5 times]
 *   if (MSR.DIO == 1)
 *       st[1] = read DATA;
 *
 */
static const irq_cmd_t pc_fdc_irq_cmds[] = {
	{
		.cmd = CMD_ACCEPT
	}

};

/** Create PC floppy controller driver instnace.
 *
 * @param dev DDF device
 * @param res HW resources
 * @param rfdc Place to store pointer to floppy controller
 */
errno_t pc_fdc_create(ddf_dev_t *dev, pc_fdc_hwres_t *res, pc_fdc_t **rfdc)
{
	errno_t rc;
	pc_fdc_t *fdc;
	bool irq_inited = false;
	void *buffer = NULL;

	ddf_msg(LVL_DEBUG, "pc_fdc_init()");

	fdc = ddf_dev_data_alloc(dev, sizeof(pc_fdc_t));
	if (fdc == NULL) {
		ddf_msg(LVL_ERROR, "Failed allocating FDC.");
		rc = ENOMEM;
		goto error;
	}

	fdc->dev = dev;

	fibril_mutex_initialize(&fdc->lock);
	fdc->regs_physical = res->regs;
	fdc->irq = res->irq;
	fdc->dma = res->dma;

	ddf_msg(LVL_NOTE, "I/O address %p", (void *)fdc->regs_physical);

	ddf_msg(LVL_DEBUG, "Init I/O");
	rc = pc_fdc_init_io(fdc);
	if (rc != EOK)
		return rc;

	ddf_msg(LVL_DEBUG, "Init IRQ");
	rc = pc_fdc_init_irq(fdc);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Init IRQ failed");
		return rc;
	}

	irq_inited = true;

	ddf_msg(LVL_DEBUG, "pc_fdc_ctrl_init(): read ID");

	fdc->dma_buf_size = fdc_def_dma_buf_size;

	buffer = AS_AREA_ANY;
	rc = dmamem_map_anonymous(fdc->dma_buf_size, DMAMEM_1MiB | 0xffff,
	    AS_AREA_WRITE | AS_AREA_READ, 0, &fdc->dma_buf_pa, &buffer);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed allocating PRD table.");
		goto error;
	}

	fdc->dma_buf = buffer;

	ddf_msg(LVL_DEBUG, "pc_fdc_ctrl_init: reset controller...");
	(void)pc_fdc_reset(fdc);
	ddf_msg(LVL_DEBUG, "pc_fdc_ctrl_init: read_ID..");

	ddf_msg(LVL_DEBUG, "pc_fdc_ctrl_init: MSR=0x%x",
	    pio_read_8(&fdc->regs->msr));
	ddf_msg(LVL_DEBUG, "pc_fdc_ctrl_init: DIR=0x%x",
	    pio_read_8(&fdc->regs->dir));
	ddf_msg(LVL_DEBUG, "pc_fdc_ctrl_init: SRA=0x%x",
	    pio_read_8(&fdc->regs->sra));
	ddf_msg(LVL_DEBUG, "pc_fdc_ctrl_init: SRB=0x%x",
	    pio_read_8(&fdc->regs->srb));

	rc = pc_fdc_sense_int_sts(fdc);
	if (rc != EOK)
		return rc;
	rc = pc_fdc_sense_int_sts(fdc);
	if (rc != EOK)
		return rc;
	rc = pc_fdc_sense_int_sts(fdc);
	if (rc != EOK)
		return rc;
	rc = pc_fdc_sense_int_sts(fdc);
	if (rc != EOK)
		return rc;

	/* Read ID MFM, D0, H0 */
	rc = pc_fdc_read_id(fdc, true, 0, 0);
	ddf_msg(LVL_DEBUG, "pc_fdc_ctrl_init: read_ID -> %d", rc);

	rc = pc_fdc_drive_create(fdc, 0, &fdc->drive[0]);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "pc_fdc_ctrl_init: pc_fdc_drive_create "
		    "failed");
		goto error;
	}

	fdc->drive[0]->sec_size = 512;
	fdc->drive[0]->cylinders = 80;
	fdc->drive[0]->heads = 2;
	fdc->drive[0]->sectors = 18;

	ddf_msg(LVL_DEBUG, "pc_fdc_ctrl_init: DONE");
	return EOK;
error:
	if (buffer != NULL)
		dmamem_unmap_anonymous(buffer);
	if (irq_inited)
		pc_fdc_fini_irq(fdc);
	pc_fdc_fini_io(fdc);
	return rc;
}

/** Destroy floppy controller instance.
 *
 * @param fdc Floppy controller
 */
errno_t pc_fdc_destroy(pc_fdc_t *fdc)
{
	ddf_msg(LVL_DEBUG, ": pc_fdc_destroy()");

	fibril_mutex_lock(&fdc->lock);

	pc_fdc_fini_irq(fdc);
	pc_fdc_fini_io(fdc);
	fibril_mutex_unlock(&fdc->lock);

	free(fdc);
	return EOK;
}

/** Enable device I/O.
 *
 * @param fdc Floppy controller
 * @return EOK on success or an error code
 */
static errno_t pc_fdc_init_io(pc_fdc_t *fdc)
{
	errno_t rc;
	void *vaddr;

	rc = pio_enable((void *) fdc->regs_physical, sizeof(pc_fdc_regs_t),
	    &vaddr);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Cannot initialize device I/O space.");
		return rc;
	}

	fdc->regs = vaddr;
	return EOK;
}

/** Clean up device I/O.
 *
 * @param fdc Floppy controller
 */
static void pc_fdc_fini_io(pc_fdc_t *fdc)
{
	(void)fdc;
	/* XXX TODO */
}

/** Initialize IRQ.
 *
 * @param fdc Floppy controller
 * @return EOK on success or an error code
 */
static errno_t pc_fdc_init_irq(pc_fdc_t *fdc)
{
	irq_code_t irq_code;
	irq_pio_range_t *ranges;
	irq_cmd_t *cmds;
	async_sess_t *parent_sess;
	errno_t rc;

	if (fdc->irq < 0)
		return EOK;

	ranges = malloc(sizeof(pc_fdc_irq_ranges));
	if (ranges == NULL)
		return ENOMEM;

	cmds = malloc(sizeof(pc_fdc_irq_cmds));
	if (cmds == NULL) {
		free(cmds);
		return ENOMEM;
	}

	memcpy(ranges, &pc_fdc_irq_ranges, sizeof(pc_fdc_irq_ranges));
	ranges[0].base = fdc->regs_physical;
	memcpy(cmds, &pc_fdc_irq_cmds, sizeof(pc_fdc_irq_cmds));

	irq_code.rangecount = sizeof(pc_fdc_irq_ranges) / sizeof(irq_pio_range_t);
	irq_code.ranges = ranges;
	irq_code.cmdcount = sizeof(pc_fdc_irq_cmds) / sizeof(irq_cmd_t);
	irq_code.cmds = cmds;

	ddf_msg(LVL_NOTE, "IRQ %d", fdc->irq);

	rc = register_interrupt_handler(fdc->dev, fdc->irq,
	    pc_fdc_irq_handler, (void *)fdc, &irq_code, &fdc->ihandle);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error registering IRQ.");
		goto error;
	}

	parent_sess = ddf_dev_parent_sess_get(fdc->dev);

	rc = hw_res_enable_interrupt(parent_sess, fdc->irq);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error enabling IRQ.");
		return rc;
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

/** Clean up IRQ.
 *
 * @param fdc Floppy disk controller
 */
static void pc_fdc_fini_irq(pc_fdc_t *fdc)
{
	errno_t rc;
	async_sess_t *parent_sess;

	parent_sess = ddf_dev_parent_sess_get(fdc->dev);

	rc = hw_res_disable_interrupt(parent_sess, fdc->irq);
	if (rc != EOK)
		ddf_msg(LVL_ERROR, "Error disabling IRQ.");

	(void) unregister_interrupt_handler(fdc->dev, fdc->ihandle);
}

/** Get DDF function name for drive.
 *
 * @param fdc Floppy disk controller
 * @param idx Drive index
 * @return Function name (newly allocated string)
 */
static char *pc_fdc_fun_name(pc_fdc_t *fdc, unsigned idx)
{
	char *fun_name;

	if (asprintf(&fun_name, "d%u", idx) < 0)
		return NULL;

	return fun_name;
}

/** Block device connection handler.
 *
 * @param icall Incoming call (pc_fdc_drive_t *)
 * @param arg Argument
 */
static void pc_fdc_connection(ipc_call_t *icall, void *arg)
{
	pc_fdc_drive_t *drive;

	drive = (pc_fdc_drive_t *)ddf_fun_data_get((ddf_fun_t *)arg);
	bd_conn(icall, &drive->bds);
}

/** Create floppy drive object.
 *
 * @param fdc Floppy disk controller
 * @param idx Drive index
 * @param rdrive Place to store pointer to drive
 *
 * @return EOK on success or an error code
 */
static errno_t pc_fdc_drive_create(pc_fdc_t *fdc, unsigned idx,
    pc_fdc_drive_t **rdrive)
{
	errno_t rc;
	char *fun_name = NULL;
	ddf_fun_t *fun = NULL;
	pc_fdc_drive_t *drive = NULL;
	bool bound = false;

	fun_name = pc_fdc_fun_name(fdc, idx);
	if (fun_name == NULL) {
		ddf_msg(LVL_ERROR, "Out of memory.");
		rc = ENOMEM;
		goto error;
	}

	fun = ddf_fun_create(fdc->dev, fun_exposed, fun_name);
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating DDF function.");
		rc = ENOMEM;
		goto error;
	}

	/* Allocate soft state */
	drive = ddf_fun_data_alloc(fun, sizeof(pc_fdc_drive_t));
	if (drive == NULL) {
		ddf_msg(LVL_ERROR, "Failed allocating softstate.");
		rc = ENOMEM;
		goto error;
	}

	drive->fdc = fdc;
	drive->fun = fun;

	bd_srvs_init(&drive->bds);
	drive->bds.ops = &pc_fdc_bd_ops;
	drive->bds.sarg = (void *)drive;

	/* Set up a connection handler. */
	ddf_fun_set_conn_handler(fun, pc_fdc_connection);

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding DDF function %s: %s",
		    fun_name, str_error(rc));
		goto error;
	}

	bound = true;

#if 0
	rc = ddf_fun_add_to_category(fun, "partition");
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed adding function %s to "
		    "category 'partition': %s", fun_name, str_error(rc));
		goto error;
	}
#endif

	free(fun_name);
	*rdrive = drive;
	return EOK;
error:
	if (bound)
		ddf_fun_unbind(fun);
	if (fun != NULL)
		ddf_fun_destroy(fun);
	if (fun_name != NULL)
		free(fun_name);

	return rc;
}

/** Send byte to FDC data register.
 *
 * @param fdc Floppy controller
 * @param byte Data byte
 *
 * @return EOK on success or an error code
 */
static errno_t pc_fdc_send_byte(pc_fdc_t *fdc, uint8_t byte)
{
	unsigned cnt;
	uint8_t status;
	stopwatch_t sw;
	nsec_t nsec;

	/*
	 * We need to wait for up to 250 usec in total
	 * per Intel 82077AA programming guidelines
	 */
	stopwatch_init(&sw);
	stopwatch_start(&sw);

	status = pio_read_8(&fdc->regs->msr);
	ddf_msg(LVL_DEBUG, "pc_fdc_send_byte: status=0x%x", status);
	do {
		cnt = msr_read_cycles;
		while (cnt > 0) {
			if ((status & fmsr_rqm) != 0 &&
			    (status & fmsr_dio) == 0) {
				pio_write_8(&fdc->regs->data, byte);
				return EOK;
			}

			--cnt;
			status = pio_read_8(&fdc->regs->msr);
		}

		stopwatch_stop(&sw);
		nsec = stopwatch_get_nanos(&sw);
		ddf_msg(LVL_DEBUG, "nsec=%lld", nsec);
	} while (nsec < 1000 * msr_max_wait_usec);

	ddf_msg(LVL_ERROR, "pc_fdc_send_byte: FAILED (status=0x%x)", status);
	return EIO;
}

/** Send a block of data to FDC data register.
 *
 * @param fdc Floppy controller
 * @param data Data block
 * @parma nbytes Size of data block in bytes
 *
 * @return EOK on success or an error code
 */
static errno_t pc_fdc_send(pc_fdc_t *fdc, const void *data, size_t nbytes)
{
	size_t i;
	errno_t rc;
	uint8_t status;

	for (i = 0; i < nbytes; i++) {
		rc = pc_fdc_send_byte(fdc, ((uint8_t *)data)[i]);
		if (rc != EOK)
			return rc;
	}

	status = pio_read_8(&fdc->regs->msr);
	ddf_msg(LVL_DEBUG, "pc_fdc_send: final status=0x%x", status);
	return EOK;
}

/** Get byte from FDC data register.
 *
 * @param fdc Floppy controller
 * @param byte Place to store data byte
 *
 * @return EOK on success or an error code
 */
static errno_t pc_fdc_get_byte(pc_fdc_t *fdc, uint8_t *byte)
{
	unsigned cnt;
	uint8_t status;
	stopwatch_t sw;
	nsec_t nsec;

	/*
	 * We need to wait for up to 250 usec in total
	 * per Intel 82077AA programming guidelines
	 */
	stopwatch_init(&sw);
	stopwatch_start(&sw);

	status = pio_read_8(&fdc->regs->msr);
	ddf_msg(LVL_DEBUG, "pc_fdc_get_byte: status=0x%x", status);
	do {
		cnt = msr_read_cycles;
		while (cnt > 0) {
			if ((status & fmsr_rqm) != 0 &&
			    (status & fmsr_dio) != 0) {
				*byte = pio_read_8(&fdc->regs->data);
				return EOK;
			}

			--cnt;
			status = pio_read_8(&fdc->regs->msr);
		}

		stopwatch_stop(&sw);
		nsec = stopwatch_get_nanos(&sw);
	} while (nsec / 1000 < 1000 * msr_max_wait_usec);

	ddf_msg(LVL_ERROR, "pc_fdc_get_byte: FAILED (status=0x%x)", status);
	return EIO;
}

/** Receive a block of data to FDC data register.
 *
 * @param fdc Floppy controller
 * @param buf Data buffer
 * @param bsize Size of data buffer in bytes
 *
 * @return EOK on success or an error code
 */
static errno_t pc_fdc_get(pc_fdc_t *fdc, void *buf, size_t bsize)
{
	size_t i;
	errno_t rc;
	uint8_t status;

	for (i = 0; i < bsize; i++) {
		rc = pc_fdc_get_byte(fdc, &((uint8_t *)buf)[i]);
		if (rc != EOK) {
			ddf_msg(LVL_ERROR, "pc_fdc_get: abort after "
			    "reading %zu bytes", i);
			return rc;
		}
	}

	ddf_msg(LVL_DEBUG, "pc_fdc_get: successfully read %zu bytes", i);
	status = pio_read_8(&fdc->regs->msr);
	ddf_msg(LVL_DEBUG, "pc_fdc_get: final status=0x%x", status);
	return EOK;
}

/** Reset floppy controller.
 *
 * @param fdc Floppy controller
 * @return EOK on success or an error code
 */
static errno_t pc_fdc_reset(pc_fdc_t *fdc)
{
	uint8_t dor;

	/* Use DSR reset for 82072 (or older) compatibility */
	pio_write_8(&fdc->regs->dsr, fdsr_sw_reset | fdsr_drate_500kbps);

	/* Clear DOR reset in case it was set (i.e., nreset := 1) */
	dor = pio_read_8(&fdc->regs->dor);
	ddf_msg(LVL_DEBUG, "pc_fdc_reset: old DOR=0x%x, DOR := 0x%x", dor,
	    dor & ~fdor_nreset);
	pio_write_8(&fdc->regs->dor, dor & ~fdor_nreset);

	dor = pio_read_8(&fdc->regs->dor);
	ddf_msg(LVL_DEBUG, "pc_fdc_reset: read DOR: value= 0x%x", dor);

	fibril_usleep(4);

	ddf_msg(LVL_DEBUG, "pc_fdc_reset: old DOR=0x%x, DOR := 0x%x", dor,
	    dor | fdor_nreset | fdor_ndmagate);
	pio_write_8(&fdc->regs->dor, dor | fdor_nreset | fdor_ndmagate);

	dor = pio_read_8(&fdc->regs->dor);
	ddf_msg(LVL_DEBUG, "pc_fdc_reset: read DOR: value= 0x%x", dor);

	return EOK;
}

/** Perform Read ID command.
 *
 * @param fdc Floppy controller
 * @param mfm Enable MFM (double density) mode
 * @param drive Drive (0-3)
 * @param head Head (0-1)
 *
 * @return EOK on success or an error code
 */
static errno_t pc_fdc_read_id(pc_fdc_t *fdc, bool mfm, uint8_t drive,
    uint8_t head)
{
	pc_fdc_read_id_data_t cmd;
	pc_fdc_cmd_status_t status;
	uint8_t dor;
	errno_t rc;

	dor = pio_read_8(&fdc->regs->dor);
	ddf_msg(LVL_DEBUG, "pc_fdc_read_id: read DOR: value= 0x%x", dor);

	dor |= fdor_me0; /* turn drive 0 motor on */
	dor = (dor & ~0x03) | 0x00; /* select drive 0 */
	pio_write_8(&fdc->regs->dor, dor);
	ddf_msg(LVL_DEBUG, "pc_fdc_read_id: DOR := 0x%x", dor);

	dor = pio_read_8(&fdc->regs->dor);
	ddf_msg(LVL_DEBUG, "pc_fdc_read_id: read DOR: value= 0x%x", dor);

	/* 500 ms to let drive spin up */
	fibril_usleep(500 * 1000);

	cmd.flags_cc = fcf_mf | fcc_read_id;
	cmd.hd_us = 0x00;

	ddf_msg(LVL_DEBUG, "read ID: send");
	rc = pc_fdc_send(fdc, &cmd, sizeof(cmd));
	if (rc != EOK) {
		ddf_msg(LVL_WARN, "Failed sending READ ID command.");
		return rc;
	}

	ddf_msg(LVL_DEBUG, "read ID: get");
	rc = pc_fdc_get(fdc, &status, sizeof(status));
	if (rc != EOK) {
		ddf_msg(LVL_WARN, "Failed getting status for READ ID");
		return rc;
	}

	ddf_msg(LVL_DEBUG, "read ID: DONE");
	ddf_msg(LVL_DEBUG, "st0=0x%x st1=0x%x st2=0x%x cyl=%u head=%u rec=%u "
	    "number=%u", status.st0, status.st1, status.st2,
	    status.cyl, status.head, status.rec, status.number);

	/* Check for success status */
	if ((status.st0 & fsr0_ic_mask) != 0)
		return EIO;

	return EOK;
}

/** Perform Read Data command.
 *
 * @param drive Floppy drive
 * @param cyl Cylinder
 * @param head Head
 * @param sec Sector
 * @param buf Destination buffer
 * @param buf_size Destination buffer size
 *
 * @return EOK on success or an error code
 */
static errno_t pc_fdc_drive_read_data(pc_fdc_drive_t *drive,
    uint8_t cyl, uint8_t head, uint8_t sec, void *buf, size_t buf_size)
{
	pc_fdc_t *fdc = drive->fdc;
	pc_fdc_cmd_data_t cmd;
	pc_fdc_cmd_status_t status;
	async_sess_t *sess;
	size_t csize;
	errno_t rc;

	ddf_msg(LVL_DEBUG, "pc_fdc_drive_read_data");

	memset(fdc->dma_buf, 0, fdc->dma_buf_size);

	sess = ddf_dev_parent_sess_get(fdc->dev);
	ddf_msg(LVL_DEBUG, "hw_res_dma_channel_setup(sess=%p, chan=%d "
	    "pa=%" PRIuPTR " size=%zu", sess, fdc->dma, fdc->dma_buf_pa,
	    fdc->dma_buf_size);
	rc = hw_res_dma_channel_setup(sess, fdc->dma, fdc->dma_buf_pa,
	    fdc->dma_buf_size, DMA_MODE_READ | DMA_MODE_AUTO |
	    DMA_MODE_ON_DEMAND);
	ddf_msg(LVL_DEBUG, "hw_res_dma_channel_setup->%d", rc);

	cmd.flags_cc = fcf_mf | fcc_read_data;
	cmd.hd_us = (head & 1) << 2 | 0x00 /* drive 0 */;
	cmd.cyl = cyl;
	cmd.head = head;
	cmd.rec = sec;
	cmd.number = 2; /* 512 bytes */
	cmd.eot = sec;
	cmd.gpl = 0x1b;
	cmd.dtl = 0xff;

	ddf_msg(LVL_DEBUG, "read data: send");
	rc = pc_fdc_send(fdc, &cmd, sizeof(cmd));
	if (rc != EOK) {
		ddf_msg(LVL_WARN, "Failed sending Read Data command.");
		return rc;
	}

	ddf_msg(LVL_DEBUG, "read data: get");
	rc = pc_fdc_get(fdc, &status, sizeof(status));
	if (rc != EOK) {
		ddf_msg(LVL_WARN, "Failed getting status for Read Data");
		return rc;
	}

	ddf_msg(LVL_DEBUG, "read data: DONE");
	ddf_msg(LVL_DEBUG, "st0=0x%x st1=0x%x st2=0x%x cyl=%u head=%u rec=%u "
	    "number=%u", status.st0, status.st1, status.st2,
	    status.cyl, status.head, status.rec, status.number);

	/* Check for success status */
	if ((status.st0 & fsr0_ic_mask) != 0)
		return EIO;

	/* Copy data from DMA buffer to destination buffer */
	csize = min(fdc->dma_buf_size, buf_size);
	memcpy(buf, fdc->dma_buf, csize);

	return EOK;
}

/** Perform Write Data command.
 *
 * @param drive Floppy drive
 * @param cyl Cylinder
 * @param head Head
 * @param sec Sector
 * @param buf Source buffer
 * @param buf_size Source buffer size
 *
 * @return EOK on success or an error code
 */
static errno_t pc_fdc_drive_write_data(pc_fdc_drive_t *drive,
    uint8_t cyl, uint8_t head, uint8_t sec, const void *buf, size_t buf_size)
{
	pc_fdc_t *fdc = drive->fdc;
	pc_fdc_cmd_data_t cmd;
	pc_fdc_cmd_status_t status;
	async_sess_t *sess;
	size_t csize;
	errno_t rc;

	ddf_msg(LVL_DEBUG, "pc_fdc_drive_write_data");

	/* Copy data from source buffer to DMA buffer */
	csize = min(fdc->dma_buf_size, buf_size);
	memcpy(fdc->dma_buf, buf, csize);

	sess = ddf_dev_parent_sess_get(fdc->dev);
	ddf_msg(LVL_DEBUG, "hw_res_dma_channel_setup(sess=%p, chan=%d "
	    "pa=%" PRIuPTR " size=%zu", sess, fdc->dma, fdc->dma_buf_pa,
	    fdc->dma_buf_size);
	rc = hw_res_dma_channel_setup(sess, fdc->dma, fdc->dma_buf_pa,
	    fdc->dma_buf_size, DMA_MODE_WRITE | DMA_MODE_AUTO |
	    DMA_MODE_ON_DEMAND);
	ddf_msg(LVL_DEBUG, "hw_res_dma_channel_setup->%d", rc);

	cmd.flags_cc = fcf_mf | fcc_write_data;
	cmd.hd_us = (head & 1) << 2 | 0x00 /* drive 0 */;
	cmd.cyl = cyl;
	cmd.head = head;
	cmd.rec = sec;
	cmd.number = 2; /* 512 bytes */
	cmd.eot = sec;
	cmd.gpl = 0x1b;
	cmd.dtl = 0xff;

	ddf_msg(LVL_DEBUG, "write data: send");
	rc = pc_fdc_send(fdc, &cmd, sizeof(cmd));
	if (rc != EOK) {
		ddf_msg(LVL_WARN, "Failed sending Write Data command.");
		return rc;
	}

	ddf_msg(LVL_DEBUG, "write data: get");
	rc = pc_fdc_get(fdc, &status, sizeof(status));
	if (rc != EOK) {
		ddf_msg(LVL_WARN, "Failed getting status for Write Data");
		return rc;
	}

	ddf_msg(LVL_DEBUG, "write data: DONE");
	ddf_msg(LVL_DEBUG, "st0=0x%x st1=0x%x st2=0x%x cyl=%u head=%u rec=%u "
	    "number=%u", status.st0, status.st1, status.st2,
	    status.cyl, status.head, status.rec, status.number);

	/* Check for success status */
	if ((status.st0 & fsr0_ic_mask) != 0)
		return EIO;

	return EOK;
}

/** Perform Sense Interrupt Status command.
 *
 * @param fdc Floppy controller
 * @param mfm Enable MFM (double density) mode
 * @param drive Drive (0-3)
 * @param head Head (0-1)
 *
 * @return EOK on success or an error code
 */
static errno_t pc_fdc_sense_int_sts(pc_fdc_t *fdc)
{
	pc_fdc_sense_int_sts_data_t cmd;
	pc_fdc_sense_int_sts_status_t status;
	errno_t rc;

	cmd.cc = fcc_sense_int_sts;

	ddf_msg(LVL_DEBUG, "Sense Interrupt Status: send");
	rc = pc_fdc_send(fdc, &cmd, sizeof(cmd));
	if (rc != EOK) {
		ddf_msg(LVL_WARN, "Failed sending Sense Interrupt Status "
		    "command.");
		return rc;
	}

	ddf_msg(LVL_DEBUG, "Sense Interrupt Status: get");
	rc = pc_fdc_get(fdc, &status, sizeof(status));
	if (rc != EOK) {
		ddf_msg(LVL_WARN, "Failed getting status for Sense Interrupt "
		    "Status");
		return rc;
	}

	ddf_msg(LVL_DEBUG, "Sense Interrupt Status: DONE");
	ddf_msg(LVL_DEBUG, "st0=0x%x pcn=0x%x", status.st0, status.pcn);

	return EOK;
}

/** Interrupt handler.
 *
 * @param call Call data
 * @param arg Argument (pc_fdc_channel_t *)
 */
static void pc_fdc_irq_handler(ipc_call_t *call, void *arg)
{
	pc_fdc_t *fdc = (pc_fdc_t *)arg;
	uint8_t st0;
	uint8_t st1;
	uint8_t st2;
	uint8_t c, h, n;
	async_sess_t *parent_sess;

	st0 = ipc_get_arg1(call);
	st1 = ipc_get_arg2(call);
	st2 = ipc_get_arg3(call);
	c = ipc_get_arg4(call);
	h = ipc_get_arg5(call);
	n = ipc_get_imethod(call);
	ddf_msg(LVL_DEBUG, "pc_fdc_irq_handler st0=%x st1=%x st2=%x c=%u h=%u n=%u",
	    st0, st1, st2, c, h, n);

	parent_sess = ddf_dev_parent_sess_get(fdc->dev);
	hw_res_clear_interrupt(parent_sess, fdc->irq);
}

/** Get floppy drive from block device service.
 *
 * @param bd Block device
 * @return Floppy drive
 */
static pc_fdc_drive_t *bd_srv_drive(bd_srv_t *bd)
{
	return (pc_fdc_drive_t *)bd->srvs->sarg;
}

/** Convert LBA to CHS.
 *
 * @param drive Floppy drive
 * @param ba Logical block address
 * @param cyl Place to store cylinder number
 * @param head Place to store head number
 * @param sec Place to store sector number
 */
static void pc_fdc_drive_ba_to_chs(pc_fdc_drive_t *drive, uint64_t ba,
    uint8_t *cyl, uint8_t *head, uint8_t *sec)
{
	unsigned ch;

	*sec = 1 + (ba % drive->sectors);
	ch = ba / drive->sectors;
	*head = ch % drive->heads;
	*cyl = ch / drive->heads;
}

/** Open block device.
 *
 * @param bds Block device server
 * @param bd Block device
 * @return EOK on success or an error code
 */
static errno_t pc_fdc_bd_open(bd_srvs_t *bds, bd_srv_t *bd)
{
	return EOK;
}

/** Close block device.
 *
 * @param bd Block device
 * @return EOK on success or an error code
 */
static errno_t pc_fdc_bd_close(bd_srv_t *bd)
{
	return EOK;
}

/** Read multiple blocks from block device.
 *
 * @param bd Block device
 * @param ba Address of first block
 * @param cnt Number of blocks
 * @param buf Destination buffer
 * @param size Size of destination buffer in bytes
 *
 * @return EOK on success or an error code
 */
static errno_t pc_fdc_bd_read_blocks(bd_srv_t *bd, uint64_t ba, size_t cnt,
    void *buf, size_t size)
{
	pc_fdc_drive_t *drive = bd_srv_drive(bd);
	uint8_t cyl, head, sec;
	errno_t rc;

	ddf_msg(LVL_DEBUG, "pc_fdc_bd_read_blocks");

	if (size < cnt * drive->sec_size) {
		rc = EINVAL;
		goto error;
	}

	while (cnt > 0) {
		pc_fdc_drive_ba_to_chs(drive, ba, &cyl, &head, &sec);

		/* Read one block */
		rc = pc_fdc_drive_read_data(drive, cyl, head, sec, buf,
		    drive->sec_size);
		if (rc != EOK)
			goto error;

		++ba;
		--cnt;
		buf += drive->sec_size;
	}

	return EOK;
error:
	ddf_msg(LVL_ERROR, "pc_fdc_bd_read_blocks: rc=%d", rc);
	return rc;
}

/** Read TOC from block device.
 *
 * @param bd Block device
 * @param session Session number
 * @param buf Destination buffer
 * @param size Size of destination buffer in bytes
 *
 * @return EOK on success or an error code
 */
static errno_t pc_fdc_bd_read_toc(bd_srv_t *bd, uint8_t session, void *buf, size_t size)
{
	return ENOTSUP;
}

/** Write multiple blocks to block device.
 *
 * @param bd Block device
 * @param ba Address of first block
 * @param cnt Number of blocks
 * @param buf Source buffer
 * @param size Size of source buffer in bytes
 *
 * @return EOK on success or an error code
 */
static errno_t pc_fdc_bd_write_blocks(bd_srv_t *bd, uint64_t ba, size_t cnt,
    const void *buf, size_t size)
{
	pc_fdc_drive_t *drive = bd_srv_drive(bd);
	uint8_t cyl, head, sec;
	errno_t rc;

	ddf_msg(LVL_DEBUG, "pc_fdc_bd_write_blocks");

	if (size < cnt * drive->sec_size) {
		rc = EINVAL;
		goto error;
	}

	while (cnt > 0) {
		pc_fdc_drive_ba_to_chs(drive, ba, &cyl, &head, &sec);

		/* Write one block */
		rc = pc_fdc_drive_write_data(drive, cyl, head, sec, buf,
		    drive->sec_size);
		if (rc != EOK)
			goto error;

		++ba;
		--cnt;
		buf += drive->sec_size;
	}

	return EOK;
error:
	ddf_msg(LVL_ERROR, "pc_fdc_bd_write_blocks: rc=%d", rc);
	return rc;
}

/** Get device block size. */
static errno_t pc_fdc_bd_get_block_size(bd_srv_t *bd, size_t *rbsize)
{
	pc_fdc_drive_t *drive = bd_srv_drive(bd);

	*rbsize = drive->sec_size;
	return EOK;
}

/** Get device number of blocks. */
static errno_t pc_fdc_bd_get_num_blocks(bd_srv_t *bd, aoff64_t *rnb)
{
	pc_fdc_drive_t *drive = bd_srv_drive(bd);

	*rnb = drive->cylinders * drive->heads * drive->sectors;
	return EOK;
}

/** Flush cache. */
static errno_t pc_fdc_bd_sync_cache(bd_srv_t *bd, uint64_t ba, size_t cnt)
{
	return EOK;
}

/**
 * @}
 */
