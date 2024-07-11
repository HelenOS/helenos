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

/** @file PC floppy disk driver main
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <str_error.h>
#include <ddf/driver.h>
#include <ddf/log.h>
#include <device/hw_res_parsed.h>

#include "pc-floppy.h"

static errno_t pc_fdc_dev_add(ddf_dev_t *dev);
static errno_t pc_fdc_dev_remove(ddf_dev_t *dev);
static errno_t pc_fdc_dev_gone(ddf_dev_t *dev);
static errno_t pc_fdc_fun_online(ddf_fun_t *fun);
static errno_t pc_fdc_fun_offline(ddf_fun_t *fun);

static driver_ops_t driver_ops = {
	.dev_add = &pc_fdc_dev_add,
	.dev_remove = &pc_fdc_dev_remove,
	.dev_gone = &pc_fdc_dev_gone,
	.fun_online = &pc_fdc_fun_online,
	.fun_offline = &pc_fdc_fun_offline
};

static driver_t pc_fdc_driver = {
	.name = NAME,
	.driver_ops = &driver_ops
};

/** Parse FDC hardware resources.
 *
 * @param dev DDF device (from which we get resources)
 * @param res Place to store FDC hardware resources
 * @return EOK on success or an error code
 */
static errno_t pc_fdc_get_res(ddf_dev_t *dev, pc_fdc_hwres_t *res)
{
	async_sess_t *parent_sess;
	hw_res_list_parsed_t hw_res;
	hw_res_flags_t flags;
	errno_t rc;

	parent_sess = ddf_dev_parent_sess_get(dev);
	if (parent_sess == NULL)
		return ENOMEM;

	rc = hw_res_get_flags(parent_sess, &flags);
	if (rc != EOK)
		return rc;

	hw_res_list_parsed_init(&hw_res);
	rc = hw_res_get_list_parsed(parent_sess, &hw_res, 0);
	if (rc != EOK)
		return rc;

	if (hw_res.io_ranges.count != 1) {
		rc = EINVAL;
		goto error;
	}

	/* I/O range */

	addr_range_t *regs_rng = &hw_res.io_ranges.ranges[0];
	res->regs = RNGABS(*regs_rng);

	if (RNGSZ(*regs_rng) < sizeof(pc_fdc_regs_t)) {
		rc = EINVAL;
		goto error;
	}

	/* IRQ */
	if (hw_res.irqs.count > 0) {
		res->irq = hw_res.irqs.irqs[0];
	} else {
		res->irq = -1;
	}

	/* DMA channel */
	if (hw_res.dma_channels.count > 0) {
		res->dma = hw_res.dma_channels.channels[0];
		ddf_msg(LVL_NOTE, "DMA channel %u", res->dma);
	} else {
		res->dma = -1;
	}

	return EOK;
error:
	hw_res_list_parsed_clean(&hw_res);
	return rc;
}

/** Add new FDC device
 *
 * @param dev New device
 * @return EOK on success or an error code.
 */
static errno_t pc_fdc_dev_add(ddf_dev_t *dev)
{
	pc_fdc_t *fdc;
	pc_fdc_hwres_t res;
	errno_t rc;

	rc = pc_fdc_get_res(dev, &res);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Invalid HW resource configuration.");
		return EINVAL;
	}

	rc = pc_fdc_create(dev, &res, &fdc);
	if (rc == ENOENT)
		goto error;

	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed initializing floppy drive "
		    "controller.");
		rc = EIO;
		goto error;
	}

	return EOK;
error:
	return rc;
}

/** Remove FDC device.
 *
 * @param dev Device
 * @return EOK on success or an error code
 */
static errno_t pc_fdc_dev_remove(ddf_dev_t *dev)
{
	pc_fdc_t *fdc = (pc_fdc_t *)ddf_dev_data_get(dev);
	errno_t rc;

	ddf_msg(LVL_DEBUG, "pc_fdc_dev_remove(%p)", dev);

	rc = pc_fdc_destroy(fdc);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Suprise removal of FDC device.
 *
 * @param dev Device
 * @return EOK on success or an error code
 */
static errno_t pc_fdc_dev_gone(ddf_dev_t *dev)
{
	(void)dev;
	return ENOTSUP;
}

/** Online FDC function.
 *
 * @param fun DDF function
 * @return EOK on success or an error code
 */
static errno_t pc_fdc_fun_online(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "pc_fdc_fun_online()");
	return ddf_fun_online(fun);
}

/** Offline FDC function.
 *
 * @param fun DDF function
 * @return EOK on success or an error code
 */
static errno_t pc_fdc_fun_offline(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "pc_fdc_fun_offline()");
	return ddf_fun_offline(fun);
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS PC floppy disk driver\n");
	ddf_log_init(NAME);
	return ddf_driver_main(&pc_fdc_driver);
}

/**
 * @}
 */
