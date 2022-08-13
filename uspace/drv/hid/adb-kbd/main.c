/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup uspace_drv_adb_kbd
 * @{
 */
/** @file ADB keyboard driver
 */

#include <ddf/driver.h>
#include <ddf/log.h>
#include <errno.h>
#include <stdio.h>

#include "adb-kbd.h"

#define NAME  "adb-kbd"

static errno_t adb_kbd_dev_add(ddf_dev_t *dev);
static errno_t adb_kbd_dev_remove(ddf_dev_t *dev);
static errno_t adb_kbd_dev_gone(ddf_dev_t *dev);
static errno_t adb_kbd_fun_online(ddf_fun_t *fun);
static errno_t adb_kbd_fun_offline(ddf_fun_t *fun);

static driver_ops_t driver_ops = {
	.dev_add = adb_kbd_dev_add,
	.dev_remove = adb_kbd_dev_remove,
	.dev_gone = adb_kbd_dev_gone,
	.fun_online = adb_kbd_fun_online,
	.fun_offline = adb_kbd_fun_offline
};

static driver_t adb_kbd_driver = {
	.name = NAME,
	.driver_ops = &driver_ops
};

static errno_t adb_kbd_dev_add(ddf_dev_t *dev)
{
	adb_kbd_t *adb_kbd;

	ddf_msg(LVL_DEBUG, "adb_kbd_dev_add(%p)", dev);
	adb_kbd = ddf_dev_data_alloc(dev, sizeof(adb_kbd_t));
	if (adb_kbd == NULL) {
		ddf_msg(LVL_ERROR, "Failed allocating soft state.");
		return ENOMEM;
	}

	adb_kbd->dev = dev;

	return adb_kbd_add(adb_kbd);
}

static errno_t adb_kbd_dev_remove(ddf_dev_t *dev)
{
	adb_kbd_t *adb_kbd = (adb_kbd_t *)ddf_dev_data_get(dev);

	ddf_msg(LVL_DEBUG, "adb_kbd_dev_remove(%p)", dev);

	return adb_kbd_remove(adb_kbd);
}

static errno_t adb_kbd_dev_gone(ddf_dev_t *dev)
{
	adb_kbd_t *adb_kbd = (adb_kbd_t *)ddf_dev_data_get(dev);

	ddf_msg(LVL_DEBUG, "adb_kbd_dev_gone(%p)", dev);

	return adb_kbd_gone(adb_kbd);
}

static errno_t adb_kbd_fun_online(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "adb_kbd_fun_online()");
	return ddf_fun_online(fun);
}

static errno_t adb_kbd_fun_offline(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "adb_kbd_fun_offline()");
	return ddf_fun_offline(fun);
}

int main(int argc, char *argv[])
{
	printf(NAME ": ADB keyboard driver\n");
	ddf_log_init(NAME);
	return ddf_driver_main(&adb_kbd_driver);
}

/** @}
 */
