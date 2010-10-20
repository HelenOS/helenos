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

/** @addtogroup usb
 * @{
 */
/** @file
 * @brief Virtual USB hub operations.
 */
#include <usb/classes.h>
#include <usb/hub.h>
#include <usbvirt/hub.h>
#include <usbvirt/device.h>
#include <errno.h>

#include "vhcd.h"
#include "hub.h"
#include "hubintern.h"

static int on_get_descriptor(struct usbvirt_device *dev,
    usb_device_request_setup_packet_t *request, uint8_t *data);
static int on_class_request(struct usbvirt_device *dev,
    usb_device_request_setup_packet_t *request, uint8_t *data);

static usbvirt_standard_device_request_ops_t standard_request_ops = {
	.on_get_status = NULL,
	.on_clear_feature = NULL,
	.on_set_feature = NULL,
	.on_set_address = NULL,
	.on_get_descriptor = on_get_descriptor,
	.on_set_descriptor = NULL,
	.on_get_configuration = NULL,
	.on_set_configuration = NULL,
	.on_get_interface = NULL,
	.on_set_interface = NULL,
	.on_synch_frame = NULL
};


usbvirt_device_ops_t hub_ops = {
	.standard_request_ops = &standard_request_ops,
	.on_class_device_request = on_class_request,
	.on_data = NULL
};

static int on_get_descriptor(struct usbvirt_device *dev,
    usb_device_request_setup_packet_t *request, uint8_t *data)
{
	if (request->value_high == USB_DESCTYPE_HUB) {
		int rc = dev->send_data(dev, 0,
		    &hub_descriptor, hub_descriptor.length);
		
		return rc;
	}
	/* Let the framework handle all the rest. */
	return EFORWARD;
}

static int clear_hub_feature(uint16_t feature)
{
	return ENOTSUP;
}

static int clear_port_feature(uint16_t feature, uint16_t portindex)
{
	return ENOTSUP;
}

static int get_bus_state(uint16_t portindex)
{
	return ENOTSUP;
}

static int get_hub_descriptor(uint8_t descriptor_type,
    uint8_t descriptor_index, uint16_t length)
{
	return ENOTSUP;
}

static int get_hub_status(void)
{
	return ENOTSUP;
}

static int get_port_status(uint16_t portindex)
{
	return ENOTSUP;
}


static int set_hub_feature(uint16_t feature)
{
	return ENOTSUP;
}

static int set_port_feature(uint16_t feature, uint16_t portindex)
{
	return ENOTSUP;
}


static int on_class_request(struct usbvirt_device *dev,
    usb_device_request_setup_packet_t *request, uint8_t *data)
{	
	printf("%s: hub class request (%d)\n", NAME, (int) request->request);
	
	uint8_t recipient = request->request_type & 31;
	uint8_t direction = request->request_type >> 7;
	
#define _VERIFY(cond) \
	do { \
		if (!(cond)) { \
			printf("%s: WARN: invalid class request (%s not met).\n", \
			    NAME, #cond); \
			return EINVAL; \
		} \
	} while (0)
	
	switch (request->request) {
		case USB_HUB_REQUEST_CLEAR_FEATURE:
			_VERIFY(direction == 0);
			_VERIFY(request->length == 0);
			if (recipient == 0) {
				_VERIFY(request->index == 0);
				return clear_hub_feature(request->value);
			} else {
				_VERIFY(recipient == 3);
				return clear_port_feature(request->value,
				    request->index);
			}
			
		case USB_HUB_REQUEST_GET_STATE:
			return get_bus_state(request->index);
			
		case USB_HUB_REQUEST_GET_DESCRIPTOR:
			return get_hub_descriptor(request->value_low,
			    request->value_high, request->length);
			
		case USB_HUB_REQUEST_GET_STATUS:
			if (recipient == 0) {
				return get_hub_status();
			} else {
				return get_port_status(request->index);
			}
			
		case USB_HUB_REQUEST_SET_FEATURE:
			if (recipient == 0) {
				return set_hub_feature(request->value);
			} else {
				return set_port_feature(request->value, request->index);
			}
			
		default:
			break;
	}
	
#undef _VERIFY	


	return EOK;
}

/**
 * @}
 */
