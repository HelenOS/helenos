/*
 * Copyright (c) 2011 Vojtech Horky
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

/** @addtogroup drvusbfallback
 * @{
 */
/**
 * @file
 * Main routines of USB fallback driver.
 */
#include <usb/dev/driver.h>
#include <usb/debug.h>
#include <errno.h>
#include <str_error.h>

#define NAME "usbflbk"

/** Callback when new device is attached and recognized by DDF.
 *
 * @param dev Representation of a generic DDF device.
 * @return Error code.
 */
static int usbfallback_device_add(usb_device_t *dev)
{
	int rc;
	const char *fun_name = "ctl";

	ddf_fun_t *ctl_fun = ddf_fun_create(dev->ddf_dev, fun_exposed,
	    fun_name);
	if (ctl_fun == NULL) {
		usb_log_error("Failed to create control function.\n");
		return ENOMEM;
	}
	rc = ddf_fun_bind(ctl_fun);
	if (rc != EOK) {
		usb_log_error("Failed to bind control function: %s.\n",
		    str_error(rc));
		return rc;
	}

	dev->driver_data = ctl_fun;

	usb_log_info("Pretending to control %s `%s'" \
	    " (node `%s', handle %" PRIun ").\n",
	    dev->interface_no < 0 ? "device" : "interface",
	    ddf_dev_get_name(dev->ddf_dev), fun_name, ddf_dev_get_handle(dev->ddf_dev));

	return EOK;
}

/** Callback when new device is removed and recognized as gone by DDF.
 *
 * @param dev Representation of a generic DDF device.
 * @return Error code.
 */
static int usbfallback_device_gone(usb_device_t *dev)
{
	assert(dev);
	ddf_fun_t *ctl_fun = dev->driver_data;
	const int ret = ddf_fun_unbind(ctl_fun);
	if (ret != EOK) {
		usb_log_error("Failed to unbind %s.\n", ddf_fun_get_name(ctl_fun));
		return ret;
	}
	ddf_fun_destroy(ctl_fun);
	dev->driver_data = NULL;

	return EOK;
}

/** USB fallback driver ops. */
static const usb_driver_ops_t usbfallback_driver_ops = {
	.device_add = usbfallback_device_add,
	.device_rem = usbfallback_device_gone,
	.device_gone = usbfallback_device_gone,
};

/** USB fallback driver. */
static const usb_driver_t usbfallback_driver = {
	.name = NAME,
	.ops = &usbfallback_driver_ops,
	.endpoints = NULL
};

int main(int argc, char *argv[])
{
	log_init(NAME);

	return usb_driver_main(&usbfallback_driver);
}

/**
 * @}
 */
