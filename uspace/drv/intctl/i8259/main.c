/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup i8259
 * @{
 */
/** @file i8259 Interrupt Controller driver
 */

#include <ddf/driver.h>
#include <ddf/log.h>
#include <device/hw_res_parsed.h>
#include <errno.h>
#include <stdio.h>

#include "i8259.h"

#define NAME  "i8259"

static errno_t i8259_dev_add(ddf_dev_t *dev);
static errno_t i8259_dev_remove(ddf_dev_t *dev);
static errno_t i8259_dev_gone(ddf_dev_t *dev);
static errno_t i8259_fun_online(ddf_fun_t *fun);
static errno_t i8259_fun_offline(ddf_fun_t *fun);

static driver_ops_t driver_ops = {
	.dev_add = i8259_dev_add,
	.dev_remove = i8259_dev_remove,
	.dev_gone = i8259_dev_gone,
	.fun_online = i8259_fun_online,
	.fun_offline = i8259_fun_offline
};

static driver_t i8259_driver = {
	.name = NAME,
	.driver_ops = &driver_ops
};

static errno_t i8259_get_res(ddf_dev_t *dev, i8259_res_t *res)
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

	res->base0 = RNGABS(hw_res.mem_ranges.ranges[0]);
	res->base1 = RNGABS(hw_res.mem_ranges.ranges[1]);

	return EOK;
error:
	hw_res_list_parsed_clean(&hw_res);
	return rc;
}

static errno_t i8259_dev_add(ddf_dev_t *dev)
{
	i8259_t *i8259;
	i8259_res_t i8259_res;
	errno_t rc;

	ddf_msg(LVL_DEBUG, "i8259_dev_add(%p)", dev);
	i8259 = ddf_dev_data_alloc(dev, sizeof(i8259_t));
	if (i8259 == NULL) {
		ddf_msg(LVL_ERROR, "Failed allocating soft state.");
		return ENOMEM;
	}

	i8259->dev = dev;

	rc = i8259_get_res(dev, &i8259_res);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed getting hardware resource list.\n");
		return EIO;
	}

	return i8259_add(i8259, &i8259_res);
}

static errno_t i8259_dev_remove(ddf_dev_t *dev)
{
	i8259_t *i8259 = (i8259_t *)ddf_dev_data_get(dev);

	ddf_msg(LVL_DEBUG, "i8259_dev_remove(%p)", dev);

	return i8259_remove(i8259);
}

static errno_t i8259_dev_gone(ddf_dev_t *dev)
{
	i8259_t *i8259 = (i8259_t *)ddf_dev_data_get(dev);

	ddf_msg(LVL_DEBUG, "i8259_dev_gone(%p)", dev);

	return i8259_gone(i8259);
}

static errno_t i8259_fun_online(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "i8259_fun_online()");
	return ddf_fun_online(fun);
}

static errno_t i8259_fun_offline(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "i8259_fun_offline()");
	return ddf_fun_offline(fun);
}

int main(int argc, char *argv[])
{
	printf(NAME ": i8259 Interrupt Controller driver\n");
	ddf_log_init(NAME);
	return ddf_driver_main(&i8259_driver);
}

/** @}
 */
