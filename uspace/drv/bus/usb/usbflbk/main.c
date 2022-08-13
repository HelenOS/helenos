/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 * SPDX-FileCopyrightText: 2018 Petr Manek
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
static errno_t usbfallback_device_add(usb_device_t *dev)
{
	usb_log_info("Pretending to control %s `%s'.",
	    usb_device_get_iface_number(dev) < 0 ? "device" : "interface",
	    usb_device_get_name(dev));
	return EOK;
}

/** Callback when new device is removed and recognized as gone by DDF.
 *
 * @param dev Representation of a generic DDF device.
 * @return Error code.
 */
static errno_t usbfallback_device_gone(usb_device_t *dev)
{
	assert(dev);
	usb_log_info("Device '%s' gone.", usb_device_get_name(dev));
	return EOK;
}

static errno_t usbfallback_device_remove(usb_device_t *dev)
{
	assert(dev);
	usb_log_info("Device '%s' removed.", usb_device_get_name(dev));
	return EOK;
}

/** USB fallback driver ops. */
static const usb_driver_ops_t usbfallback_driver_ops = {
	.device_add = usbfallback_device_add,
	.device_remove = usbfallback_device_remove,
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
