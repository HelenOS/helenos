/*
 * Copyright (c) 2013 Jiri Svoboda
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
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <str_error.h>
#include <ddf/driver.h>
#include <ddf/log.h>
#include <device/hw_res_parsed.h>

#include "ata_bd.h"
#include "main.h"

static errno_t ata_dev_add(ddf_dev_t *dev);
static errno_t ata_dev_remove(ddf_dev_t *dev);
static errno_t ata_dev_gone(ddf_dev_t *dev);
static errno_t ata_fun_online(ddf_fun_t *fun);
static errno_t ata_fun_offline(ddf_fun_t *fun);

static void ata_bd_connection(cap_call_handle_t, ipc_call_t *, void *);

static driver_ops_t driver_ops = {
	.dev_add = &ata_dev_add,
	.dev_remove = &ata_dev_remove,
	.dev_gone = &ata_dev_gone,
	.fun_online = &ata_fun_online,
	.fun_offline = &ata_fun_offline
};

static driver_t ata_driver = {
	.name = NAME,
	.driver_ops = &driver_ops
};

static errno_t ata_get_res(ddf_dev_t *dev, ata_base_t *ata_res)
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

	if (hw_res.io_ranges.count != 2) {
		rc = EINVAL;
		goto error;
	}

	addr_range_t *cmd_rng = &hw_res.io_ranges.ranges[0];
	addr_range_t *ctl_rng = &hw_res.io_ranges.ranges[1];
	ata_res->cmd = RNGABS(*cmd_rng);
	ata_res->ctl = RNGABS(*ctl_rng);

	if (RNGSZ(*ctl_rng) < sizeof(ata_ctl_t)) {
		rc = EINVAL;
		goto error;
	}

	if (RNGSZ(*cmd_rng) < sizeof(ata_cmd_t)) {
		rc = EINVAL;
		goto error;
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
static errno_t ata_dev_add(ddf_dev_t *dev)
{
	ata_ctrl_t *ctrl;
	ata_base_t res;
	errno_t rc;

	rc = ata_get_res(dev, &res);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Invalid HW resource configuration.");
		return EINVAL;
	}

	ctrl = ddf_dev_data_alloc(dev, sizeof(ata_ctrl_t));
	if (ctrl == NULL) {
		ddf_msg(LVL_ERROR, "Failed allocating soft state.");
		rc = ENOMEM;
		goto error;
	}

	ctrl->dev = dev;

	rc = ata_ctrl_init(ctrl, &res);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed initializing ATA controller.");
		rc = EIO;
		goto error;
	}

	return EOK;
error:
	return rc;
}

static char *ata_fun_name(disk_t *disk)
{
	char *fun_name;

	if (asprintf(&fun_name, "d%u", disk->disk_id) < 0)
		return NULL;

	return fun_name;
}

errno_t ata_fun_create(disk_t *disk)
{
	ata_ctrl_t *ctrl = disk->ctrl;
	errno_t rc;
	char *fun_name = NULL;
	ddf_fun_t *fun = NULL;
	ata_fun_t *afun = NULL;

	fun_name = ata_fun_name(disk);
	if (fun_name == NULL) {
		ddf_msg(LVL_ERROR, "Out of memory.");
		rc = ENOMEM;
		goto error;
	}

	fun = ddf_fun_create(ctrl->dev, fun_exposed, fun_name);
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating DDF function.");
		rc = ENOMEM;
		goto error;
	}

	/* Allocate soft state */
	afun = ddf_fun_data_alloc(fun, sizeof(ata_fun_t));
	if (afun == NULL) {
		ddf_msg(LVL_ERROR, "Failed allocating softstate.");
		rc = ENOMEM;
		goto error;
	}

	afun->fun = fun;
	afun->disk = disk;

	bd_srvs_init(&afun->bds);
	afun->bds.ops = &ata_bd_ops;
	afun->bds.sarg = disk;

	/* Set up a connection handler. */
	ddf_fun_set_conn_handler(fun, ata_bd_connection);

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding DDF function %s: %s",
		    fun_name, str_error(rc));
		goto error;
	}

	ddf_fun_add_to_category(fun, "disk");

	free(fun_name);
	disk->afun = afun;
	return EOK;
error:
	if (fun != NULL)
		ddf_fun_destroy(fun);
	if (fun_name != NULL)
		free(fun_name);

	return rc;
}

errno_t ata_fun_remove(disk_t *disk)
{
	errno_t rc;
	char *fun_name;

	if (disk->afun == NULL)
		return EOK;

	fun_name = ata_fun_name(disk);
	if (fun_name == NULL) {
		ddf_msg(LVL_ERROR, "Out of memory.");
		rc = ENOMEM;
		goto error;
	}

	ddf_msg(LVL_DEBUG, "ata_fun_remove(%p, '%s')", disk, fun_name);
	rc = ddf_fun_offline(disk->afun->fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error offlining function '%s'.", fun_name);
		goto error;
	}

	rc = ddf_fun_unbind(disk->afun->fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed unbinding function '%s'.", fun_name);
		goto error;
	}

	ddf_fun_destroy(disk->afun->fun);
	disk->afun = NULL;
	free(fun_name);
	return EOK;
error:
	if (fun_name != NULL)
		free(fun_name);
	return rc;
}

errno_t ata_fun_unbind(disk_t *disk)
{
	errno_t rc;
	char *fun_name;

	if (disk->afun == NULL)
		return EOK;

	fun_name = ata_fun_name(disk);
	if (fun_name == NULL) {
		ddf_msg(LVL_ERROR, "Out of memory.");
		rc = ENOMEM;
		goto error;
	}

	ddf_msg(LVL_DEBUG, "ata_fun_unbind(%p, '%s')", disk, fun_name);
	rc = ddf_fun_unbind(disk->afun->fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed unbinding function '%s'.", fun_name);
		goto error;
	}

	ddf_fun_destroy(disk->afun->fun);
	disk->afun = NULL;
	free(fun_name);
	return EOK;
error:
	if (fun_name != NULL)
		free(fun_name);
	return rc;
}

static errno_t ata_dev_remove(ddf_dev_t *dev)
{
	ata_ctrl_t *ctrl = (ata_ctrl_t *)ddf_dev_data_get(dev);

	ddf_msg(LVL_DEBUG, "ata_dev_remove(%p)", dev);

	return ata_ctrl_remove(ctrl);
}

static errno_t ata_dev_gone(ddf_dev_t *dev)
{
	ata_ctrl_t *ctrl = (ata_ctrl_t *)ddf_dev_data_get(dev);

	ddf_msg(LVL_DEBUG, "ata_dev_gone(%p)", dev);

	return ata_ctrl_gone(ctrl);
}

static errno_t ata_fun_online(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "ata_fun_online()");
	return ddf_fun_online(fun);
}

static errno_t ata_fun_offline(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "ata_fun_offline()");
	return ddf_fun_offline(fun);
}

/** Block device connection handler */
static void ata_bd_connection(cap_call_handle_t icall_handle, ipc_call_t *icall, void *arg)
{
	ata_fun_t *afun;

	afun = (ata_fun_t *) ddf_fun_data_get((ddf_fun_t *)arg);
	bd_conn(icall_handle, icall, &afun->bds);
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS ATA(PI) device driver\n");
	ddf_log_init(NAME);
	return ddf_driver_main(&ata_driver);
}

