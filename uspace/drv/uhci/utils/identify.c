/*
 * Copyright (c) 2010 Jan Vesely
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

#include <errno.h>
#include <usb/classes/classes.h>
#include <usb/descriptor.h>
#include <usb/usbdrv.h>

#include "identify.h"

struct device_descriptor_packet
{
	usb_device_request_setup_packet_t request;
	usb_standard_device_descriptor_t descriptor;
};
#define DEVICE_DESCRIPTOR_PACKET_INITIALIZER \
	{ \
		.request = { \
			.request_type = 0, \
			.request = USB_DEVREQ_GET_DESCRIPTOR, \
			{ .value = USB_DESCTYPE_DEVICE }, \
			.index = 0, \
			.length = sizeof(usb_standard_device_descriptor_t) \
		} \
	}
/*----------------------------------------------------------------------------*/
struct configuration_descriptor_packet
{
	usb_device_request_setup_packet_t request;
	usb_standard_configuration_descriptor_t descriptor;
};
#define CONFIGURATION_DESCRIPTOR_PACKET_INITIALIZER \
	{ \
		.request = { \
			.request_type = 0, \
			.request = USB_DEVREQ_GET_DESCRIPTOR, \
			{ .value = USB_DESCTYPE_CONFIGURATION }, \
			.index = 0, \
			.length = sizeof(usb_standard_device_descriptor_t); \
		}; \
	}
/*----------------------------------------------------------------------------*/
int identify_device(device_t *hc, device_t *child, usb_address_t address)
{
	struct device_descriptor_packet packet =
	  DEVICE_DESCRIPTOR_PACKET_INITIALIZER;

  packet.descriptor.device_class = USB_CLASS_HUB;
  usb_drv_create_match_ids_from_device_descriptor(
	  &child->match_ids, &packet.descriptor );

	return 0;
}
