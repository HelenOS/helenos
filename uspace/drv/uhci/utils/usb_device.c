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

#include "debug.h"
#include "usb_device.h"

/*----------------------------------------------------------------------------*/
#define CHECK_RET_RETURN(ret, child, message, args...)\
  if (ret < 0) { \
    uhci_print_error("Failed(%d) to "message, ret, ##args); \
    return ret; \
  } else (void)0

int usb_device_init(device_t *device, device_t *hc, usb_address_t address,
  int hub_port)
{
	assert(device);
	assert(hc);

	int ret = 0;
  char *name;

  /* create name */
  ret = asprintf(&name, "usbdevice on hc%p/root_hub[%d]/%#x",
	  hc, hub_port, address);
  CHECK_RET_RETURN(ret, child, "create device name.\n");

  device->name = name;

	/* use descriptors to identify the device */
  ret = usb_device_identify(device, hc, address);
  CHECK_RET_RETURN(ret, child, "identify device.\n");

	return EOK;
}
/*----------------------------------------------------------------------------*/
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
int usb_device_identify(device_t *device, device_t *hc, usb_address_t address)
{
	assert(device);
	assert(hc);

	struct device_descriptor_packet packet =
	  DEVICE_DESCRIPTOR_PACKET_INITIALIZER;

  packet.descriptor.device_class = USB_CLASS_HUB;
  usb_drv_create_match_ids_from_device_descriptor(
	  &device->match_ids, &packet.descriptor );

	return 0;
}
/*----------------------------------------------------------------------------*/
