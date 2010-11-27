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

static usbhc_iface_t usb_interface = {
	.interrupt_out = NULL,
	.interrupt_in = NULL
};

static device_ops_t usb_device_ops = {
	.interfaces[USBHC_DEV_IFACE] = &usb_interface
};

int usb_add_hc_device(device_t *dev)
{
	usb_hc_device_t *hc_dev = malloc(sizeof (usb_hc_device_t));
	list_initialize(&hc_dev->link);
	hc_dev->transfer_ops = NULL;

	hc_dev->generic = dev;
	dev->ops = &usb_device_ops;
	hc_dev->generic->driver_data = hc_dev;

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

	//add keyboard
	/// @TODO this is not correct code

	/*
	 * Announce presence of child device.
	 */
	device_t *kbd = NULL;
	match_id_t *match_id = NULL;

	kbd = create_device();
	if (kbd == NULL) {
		printf("ERROR: enomem\n");
	}
	kbd->name = USB_KBD_DEVICE_NAME;

	match_id = create_match_id();
	if (match_id == NULL) {
		printf("ERROR: enomem\n");
	}

	char *id;
	rc = asprintf(&id, USB_KBD_DEVICE_NAME);
	if (rc <= 0) {
		printf("ERROR: enomem\n");
		return rc;
	}

	match_id->id = id;
	match_id->score = 30;

	add_match_id(&kbd->match_ids, match_id);

	rc = child_device_register(kbd, dev);
	if (rc != EOK) {
		printf("ERROR: cannot register kbd\n");
		return rc;
	}

	printf("%s: registered root hub\n", dev->name);
	return EOK;
}

/**
 * @}
 */
