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

/** @addtogroup libusbvirt usb
 * @{
 */
/** @file
 * @brief Preprocessing of standard device requests.
 */
#include <errno.h>
#include <stdlib.h>
#include <mem.h>
#include <usb/devreq.h>

#include "private.h"

/*
 * All sub handlers must return EFORWARD to inform the caller that
 * they were not able to process the request (yes, it is abuse of
 * this error code but such error code shall not collide with anything
 * else in this context).
 */
 
/** GET_DESCRIPTOR handler. */
static int handle_get_descriptor(usbvirt_device_t *device,
    usb_device_request_setup_packet_t *setup_packet, uint8_t *extra_data)
{
	uint8_t type = setup_packet->value_high;
	uint8_t index = setup_packet->value_low;

	/* 
	 * Standard device descriptor.
	 */
	if ((type == USB_DESCTYPE_DEVICE) && (index == 0)) {
		if (device->descriptors && device->descriptors->device) {
			return device->control_transfer_reply(device, 0,
			    device->descriptors->device,
			    device->descriptors->device->length);
		} else {
			return EFORWARD;
		}
	}
	
	/*
	 * Configuration descriptor together with interface, endpoint and
	 * class-specific descriptors.
	 */
	if (type == USB_DESCTYPE_CONFIGURATION) {
		if (!device->descriptors) {
			return EFORWARD;
		}
		if (index >= device->descriptors->configuration_count) {
			return EFORWARD;
		}
		/* Copy the data. */
		usbvirt_device_configuration_t *config = &device->descriptors
		    ->configuration[index];
		uint8_t *all_data = malloc(config->descriptor->total_length);
		if (all_data == NULL) {
			return ENOMEM;
		}
		
		uint8_t *ptr = all_data;
		memcpy(ptr, config->descriptor, config->descriptor->length);
		ptr += config->descriptor->length;
		size_t i;
		for (i = 0; i < config->extra_count; i++) {
			usbvirt_device_configuration_extras_t *extra
			    = &config->extra[i];
			memcpy(ptr, extra->data, extra->length);
			ptr += extra->length;
		}
		
		int rc = device->control_transfer_reply(device, 0,
		    all_data, config->descriptor->total_length);
		
		free(all_data);
		
		return rc;
	}
	
	return EFORWARD;
}

/** SET_ADDRESS handler. */
static int handle_set_address(usbvirt_device_t *device,
    usb_device_request_setup_packet_t *setup_packet, uint8_t *extra_data)
{
	uint16_t new_address = setup_packet->value;
	uint16_t zero1 = setup_packet->index;
	uint16_t zero2 = setup_packet->length;

	if ((zero1 != 0) || (zero2 != 0)) {
		return EINVAL;
	}
	
	if (new_address > 127) {
		return EINVAL;
	}
	
	device->new_address = new_address;
	
	return EOK;
}

/** SET_CONFIGURATION handler. */
static int handle_set_configuration(usbvirt_device_t *device,
    usb_device_request_setup_packet_t *setup_packet, uint8_t *extra_data)
{
	uint16_t configuration_value = setup_packet->value;
	uint16_t zero1 = setup_packet->index;
	uint16_t zero2 = setup_packet->length;

	if ((zero1 != 0) || (zero2 != 0)) {
		return EINVAL;
	}
	
	/*
	 * Configuration value is 1 byte information.
	 */
	if (configuration_value > 255) {
		return EINVAL;
	}
	
	/*
	 * Do nothing when in default state. According to specification,
	 * this is not specified.
	 */
	if (device->state == USBVIRT_STATE_DEFAULT) {
		return EOK;
	}
	
	if (configuration_value == 0) {
		device->state = USBVIRT_STATE_ADDRESS;
	} else {
		/*
		* TODO: browse provided configurations and verify that
		* user selected existing configuration.
		*/
		device->state = USBVIRT_STATE_CONFIGURED;
		if (device->descriptors) {
			device->descriptors->current_configuration
			    = configuration_value;
		}
	}
		
	return EOK;
}


#define MAKE_BM_REQUEST(direction, recipient) \
	USBVIRT_MAKE_CONTROL_REQUEST_TYPE(direction, \
	    USBVIRT_REQUEST_TYPE_STANDARD, recipient)
#define MAKE_BM_REQUEST_DEV(direction) \
	MAKE_BM_REQUEST(direction, USBVIRT_REQUEST_RECIPIENT_DEVICE)

usbvirt_control_transfer_handler_t control_pipe_zero_local_handlers[] = {
	{
		.request_type = MAKE_BM_REQUEST_DEV(USB_DIRECTION_IN),
		.request = USB_DEVREQ_GET_DESCRIPTOR,
		.callback = handle_get_descriptor
	},
	{
		.request_type = MAKE_BM_REQUEST_DEV(USB_DIRECTION_OUT),
		.request = USB_DEVREQ_SET_ADDRESS,
		.callback = handle_set_address
	},
	{
		.request_type = MAKE_BM_REQUEST_DEV(USB_DIRECTION_OUT),
		.request = USB_DEVREQ_SET_CONFIGURATION,
		.callback = handle_set_configuration
	},
	USBVIRT_CONTROL_TRANSFER_HANDLER_LAST
};

/**
 * @}
 */
