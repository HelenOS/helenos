/*
 * Copyright (c) 2010 Vojtech Horky
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

/** @addtogroup libusb usb
 * @{
 */
/** @file
 * @brief HC driver.
 */
#include <usb/hcdhubd.h>
#include <usb/devreq.h>
#include <usbhc_iface.h>
#include <usb/descriptor.h>
#include <driver.h>
#include <bool.h>
#include <errno.h>
#include <usb/classes/hub.h>

#include "hcdhubd_private.h"

/** List of handled host controllers. */
LIST_INITIALIZE(hc_list);

/** Our HC driver. */
usb_hc_driver_t *hc_driver = NULL;

static device_ops_t usb_device_ops = {
	.interfaces[USBHC_DEV_IFACE] = &usbhc_interface
};

static usb_hc_device_t *usb_hc_device_create(device_t *dev) {
	usb_hc_device_t *hc_dev = malloc(sizeof (usb_hc_device_t));

	list_initialize(&hc_dev->link);
	list_initialize(&hc_dev->hubs);
	list_initialize(&hc_dev->attached_devices);
	hc_dev->transfer_ops = NULL;

	hc_dev->generic = dev;
	dev->ops = &usb_device_ops;
	hc_dev->generic->driver_data = hc_dev;

	return hc_dev;
}

int usb_add_hc_device(device_t *dev)
{
	usb_hc_device_t *hc_dev = usb_hc_device_create(dev);

	int rc = hc_driver->add_hc(hc_dev);
	if (rc != EOK) {
		free(hc_dev);
		return rc;
	}

	/*
	 * FIXME: The following line causes devman to hang.
	 * Will investigate later why.
	 */
	// add_device_to_class(dev, "usbhc");

	list_append(&hc_dev->link, &hc_list);

	/*
	 * FIXME: the following is a workaround to force loading of USB
	 * keyboard driver.
	 * Will be removed as soon as the hub driver is completed and
	 * can detect connected devices.
	 */
	printf("%s: trying to add USB HID child device...\n", hc_driver->name);
	rc = usb_hc_add_child_device(dev, USB_KBD_DEVICE_NAME, "usb&hid", false);
	if (rc != EOK) {
		printf("%s: adding USB HID child failed...\n", hc_driver->name);
	}

	return EOK;
}

/**
 * @}
 */
