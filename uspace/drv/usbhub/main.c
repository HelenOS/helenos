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

/** @addtogroup drvusbhub
 * @{
 */

#include <ddf/driver.h>
#include <errno.h>
#include <async.h>
#include <stdio.h>

#include <usb/devdrv.h>
#include <usb/classes/classes.h>

#include "usbhub.h"
#include "usbhub_private.h"


usb_endpoint_description_t hub_status_change_endpoint_description = {
	.transfer_type = USB_TRANSFER_INTERRUPT,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_HUB,
	.interface_subclass = 0,
	.interface_protocol = 0,
	.flags = 0
};


static usb_driver_ops_t usb_hub_driver_ops = {
	.add_device = usb_hub_add_device
};

static usb_driver_t usb_hub_driver = {
	.name = "usbhub",
	.ops = &usb_hub_driver_ops
};


int main(int argc, char *argv[])
{
	usb_log_enable(USB_LOG_LEVEL_DEBUG, NAME);
	usb_log_info("starting hub driver\n");

	
	usb_hub_driver.endpoints = (usb_endpoint_description_t**)
			malloc(2 * sizeof(usb_endpoint_description_t*));
	usb_hub_driver.endpoints[0] = &hub_status_change_endpoint_description;
	usb_hub_driver.endpoints[1] = NULL;

	return usb_driver_main(&usb_hub_driver);
}

/**
 * @}
 */

