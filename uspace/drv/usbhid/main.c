/*
 * Copyright (c) 2010 Vojtech Horky
 * Copyright (c) 2011 Lubos Slovak
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

#include "kbddev.h"

/*----------------------------------------------------------------------------*/

#define NAME "usbhid"

/*----------------------------------------------------------------------------*/
/**
 * Callback for passing a new device to the driver.
 *
 * @note Currently, only boot-protocol keyboards are supported by this driver.
 *
 * @param dev Structure representing the new device.
 *
 * @retval EOK if successful. 
 * @retval EREFUSED if the device is not supported.
 */
static int usbhid_add_device(ddf_dev_t *dev)
{
	usb_log_debug("usbhid_add_device()\n");
	
	int rc = usbhid_kbd_try_add_device(dev);
	
	if (rc != EOK) {
		usb_log_warning("Device is not a supported keyboard.\n");
		usb_log_error("Failed to add HID device: %s.\n",
		    str_error(rc));
		return rc;
	}
	
	usb_log_info("Keyboard `%s' ready to use.\n", dev->name);

	return EOK;
}

/*----------------------------------------------------------------------------*/

static driver_ops_t kbd_driver_ops = {
	.add_device = usbhid_add_device,
};

/*----------------------------------------------------------------------------*/

static driver_t kbd_driver = {
	.name = NAME,
	.driver_ops = &kbd_driver_ops
};

/*----------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS USB HID driver.\n");

	usb_log_enable(USB_LOG_LEVEL_DEFAULT, NAME);

	return ddf_driver_main(&kbd_driver);
}

/**
 * @}
 */
