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

/** @addtogroup uspace_drv_dwapb
 * @{
 */
/** @file
 * @brief Synopsys DesignWare DW_apb_uart driver.
 */

#include <ddf/driver.h>
#include <ddf/log.h>
#include <device/hw_res_parsed.h>
#include <errno.h>
#include <stdio.h>

#include "dwapb.h"

#define NAME  "dwapb"

static errno_t dwapb_dev_add(ddf_dev_t *dev);
static errno_t dwapb_dev_remove(ddf_dev_t *dev);
static errno_t dwapb_dev_gone(ddf_dev_t *dev);
static errno_t dwapb_fun_online(ddf_fun_t *fun);
static errno_t dwapb_fun_offline(ddf_fun_t *fun);

static driver_ops_t driver_ops = {
	.dev_add = dwapb_dev_add,
	.dev_remove = dwapb_dev_remove,
	.dev_gone = dwapb_dev_gone,
	.fun_online = dwapb_fun_online,
	.fun_offline = dwapb_fun_offline
};

static driver_t dwapb_driver = {
	.name = NAME,
	.driver_ops = &driver_ops
};

static errno_t dwapb_get_res(ddf_dev_t *dev, dwapb_res_t *res)
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

	if (hw_res.mem_ranges.count != 1) {
		rc = EINVAL;
		goto error;
	}

	res->base = RNGABS(hw_res.mem_ranges.ranges[0]);

	if (hw_res.irqs.count != 1) {
		rc = EINVAL;
		goto error;
	}

	res->irq = hw_res.irqs.irqs[0];

	return EOK;
error:
	hw_res_list_parsed_clean(&hw_res);
	return rc;
}

static errno_t dwapb_dev_add(ddf_dev_t *dev)
{
	dwapb_t *dwapb;
	dwapb_res_t res;
	errno_t rc;

	ddf_msg(LVL_DEBUG, "dwapb_dev_add(%p)", dev);

	dwapb = ddf_dev_data_alloc(dev, sizeof(dwapb_t));
	if (dwapb == NULL) {
		ddf_msg(LVL_ERROR, "Failed allocating soft state.");
		return ENOMEM;
	}

	dwapb->dev = dev;

	rc = dwapb_get_res(dev, &res);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed getting hardware resource list.");
		return EIO;
	}

	return dwapb_add(dwapb, &res);
}

static errno_t dwapb_dev_remove(ddf_dev_t *dev)
{
	dwapb_t *dwapb = (dwapb_t *)ddf_dev_data_get(dev);

	ddf_msg(LVL_DEBUG, "dwapb_dev_remove(%p)", dev);

	return dwapb_remove(dwapb);
}

static errno_t dwapb_dev_gone(ddf_dev_t *dev)
{
	dwapb_t *dwapb = (dwapb_t *)ddf_dev_data_get(dev);

	ddf_msg(LVL_DEBUG, "dwapb_dev_gone(%p)", dev);

	return dwapb_gone(dwapb);
}

static errno_t dwapb_fun_online(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "dwapb_fun_online()");
	return ddf_fun_online(fun);
}

static errno_t dwapb_fun_offline(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "dwapb_fun_offline()");
	return ddf_fun_offline(fun);
}

int main(int argc, char *argv[])
{
	printf(NAME ": dwapb serial device driver\n");
	ddf_log_init(NAME);
	return ddf_driver_main(&dwapb_driver);
}

/** @}
 */
