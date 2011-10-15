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

#include <usb/dev/driver.h>
#include <usb/dev/poll.h>

#include "usbhid.h"

/*----------------------------------------------------------------------------*/

#define NAME "usbhid"

/**
 * Function for adding a new device of type USB/HID/keyboard.
 *
 * This functions initializes required structures from the device's descriptors
 * and starts new fibril for polling the keyboard for events and another one for
 * handling auto-repeat of keys.
 *
 * During initialization, the keyboard is switched into boot protocol, the idle
 * rate is set to 0 (infinity), resulting in the keyboard only reporting event
 * when a key is pressed or released. Finally, the LED lights are turned on 
 * according to the default setup of lock keys.
 *
 * @note By default, the keyboards is initialized with Num Lock turned on and 
 *       other locks turned off.
 * @note Currently supports only boot-protocol keyboards.
 *
 * @param dev Device to add.
 *
 * @retval EOK if successful.
 * @retval ENOMEM if there
 * @return Other error code inherited from one of functions usb_kbd_init(),
 *         ddf_fun_bind() and ddf_fun_add_to_class().
 */
static int usb_hid_try_add_device(usb_device_t *dev)
{
	assert(dev != NULL);

	/* 
	 * Initialize device (get and process descriptors, get address, etc.)
	 */
	usb_log_debug("Initializing USB/HID device...\n");

	usb_hid_dev_t *hid_dev = usb_hid_new();
	if (hid_dev == NULL) {
		usb_log_error("Error while creating USB/HID device "
		    "structure.\n");
		return ENOMEM;
	}

	int rc = usb_hid_init(hid_dev, dev);

	if (rc != EOK) {
		usb_log_error("Failed to initialize USB/HID device.\n");
		usb_hid_destroy(hid_dev);
		return rc;
	}

	usb_log_debug("USB/HID device structure initialized.\n");

	/*
	 * 1) subdriver vytvori vlastnu ddf_fun, vlastne ddf_dev_ops, ktore da
	 *    do nej.
	 * 2) do tych ops do .interfaces[DEV_IFACE_USBHID (asi)] priradi 
	 *    vyplnenu strukturu usbhid_iface_t.
	 * 3) klientska aplikacia - musi si rucne vytvorit telefon
	 *    (devman_device_connect() - cesta k zariadeniu (/hw/pci0/...) az 
	 *    k tej fcii.
	 *    pouzit usb/classes/hid/iface.h - prvy int je telefon
	 */

	/* Start automated polling function.
	 * This will create a separate fibril that will query the device
	 * for the data continuously 
	 */
       rc = usb_device_auto_poll(dev,
	   /* Index of the polling pipe. */
	   hid_dev->poll_pipe_index,
	   /* Callback when data arrives. */
	   usb_hid_polling_callback,
	   /* How much data to request. */
	   dev->pipes[hid_dev->poll_pipe_index].pipe->max_packet_size,
	   /* Callback when the polling ends. */
	   usb_hid_polling_ended_callback,
	   /* Custom argument. */
	   hid_dev);

	if (rc != EOK) {
		usb_log_error("Failed to start polling fibril for `%s'.\n",
		    dev->ddf_dev->name);
		usb_hid_destroy(hid_dev);
		return rc;
	}
	hid_dev->running = true;
	dev->driver_data = hid_dev;

	/*
	 * Hurrah, device is initialized.
	 */
	return EOK;
}

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
static int usb_hid_device_add(usb_device_t *dev)
{
	usb_log_debug("usb_hid_device_add()\n");

	if (dev == NULL) {
		usb_log_warning("Wrong parameter given for add_device().\n");
		return EINVAL;
	}

	if (dev->interface_no < 0) {
		usb_log_warning("Device is not a supported HID device.\n");
		usb_log_error("Failed to add HID device: endpoints not found."
		    "\n");
		return ENOTSUP;
	}

	int rc = usb_hid_try_add_device(dev);

	if (rc != EOK) {
		usb_log_warning("Device is not a supported HID device.\n");
		usb_log_error("Failed to add HID device: %s.\n",
		    str_error(rc));
		return rc;
	}

	usb_log_info("HID device `%s' ready to use.\n", dev->ddf_dev->name);

	return EOK;
}

/*----------------------------------------------------------------------------*/

/**
 * Callback for removing a device from the driver.
 *
 * @param dev Structure representing the device.
 *
 * @retval EOK if successful. 
 * @retval EREFUSED if the device is not supported.
 */
static int usb_hid_device_gone(usb_device_t *dev)
{
	usb_hid_dev_t *hid_dev = dev->driver_data;
	unsigned tries = 10;
	while (hid_dev->running) {
		async_usleep(100000);
		if (!tries--) {
			usb_log_error("Can't remove hub, still running.\n");
			return EINPROGRESS;
		}
	}

	assert(!hid_dev->running);
	usb_hid_destroy(hid_dev);
	usb_log_debug2("%s destruction complete.\n", dev->ddf_dev->name);
	return EOK;
}

/** USB generic driver callbacks */
static usb_driver_ops_t usb_hid_driver_ops = {
	.device_add = usb_hid_device_add,
	.device_gone = usb_hid_device_gone,
};


/** The driver itself. */
static usb_driver_t usb_hid_driver = {
        .name = NAME,
        .ops = &usb_hid_driver_ops,
        .endpoints = usb_hid_endpoints
};

/*----------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS USB HID driver.\n");

	usb_log_enable(USB_LOG_LEVEL_DEFAULT, NAME);

	return usb_driver_main(&usb_hid_driver);
}

/**
 * @}
 */
