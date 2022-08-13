/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup uspace_drv_adb_mouse
 * @{
 */
/** @file ADB mouse driver
 */

#include <ddf/driver.h>
#include <ddf/log.h>
#include <errno.h>
#include <stdio.h>

#include "adb-mouse.h"

#define NAME  "adb-mouse"

static errno_t adb_mouse_dev_add(ddf_dev_t *dev);
static errno_t adb_mouse_dev_remove(ddf_dev_t *dev);
static errno_t adb_mouse_dev_gone(ddf_dev_t *dev);
static errno_t adb_mouse_fun_online(ddf_fun_t *fun);
static errno_t adb_mouse_fun_offline(ddf_fun_t *fun);

static driver_ops_t driver_ops = {
	.dev_add = adb_mouse_dev_add,
	.dev_remove = adb_mouse_dev_remove,
	.dev_gone = adb_mouse_dev_gone,
	.fun_online = adb_mouse_fun_online,
	.fun_offline = adb_mouse_fun_offline
};

static driver_t adb_mouse_driver = {
	.name = NAME,
	.driver_ops = &driver_ops
};

static errno_t adb_mouse_dev_add(ddf_dev_t *dev)
{
	adb_mouse_t *adb_mouse;

	ddf_msg(LVL_DEBUG, "adb_mouse_dev_add(%p)", dev);
	adb_mouse = ddf_dev_data_alloc(dev, sizeof(adb_mouse_t));
	if (adb_mouse == NULL) {
		ddf_msg(LVL_ERROR, "Failed allocating soft state.");
		return ENOMEM;
	}

	adb_mouse->dev = dev;

	return adb_mouse_add(adb_mouse);
}

static errno_t adb_mouse_dev_remove(ddf_dev_t *dev)
{
	adb_mouse_t *adb_mouse = (adb_mouse_t *)ddf_dev_data_get(dev);

	ddf_msg(LVL_DEBUG, "adb_mouse_dev_remove(%p)", dev);

	return adb_mouse_remove(adb_mouse);
}

static errno_t adb_mouse_dev_gone(ddf_dev_t *dev)
{
	adb_mouse_t *adb_mouse = (adb_mouse_t *)ddf_dev_data_get(dev);

	ddf_msg(LVL_DEBUG, "adb_mouse_dev_gone(%p)", dev);

	return adb_mouse_gone(adb_mouse);
}

static errno_t adb_mouse_fun_online(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "adb_mouse_fun_online()");
	return ddf_fun_online(fun);
}

static errno_t adb_mouse_fun_offline(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "adb_mouse_fun_offline()");
	return ddf_fun_offline(fun);
}

int main(int argc, char *argv[])
{
	printf(NAME ": ADB mouse driver\n");
	ddf_log_init(NAME);
	return ddf_driver_main(&adb_mouse_driver);
}

/** @}
 */
