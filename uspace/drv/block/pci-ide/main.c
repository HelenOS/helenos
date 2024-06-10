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

/** @file
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <str_error.h>
#include <ddf/driver.h>
#include <ddf/log.h>
#include <device/hw_res_parsed.h>

#include "pci-ide.h"
#include "pci-ide_hw.h"
#include "main.h"

static errno_t pci_ide_dev_add(ddf_dev_t *dev);
static errno_t pci_ide_dev_remove(ddf_dev_t *dev);
static errno_t pci_ide_dev_gone(ddf_dev_t *dev);
static errno_t pci_ide_fun_online(ddf_fun_t *fun);
static errno_t pci_ide_fun_offline(ddf_fun_t *fun);

static void pci_ide_connection(ipc_call_t *, void *);

static driver_ops_t driver_ops = {
	.dev_add = &pci_ide_dev_add,
	.dev_remove = &pci_ide_dev_remove,
	.dev_gone = &pci_ide_dev_gone,
	.fun_online = &pci_ide_fun_online,
	.fun_offline = &pci_ide_fun_offline
};

static driver_t pci_ide_driver = {
	.name = NAME,
	.driver_ops = &driver_ops
};

static errno_t pci_ide_get_res(ddf_dev_t *dev, pci_ide_hwres_t *res)
{
	async_sess_t *parent_sess;
	hw_res_list_parsed_t hw_res;
	errno_t rc;

	parent_sess = ddf_dev_parent_sess_get(dev);
	if (parent_sess == NULL)
		return ENOMEM;

	hw_res_list_parsed_init(&hw_res);
	rc = hw_res_get_list_parsed(parent_sess, &hw_res, 0);
	if (rc != EOK)
		return rc;

	if (hw_res.io_ranges.count != 1) {
		rc = EINVAL;
		goto error;
	}

	/* Legacty ISA I/O ranges are fixed */

	res->cmd1 = pci_ide_ata_cmd_p;
	res->ctl1 = pci_ide_ata_ctl_p;
	res->cmd2 = pci_ide_ata_cmd_s;
	res->ctl2 = pci_ide_ata_ctl_s;

	/* PCI I/O range */
	addr_range_t *bmregs_rng = &hw_res.io_ranges.ranges[0];
	res->bmregs = RNGABS(*bmregs_rng);

	ddf_msg(LVL_NOTE, "sizes: %zu", RNGSZ(*bmregs_rng));

	if (RNGSZ(*bmregs_rng) < sizeof(pci_ide_regs_t)) {
		rc = EINVAL;
		goto error;
	}

	/* IRQ */
	if (hw_res.irqs.count > 0) {
		res->irq1 = hw_res.irqs.irqs[0];
	} else {
		res->irq1 = -1;
	}

	if (hw_res.irqs.count > 1) {
		res->irq2 = hw_res.irqs.irqs[1];
	} else {
		res->irq2 = -1;
	}

	return EOK;
error:
	hw_res_list_parsed_clean(&hw_res);
	return rc;
}

/** Add new device
 *
 * @param  dev New device
 * @return     EOK on success or an error code.
 */
static errno_t pci_ide_dev_add(ddf_dev_t *dev)
{
	pci_ide_ctrl_t *ctrl;
	pci_ide_hwres_t res;
	errno_t rc;

	rc = pci_ide_get_res(dev, &res);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Invalid HW resource configuration.");
		return EINVAL;
	}

	ctrl = ddf_dev_data_alloc(dev, sizeof(pci_ide_ctrl_t));
	if (ctrl == NULL) {
		ddf_msg(LVL_ERROR, "Failed allocating soft state.");
		rc = ENOMEM;
		goto error;
	}

	ctrl->dev = dev;

	rc = pci_ide_ctrl_init(ctrl, &res);
	if (rc != EOK)
		goto error;

	rc = pci_ide_channel_init(ctrl, &ctrl->channel[0], 0, &res);
	if (rc == ENOENT)
		goto error;

	rc = pci_ide_channel_init(ctrl, &ctrl->channel[1], 1, &res);
	if (rc == ENOENT)
		goto error;

	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed initializing ATA controller.");
		rc = EIO;
		goto error;
	}

	return EOK;
error:
	return rc;
}

static char *pci_ide_fun_name(pci_ide_channel_t *chan, unsigned idx)
{
	char *fun_name;

	if (asprintf(&fun_name, "c%ud%u", chan->chan_id, idx) < 0)
		return NULL;

	return fun_name;
}

errno_t pci_ide_fun_create(pci_ide_channel_t *chan, unsigned idx, void *charg)
{
	errno_t rc;
	char *fun_name = NULL;
	ddf_fun_t *fun = NULL;
	pci_ide_fun_t *ifun = NULL;
	bool bound = false;

	fun_name = pci_ide_fun_name(chan, idx);
	if (fun_name == NULL) {
		ddf_msg(LVL_ERROR, "Out of memory.");
		rc = ENOMEM;
		goto error;
	}

	fun = ddf_fun_create(chan->ctrl->dev, fun_exposed, fun_name);
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating DDF function.");
		rc = ENOMEM;
		goto error;
	}

	/* Allocate soft state */
	ifun = ddf_fun_data_alloc(fun, sizeof(pci_ide_fun_t));
	if (ifun == NULL) {
		ddf_msg(LVL_ERROR, "Failed allocating softstate.");
		rc = ENOMEM;
		goto error;
	}

	ifun->fun = fun;
	ifun->charg = charg;

	/* Set up a connection handler. */
	ddf_fun_set_conn_handler(fun, pci_ide_connection);

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding DDF function %s: %s",
		    fun_name, str_error(rc));
		goto error;
	}

	bound = true;

	rc = ddf_fun_add_to_category(fun, "disk");
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed adding function %s to "
		    "category 'disk': %s", fun_name, str_error(rc));
		goto error;
	}

	free(fun_name);
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

errno_t pci_ide_fun_remove(pci_ide_channel_t *chan, unsigned idx)
{
	errno_t rc;
	char *fun_name;
	pci_ide_fun_t *ifun = chan->fun[idx];

	fun_name = pci_ide_fun_name(chan, idx);
	if (fun_name == NULL) {
		ddf_msg(LVL_ERROR, "Out of memory.");
		rc = ENOMEM;
		goto error;
	}

	ddf_msg(LVL_DEBUG, "pci_ide_fun_remove(%p, '%s')", ifun, fun_name);
	rc = ddf_fun_offline(ifun->fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error offlining function '%s'.", fun_name);
		goto error;
	}

	rc = ddf_fun_unbind(ifun->fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed unbinding function '%s'.", fun_name);
		goto error;
	}

	ddf_fun_destroy(ifun->fun);
	free(fun_name);
	return EOK;
error:
	if (fun_name != NULL)
		free(fun_name);
	return rc;
}

errno_t pci_ide_fun_unbind(pci_ide_channel_t *chan, unsigned idx)
{
	errno_t rc;
	char *fun_name;
	pci_ide_fun_t *ifun = chan->fun[idx];

	fun_name = pci_ide_fun_name(chan, idx);
	if (fun_name == NULL) {
		ddf_msg(LVL_ERROR, "Out of memory.");
		rc = ENOMEM;
		goto error;
	}

	ddf_msg(LVL_DEBUG, "pci_ide_fun_unbind(%p, '%s')", ifun, fun_name);
	rc = ddf_fun_unbind(ifun->fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed unbinding function '%s'.", fun_name);
		goto error;
	}

	ddf_fun_destroy(ifun->fun);
	free(fun_name);
	return EOK;
error:
	if (fun_name != NULL)
		free(fun_name);
	return rc;
}

static errno_t pci_ide_dev_remove(ddf_dev_t *dev)
{
	pci_ide_ctrl_t *ctrl = (pci_ide_ctrl_t *)ddf_dev_data_get(dev);
	errno_t rc;

	ddf_msg(LVL_DEBUG, "pci_ide_dev_remove(%p)", dev);

	rc = pci_ide_channel_fini(&ctrl->channel[0]);
	if (rc != EOK)
		return rc;

	rc = pci_ide_channel_fini(&ctrl->channel[1]);
	if (rc != EOK)
		return rc;

	return EOK;
}

static errno_t pci_ide_dev_gone(ddf_dev_t *dev)
{
	pci_ide_ctrl_t *ctrl = (pci_ide_ctrl_t *)ddf_dev_data_get(dev);
	errno_t rc;

	ddf_msg(LVL_DEBUG, "pci_ide_dev_gone(%p)", dev);

	rc = pci_ide_ctrl_fini(ctrl);
	if (rc != EOK)
		return rc;

	rc = pci_ide_channel_fini(&ctrl->channel[0]);
	if (rc != EOK)
		return rc;

	rc = pci_ide_channel_fini(&ctrl->channel[1]);
	if (rc != EOK)
		return rc;

	return EOK;
}

static errno_t pci_ide_fun_online(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "pci_ide_fun_online()");
	return ddf_fun_online(fun);
}

static errno_t pci_ide_fun_offline(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "pci_ide_fun_offline()");
	return ddf_fun_offline(fun);
}

static void pci_ide_connection(ipc_call_t *icall, void *arg)
{
	pci_ide_fun_t *ifun;

	ifun = (pci_ide_fun_t *) ddf_fun_data_get((ddf_fun_t *)arg);
	ata_connection(icall, ifun->charg);
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS PCI IDE device driver\n");
	ddf_log_init(NAME);
	return ddf_driver_main(&pci_ide_driver);
}

/**
 * @}
 */
