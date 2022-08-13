/*
 * SPDX-FileCopyrightText: 2018 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup uspace_drv_gicv2
 * @{
 */
/** @file GICv2 driver.
 * @brief ARM Generic Interrupt Controller, Architecture version 2.0.
 *
 * This IRQ controller is present on the QEMU virt platform for ARM.
 */

#include <ddf/driver.h>
#include <ddf/log.h>
#include <device/hw_res_parsed.h>
#include <errno.h>
#include <stdio.h>

#include "gicv2.h"

#define NAME  "gicv2"

static errno_t gicv2_dev_add(ddf_dev_t *dev);
static errno_t gicv2_dev_remove(ddf_dev_t *dev);
static errno_t gicv2_dev_gone(ddf_dev_t *dev);
static errno_t gicv2_fun_online(ddf_fun_t *fun);
static errno_t gicv2_fun_offline(ddf_fun_t *fun);

static driver_ops_t driver_ops = {
	.dev_add = gicv2_dev_add,
	.dev_remove = gicv2_dev_remove,
	.dev_gone = gicv2_dev_gone,
	.fun_online = gicv2_fun_online,
	.fun_offline = gicv2_fun_offline
};

static driver_t gicv2_driver = {
	.name = NAME,
	.driver_ops = &driver_ops
};

static errno_t gicv2_get_res(ddf_dev_t *dev, gicv2_res_t *res)
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

	if (hw_res.mem_ranges.count != 2) {
		rc = EINVAL;
		goto error;
	}

	res->distr_base = RNGABS(hw_res.mem_ranges.ranges[0]);
	res->cpui_base = RNGABS(hw_res.mem_ranges.ranges[1]);

	return EOK;
error:
	hw_res_list_parsed_clean(&hw_res);
	return rc;
}

static errno_t gicv2_dev_add(ddf_dev_t *dev)
{
	gicv2_t *gicv2;
	gicv2_res_t gicv2_res;
	errno_t rc;

	ddf_msg(LVL_DEBUG, "gicv2_dev_add(%p)", dev);
	gicv2 = ddf_dev_data_alloc(dev, sizeof(gicv2_t));
	if (gicv2 == NULL) {
		ddf_msg(LVL_ERROR, "Failed allocating soft state.");
		return ENOMEM;
	}

	gicv2->dev = dev;

	rc = gicv2_get_res(dev, &gicv2_res);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed getting hardware resource list.\n");
		return EIO;
	}

	return gicv2_add(gicv2, &gicv2_res);
}

static errno_t gicv2_dev_remove(ddf_dev_t *dev)
{
	gicv2_t *gicv2 = (gicv2_t *)ddf_dev_data_get(dev);

	ddf_msg(LVL_DEBUG, "gicv2_dev_remove(%p)", dev);

	return gicv2_remove(gicv2);
}

static errno_t gicv2_dev_gone(ddf_dev_t *dev)
{
	gicv2_t *gicv2 = (gicv2_t *)ddf_dev_data_get(dev);

	ddf_msg(LVL_DEBUG, "gicv2_dev_gone(%p)", dev);

	return gicv2_gone(gicv2);
}

static errno_t gicv2_fun_online(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "gicv2_fun_online()");
	return ddf_fun_online(fun);
}

static errno_t gicv2_fun_offline(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "gicv2_fun_offline()");
	return ddf_fun_offline(fun);
}

int main(int argc, char *argv[])
{
	printf(NAME ": GICv2 interrupt controller driver\n");
	ddf_log_init(NAME);
	return ddf_driver_main(&gicv2_driver);
}

/** @}
 */
