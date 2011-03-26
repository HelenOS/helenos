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

#include <usb/devdrv.h>

#include "kbddev.h"
#include "kbdrepeat.h"

/*----------------------------------------------------------------------------*/

#define NAME "usbkbd"

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
 *
 * @sa usb_kbd_fibril(), usb_kbd_repeat_fibril()
 */
static int usbhid_try_add_device(usb_device_t *dev)
{
	/* Create the function exposed under /dev/devices. */
	ddf_fun_t *kbd_fun = ddf_fun_create(dev->ddf_dev, fun_exposed, 
	    "keyboard");
	if (kbd_fun == NULL) {
		usb_log_error("Could not create DDF function node.\n");
		return ENOMEM;
	}
	
	/* 
	 * Initialize device (get and process descriptors, get address, etc.)
	 */
	usb_log_debug("Initializing USB/HID KBD device...\n");
	
	usb_kbd_t *kbd_dev = usb_kbd_new();
	if (kbd_dev == NULL) {
		usb_log_error("Error while creating USB/HID KBD device "
		    "structure.\n");
		ddf_fun_destroy(kbd_fun);
		return ENOMEM;  // TODO: some other code??
	}
	
	int rc = usb_kbd_init(kbd_dev, dev);
	
	if (rc != EOK) {
		usb_log_error("Failed to initialize USB/HID KBD device.\n");
		ddf_fun_destroy(kbd_fun);
		usb_kbd_free(&kbd_dev);
		return rc;
	}	
	
	usb_log_debug("USB/HID KBD device structure initialized.\n");
	
	/*
	 * Store the initialized keyboard device and keyboard ops
	 * to the DDF function.
	 */
	kbd_fun->driver_data = kbd_dev;
	kbd_fun->ops = &keyboard_ops;

	rc = ddf_fun_bind(kbd_fun);
	if (rc != EOK) {
		usb_log_error("Could not bind DDF function: %s.\n",
		    str_error(rc));
		// TODO: Can / should I destroy the DDF function?
		ddf_fun_destroy(kbd_fun);
		usb_kbd_free(&kbd_dev);
		return rc;
	}
	
	rc = ddf_fun_add_to_class(kbd_fun, "keyboard");
	if (rc != EOK) {
		usb_log_error(
		    "Could not add DDF function to class 'keyboard': %s.\n",
		    str_error(rc));
		// TODO: Can / should I destroy the DDF function?
		ddf_fun_destroy(kbd_fun);
		usb_kbd_free(&kbd_dev);
		return rc;
	}
	
	/*
	 * Create new fibril for handling this keyboard
	 */
	//fid_t fid = fibril_create(usb_kbd_fibril, kbd_dev);
	
	/* Start automated polling function.
	 * This will create a separate fibril that will query the device
	 * for the data continuously 
	 */
       rc = usb_device_auto_poll(dev,
	   /* Index of the polling pipe. */
	   USB_KBD_POLL_EP_NO,
	   /* Callback when data arrives. */
	   usb_kbd_polling_callback,
	   /* How much data to request. */
	   dev->pipes[USB_KBD_POLL_EP_NO].pipe->max_packet_size,
	   /* Callback when the polling ends. */
	   usb_kbd_polling_ended_callback,
	   /* Custom argument. */
	   kbd_dev);
	
	
	if (rc != EOK) {
		usb_log_error("Failed to start polling fibril for `%s'.\n",
		    dev->ddf_dev->name);
		return rc;
	}
	//fibril_add_ready(fid);
	
	/*
	 * Create new fibril for auto-repeat
	 */
	fid_t fid = fibril_create(usb_kbd_repeat_fibril, kbd_dev);
	if (fid == 0) {
		usb_log_error("Failed to start fibril for KBD auto-repeat");
		return ENOMEM;
	}
	fibril_add_ready(fid);

	(void)keyboard_ops;

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
static int usbhid_add_device(usb_device_t *dev)
{
	usb_log_debug("usbhid_add_device()\n");
	
	if (dev->interface_no < 0) {
		usb_log_warning("Device is not a supported keyboard.\n");
		usb_log_error("Failed to add HID device: endpoint not found."
		    "\n");
		return ENOTSUP;
	}
	
	int rc = usbhid_try_add_device(dev);
	
	if (rc != EOK) {
		usb_log_warning("Device is not a supported keyboard.\n");
		usb_log_error("Failed to add HID device: %s.\n",
		    str_error(rc));
		return rc;
	}
	
	usb_log_info("Keyboard `%s' ready to use.\n", dev->ddf_dev->name);

	return EOK;
}

/*----------------------------------------------------------------------------*/

/* Currently, the framework supports only device adding. Once the framework
 * supports unplug, more callbacks will be added. */
static usb_driver_ops_t usbhid_driver_ops = {
        .add_device = usbhid_add_device,
};


/* The driver itself. */
static usb_driver_t usbhid_driver = {
        .name = NAME,
        .ops = &usbhid_driver_ops,
        .endpoints = usb_kbd_endpoints
};

/*----------------------------------------------------------------------------*/

//static driver_ops_t kbd_driver_ops = {
//	.add_device = usbhid_add_device,
//};

///*----------------------------------------------------------------------------*/

//static driver_t kbd_driver = {
//	.name = NAME,
//	.driver_ops = &kbd_driver_ops
//};

/*----------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS USB HID driver.\n");

	usb_log_enable(USB_LOG_LEVEL_DEBUG, NAME);

	return usb_driver_main(&usbhid_driver);
}

/**
 * @}
 */
