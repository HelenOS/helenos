/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @addtogroup uspace_drv_sun4v_con
 * @file Sun4v console driver
 * @{
 */

#include <ddf/driver.h>
#include <ddf/log.h>
#include <device/hw_res_parsed.h>
#include <errno.h>
#include <stdio.h>

#include "sun4v-con.h"

#define NAME  "sun4v-con"

static errno_t sun4v_con_dev_add(ddf_dev_t *dev);
static errno_t sun4v_con_dev_remove(ddf_dev_t *dev);
static errno_t sun4v_con_dev_gone(ddf_dev_t *dev);
static errno_t sun4v_con_fun_online(ddf_fun_t *fun);
static errno_t sun4v_con_fun_offline(ddf_fun_t *fun);

static driver_ops_t driver_ops = {
	.dev_add = sun4v_con_dev_add,
	.dev_remove = sun4v_con_dev_remove,
	.dev_gone = sun4v_con_dev_gone,
	.fun_online = sun4v_con_fun_online,
	.fun_offline = sun4v_con_fun_offline
};

static driver_t sun4v_con_driver = {
	.name = NAME,
	.driver_ops = &driver_ops
};

static errno_t sun4v_con_get_res(ddf_dev_t *dev, sun4v_con_res_t *res)
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

	res->in_base = RNGABS(hw_res.mem_ranges.ranges[0]);
	res->out_base = RNGABS(hw_res.mem_ranges.ranges[1]);
	return EOK;
error:
	hw_res_list_parsed_clean(&hw_res);
	return rc;
}

static errno_t sun4v_con_dev_add(ddf_dev_t *dev)
{
	sun4v_con_t *sun4v_con;
	sun4v_con_res_t res;
	errno_t rc;

	ddf_msg(LVL_DEBUG, "sun4v_con_dev_add(%p)", dev);
	sun4v_con = ddf_dev_data_alloc(dev, sizeof(sun4v_con_t));
	if (sun4v_con == NULL) {
		ddf_msg(LVL_ERROR, "Failed allocating soft state.");
		return ENOMEM;
	}

	sun4v_con->dev = dev;

	rc = sun4v_con_get_res(dev, &res);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed getting hardware resource list.\n");
		return EIO;
	}

	return sun4v_con_add(sun4v_con, &res);
}

static errno_t sun4v_con_dev_remove(ddf_dev_t *dev)
{
	sun4v_con_t *sun4v_con = (sun4v_con_t *)ddf_dev_data_get(dev);

	ddf_msg(LVL_DEBUG, "sun4v_con_dev_remove(%p)", dev);

	return sun4v_con_remove(sun4v_con);
}

static errno_t sun4v_con_dev_gone(ddf_dev_t *dev)
{
	sun4v_con_t *sun4v_con = (sun4v_con_t *)ddf_dev_data_get(dev);

	ddf_msg(LVL_DEBUG, "sun4v_con_dev_gone(%p)", dev);

	return sun4v_con_gone(sun4v_con);
}

static errno_t sun4v_con_fun_online(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "sun4v_con_fun_online()");
	return ddf_fun_online(fun);
}

static errno_t sun4v_con_fun_offline(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "sun4v_con_fun_offline()");
	return ddf_fun_offline(fun);
}

int main(int argc, char *argv[])
{
	printf(NAME ": Sun4v console driver\n");
	ddf_log_init(NAME);
	return ddf_driver_main(&sun4v_con_driver);
}

/** @}
 */
