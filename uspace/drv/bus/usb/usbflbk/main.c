/*
 * Copyright (c) 2011 Vojtech Horky
 * Copyright (c) 2018 Petr Manek
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
