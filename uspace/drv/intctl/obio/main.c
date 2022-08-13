/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup obio
 * @{
 */
/** @file OBIO driver
 */

#include <ddf/driver.h>
#include <ddf/log.h>
#include <device/hw_res_parsed.h>
#include <errno.h>
#include <stdio.h>

#include "obio.h"

#define NAME  "obio"

static errno_t obio_dev_add(ddf_dev_t *dev);
static errno_t obio_dev_remove(ddf_dev_t *dev);
static errno_t obio_dev_gone(ddf_dev_t *dev);
static errno_t obio_fun_online(ddf_fun_t *fun);
static errno_t obio_fun_offline(ddf_fun_t *fun);

static driver_ops_t driver_ops = {
	.dev_add = obio_dev_add,
	.dev_remove = obio_dev_remove,
	.dev_gone = obio_dev_gone,
	.fun_online = obio_fun_online,
	.fun_offline = obio_fun_offline
};

static driver_t obio_driver = {
	.name = NAME,
	.driver_ops = &driver_ops
};

static errno_t obio_get_res(ddf_dev_t *dev, obio_res_t *res)
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

	return EOK;
error:
	hw_res_list_parsed_clean(&hw_res);
	return rc;
}

static errno_t obio_dev_add(ddf_dev_t *dev)
{
	obio_t *obio;
	obio_res_t obio_res;
	errno_t rc;

	ddf_msg(LVL_DEBUG, "obio_dev_add(%p)", dev);
	obio = ddf_dev_data_alloc(dev, sizeof(obio_t));
	if (obio == NULL) {
		ddf_msg(LVL_ERROR, "Failed allocating soft state.");
		return ENOMEM;
	}

	obio->dev = dev;

	rc = obio_get_res(dev, &obio_res);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed getting hardware resource list.\n");
		return EIO;
	}

	return obio_add(obio, &obio_res);
}

static errno_t obio_dev_remove(ddf_dev_t *dev)
{
	obio_t *obio = (obio_t *)ddf_dev_data_get(dev);

	ddf_msg(LVL_DEBUG, "obio_dev_remove(%p)", dev);

	return obio_remove(obio);
}

static errno_t obio_dev_gone(ddf_dev_t *dev)
{
	obio_t *obio = (obio_t *)ddf_dev_data_get(dev);

	ddf_msg(LVL_DEBUG, "obio_dev_gone(%p)", dev);

	return obio_gone(obio);
}

static errno_t obio_fun_online(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "obio_fun_online()");
	return ddf_fun_online(fun);
}

static errno_t obio_fun_offline(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "obio_fun_offline()");
	return ddf_fun_offline(fun);
}

int main(int argc, char *argv[])
{
	printf(NAME ": OBIO driver\n");
	ddf_log_init(NAME);
	return ddf_driver_main(&obio_driver);
}

/** @}
 */
