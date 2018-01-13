/*
 * Copyright (c) 2011 Vojtech Horky
 * Copyright (c) 2013 Jan Vesely
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
		usb_log_error("USB device `%s' init failed (%s): %s.\n",
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
	if (driver->ops->device_rem == NULL)
		return ENOTSUP;
	/* Just tell the driver to stop whatever it is doing */
	usb_device_t *usb_dev = ddf_dev_data_get(gen_dev);
	const errno_t ret = driver->ops->device_rem(usb_dev);
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

static driver_ops_t generic_driver_ops = {
	.dev_add = generic_device_add,
	.dev_remove = generic_device_remove,
	.dev_gone = generic_device_gone,
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
