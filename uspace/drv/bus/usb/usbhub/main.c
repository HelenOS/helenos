/*
 * Copyright (c) 2010 Vojtech Horky
 * Copyright (c) 2011 Jan Vesely
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

/** @addtogroup drvusbhub
 * @{
 */

#include <ddf/driver.h>
#include <errno.h>
#include <async.h>
#include <stdio.h>

#include <usb/dev/driver.h>
#include <usb/classes/classes.h>
#include <usb/debug.h>

#include "usbhub.h"

/** Hub status-change endpoint description.
 *
 * For more information see section 11.15.1 of USB 1.1 specification.
 */
static const usb_endpoint_description_t hub_status_change_endpoint_description =
{
	.transfer_type = USB_TRANSFER_INTERRUPT,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_HUB,
	.interface_subclass = 0,
	.interface_protocol = 0,
	.flags = 0
};

/** USB hub driver operations. */
static const usb_driver_ops_t usb_hub_driver_ops = {
	.device_add = usb_hub_device_add,
//	.device_rem = usb_hub_device_remove,
	.device_gone = usb_hub_device_gone,
};

/** Hub endpoints, excluding control endpoint. */
static const usb_endpoint_description_t *usb_hub_endpoints[] = {
	&hub_status_change_endpoint_description,
	NULL,
};
/** Static usb hub driver information. */
static const usb_driver_t usb_hub_driver = {
	.name = NAME,
	.ops = &usb_hub_driver_ops,
	.endpoints = usb_hub_endpoints
};

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS USB hub driver.\n");
	log_init(NAME);

	return usb_driver_main(&usb_hub_driver);
}

/**
 * @}
 */
