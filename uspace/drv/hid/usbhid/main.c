/*
 * Copyright (c) 2010 Vojtech Horky
 * Copyright (c) 2011 Lubos Slovak
 * Copyright (c) 2018 Petr Manek, Ondrej Hlavaty
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

/** @addtogroup drvusbhid
 * @{
 */
/**
 * @file
 * Main routines of USB HID driver.
 */

#include <ddf/driver.h>
#include <usb/debug.h>
#include <errno.h>
#include <str_error.h>

#include <usb/dev/driver.h>
#include <usb/dev/poll.h>

#include "usbhid.h"

#define NAME "usbhid"

/**
 * Callback for passing a new device to the driver.
 *
 * @note Currently, only boot-protocol keyboards are supported by this driver.
 *
 * @param dev Structure representing the new device.
 * @return Error code.
 */
static errno_t usb_hid_device_add(usb_device_t *dev)
{
	usb_log_debug("%s", __FUNCTION__);

	if (dev == NULL) {
		usb_log_error("Wrong parameter given for add_device().");
		return EINVAL;
	}

	if (usb_device_get_iface_number(dev) < 0) {
		usb_log_error("Failed to add HID device: endpoints not found."
		    "\n");
		return ENOTSUP;
	}
	usb_hid_dev_t *hid_dev =
	    usb_device_data_alloc(dev, sizeof(usb_hid_dev_t));
	if (hid_dev == NULL) {
		usb_log_error("Failed to create USB/HID device structure.");
		return ENOMEM;
	}

	errno_t rc = usb_hid_init(hid_dev, dev);
	if (rc != EOK) {
		usb_log_error("Failed to initialize USB/HID device.");
		usb_hid_deinit(hid_dev);
		return rc;
	}

	usb_log_debug("USB/HID device structure initialized.");

	/*
	 * Start automated polling function.
	 * This will create a separate fibril that will query the device
	 * for the data continuously.
	 */
	rc = usb_polling_start(&hid_dev->polling);

	if (rc != EOK) {
		usb_log_error("Failed to start polling fibril for `%s'.",
		    usb_device_get_name(dev));
		usb_hid_deinit(hid_dev);
		return rc;
	}
	hid_dev->running = true;

	usb_log_info("HID device `%s' ready.", usb_device_get_name(dev));

	return EOK;
}

static errno_t join_and_clean(usb_device_t *dev)
{
	assert(dev);
	usb_hid_dev_t *hid_dev = usb_device_data_get(dev);
	assert(hid_dev);

	/* Join polling fibril (ignoring error code). */
	usb_polling_join(&hid_dev->polling);

	/* Clean up. */
	usb_hid_deinit(hid_dev);
	usb_log_info("%s destruction complete.", usb_device_get_name(dev));

	return EOK;
}

/**
 * Callback for a device about to be removed from the driver.
 *
 * @param dev Structure representing the device.
 * @return Error code.
 */
static errno_t usb_hid_device_remove(usb_device_t *dev)
{
	assert(dev);
	usb_hid_dev_t *hid_dev = usb_device_data_get(dev);
	assert(hid_dev);

	usb_log_info("Device %s removed.", usb_device_get_name(dev));
	return join_and_clean(dev);
}

/**
 * Callback for removing a device from the driver.
 *
 * @param dev Structure representing the device.
 * @return Error code.
 */
static errno_t usb_hid_device_gone(usb_device_t *dev)
{
	assert(dev);
	usb_hid_dev_t *hid_dev = usb_device_data_get(dev);
	assert(hid_dev);

	usb_log_info("Device %s gone.", usb_device_get_name(dev));
	return join_and_clean(dev);
}

/** USB generic driver callbacks */
static const usb_driver_ops_t usb_hid_driver_ops = {
	.device_add = usb_hid_device_add,
	.device_remove = usb_hid_device_remove,
	.device_gone = usb_hid_device_gone,
};

/** The driver itself. */
static const usb_driver_t usb_hid_driver = {
	.name = NAME,
	.ops = &usb_hid_driver_ops,
	.endpoints = usb_hid_endpoints
};

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS USB HID driver.\n");

	log_init(NAME);

	return usb_driver_main(&usb_hid_driver);
}
/**
 * @}
 */
