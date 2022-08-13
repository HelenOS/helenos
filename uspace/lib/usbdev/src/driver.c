/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 * SPDX-FileCopyrightText: 2013 Jan Vesely
 * SPDX-FileCopyrightText: 2018 Petr Manek
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup libusbdev
 * @{
 */
/** @file
 * USB device driver framework.
 */

#include <usb/dev/driver.h>
#include <usb/dev/device.h>
#include <usb/debug.h>

#include <assert.h>
#include <errno.h>
#include <str_error.h>
#include <ddf/driver.h>

static const usb_driver_t *driver = NULL;

/** Callback when a new device is supposed to be controlled by this driver.
 *
 * This callback is a wrapper for USB specific version of @c device_add.
 *
 * @param gen_dev Device structure as prepared by DDF.
 * @return Error code.
 */
static errno_t generic_device_add(ddf_dev_t *gen_dev)
{
	assert(driver);
	assert(driver->ops);
	assert(driver->ops->device_add);

	/* Initialize generic USB driver data. */
	const char *err_msg = NULL;
	errno_t rc = usb_device_create_ddf(gen_dev, driver->endpoints, &err_msg);
	if (rc != EOK) {
		usb_log_error("USB device `%s' init failed (%s): %s.",
		    ddf_dev_get_name(gen_dev), err_msg, str_error(rc));
		return rc;
	}

	/* Start USB driver specific initialization. */
	rc = driver->ops->device_add(ddf_dev_data_get(gen_dev));
	if (rc != EOK)
		usb_device_destroy_ddf(gen_dev);
	return rc;
}

/** Callback when a device is supposed to be removed from the system.
 *
 * This callback is a wrapper for USB specific version of @c device_remove.
 *
 * @param gen_dev Device structure as prepared by DDF.
 * @return Error code.
 */
static errno_t generic_device_remove(ddf_dev_t *gen_dev)
{
	assert(driver);
	assert(driver->ops);
	if (driver->ops->device_remove == NULL)
		return ENOTSUP;

	/* Just tell the driver to stop whatever it is doing */
	usb_device_t *usb_dev = ddf_dev_data_get(gen_dev);
	const errno_t ret = driver->ops->device_remove(usb_dev);
	if (ret != EOK)
		return ret;

	usb_device_destroy_ddf(gen_dev);
	return EOK;
}

/** Callback when a device was removed from the system.
 *
 * This callback is a wrapper for USB specific version of @c device_gone.
 *
 * @param gen_dev Device structure as prepared by DDF.
 * @return Error code.
 */
static errno_t generic_device_gone(ddf_dev_t *gen_dev)
{
	assert(driver);
	assert(driver->ops);
	if (driver->ops->device_gone == NULL)
		return ENOTSUP;
	usb_device_t *usb_dev = ddf_dev_data_get(gen_dev);
	const errno_t ret = driver->ops->device_gone(usb_dev);
	if (ret == EOK)
		usb_device_destroy_ddf(gen_dev);

	return ret;
}

/** Callback when the driver is asked to online a specific function.
 *
 * This callback is a wrapper for USB specific version of @c fun_online.
 *
 * @param gen_dev Device function structure as prepared by DDF.
 * @return Error code.
 */
static int generic_function_online(ddf_fun_t *fun)
{
	assert(driver);
	assert(driver->ops);
	if (driver->ops->function_online == NULL)
		return ENOTSUP;
	return driver->ops->function_online(fun);
}

/** Callback when the driver is asked to offline a specific function.
 *
 * This callback is a wrapper for USB specific version of @c fun_offline.
 *
 * @param gen_dev Device function structure as prepared by DDF.
 * @return Error code.
 */
static int generic_function_offline(ddf_fun_t *fun)
{
	assert(driver);
	assert(driver->ops);
	if (driver->ops->function_offline == NULL)
		return ENOTSUP;
	return driver->ops->function_offline(fun);
}

static driver_ops_t generic_driver_ops = {
	.dev_add = generic_device_add,
	.dev_remove = generic_device_remove,
	.dev_gone = generic_device_gone,
	.fun_online = generic_function_online,
	.fun_offline = generic_function_offline,
};
static driver_t generic_driver = {
	.driver_ops = &generic_driver_ops
};

/** Main routine of USB device driver.
 *
 * Under normal conditions, this function never returns.
 *
 * @param drv USB device driver structure.
 * @return Task exit status.
 */
int usb_driver_main(const usb_driver_t *drv)
{
	assert(drv != NULL);

	/* Prepare the generic driver. */
	generic_driver.name = drv->name;

	driver = drv;

	return ddf_driver_main(&generic_driver);
}

/**
 * @}
 */
