/*
 * Copyright (c) 2013 Jan Vesely
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

/** @addtogroup libusbvirt
 * @{
 */
/** @file
 * Virtual USB device main routines.
 */

#include <assert.h>
#include <macros.h>
#include <str.h>
#include <usb/classes/hub.h>

#include "virthub_base.h"

extern const usb_standard_device_descriptor_t virthub_device_descriptor;
extern const usb_standard_configuration_descriptor_t virthub_configuration_descriptor_without_hub_size;
extern const usb_standard_endpoint_descriptor_t virthub_endpoint_descriptor;
extern const usbvirt_device_configuration_extras_t virthub_interface_descriptor_ex;

void *virthub_get_data(usbvirt_device_t *dev)
{
	assert(dev);
	virthub_base_t *base = dev->device_data;
	assert(base);
	return base->data;
}

errno_t virthub_base_init(virthub_base_t *instance, const char *name,
    usbvirt_device_ops_t *ops, void *data,
    const usb_standard_device_descriptor_t *device_desc,
    const usb_hub_descriptor_header_t *hub_desc, usb_endpoint_t ep)
{
	assert(instance);
	assert(hub_desc);
	assert(name);

	if (!usb_endpoint_is_valid(ep) || (ep == USB_ENDPOINT_DEFAULT_CONTROL))
		return EINVAL;

	instance->config_descriptor =
	    virthub_configuration_descriptor_without_hub_size;
	instance->config_descriptor.total_length += hub_desc->length;

	instance->endpoint_descriptor = virthub_endpoint_descriptor;
	instance->endpoint_descriptor.endpoint_address = 128 | ep;
	instance->endpoint_descriptor.max_packet_size =
	    STATUS_BYTES(hub_desc->port_count);

	instance->descriptors.device =
	    device_desc ? device_desc : &virthub_device_descriptor;
	instance->descriptors.configuration = &instance->configuration;
	instance->descriptors.configuration_count = 1;

	instance->configuration.descriptor = &instance->config_descriptor;
	instance->configuration.extra = instance->extra;
	instance->configuration.extra_count = ARRAY_SIZE(instance->extra);

	instance->extra[0] = virthub_interface_descriptor_ex;
	instance->extra[1].data = (void *)hub_desc;
	instance->extra[1].length = hub_desc->length;
	instance->extra[2].data = (void*)&instance->endpoint_descriptor;
	instance->extra[2].length = sizeof(instance->endpoint_descriptor);

	instance->device.ops = ops;
	instance->device.descriptors = &instance->descriptors;
	instance->device.device_data = instance;
	instance->device.address = 0;
	instance->data = data;
	instance->device.name = str_dup(name);

	if (!instance->device.name)
		return ENOMEM;

	return EOK;
}

usb_address_t virthub_base_get_address(virthub_base_t *instance)
{
	assert(instance);
	return instance->device.address;
}

errno_t virthub_base_request(virthub_base_t *instance, usb_target_t target,
    usb_direction_t dir, const usb_device_request_setup_packet_t *setup,
    void *buffer, size_t buffer_size, size_t *real_size)
{
	assert(instance);
	assert(real_size);
	assert(setup);

	if (target.address != virthub_base_get_address(instance))
		return ENOENT;

	switch (dir) {
	case USB_DIRECTION_IN:
		if (target.endpoint == 0) {
			return usbvirt_control_read(&instance->device,
			    setup, sizeof(*setup), buffer, buffer_size,
			    real_size);
		} else {
			return usbvirt_data_in(&instance->device,
			    USB_TRANSFER_INTERRUPT, target.endpoint,
			    buffer, buffer_size, real_size);
		}
	case USB_DIRECTION_OUT:
		if (target.endpoint == 0) {
			return usbvirt_control_write(&instance->device,
			    setup, sizeof(*setup), buffer, buffer_size);
		}
		/* fall through */
	default:
		return ENOTSUP;

	}
}

errno_t virthub_base_get_hub_descriptor(usbvirt_device_t *dev,
    const usb_device_request_setup_packet_t *request, uint8_t *data,
    size_t *act_size)
{
	assert(dev);
	virthub_base_t *instance = dev->device_data;
	assert(instance);
	if (request->value_high == USB_DESCTYPE_HUB) {
		usbvirt_control_reply_helper(request, data, act_size,
		    instance->extra[1].data, instance->extra[1].length);
		return EOK;
	}
	/* Let the framework handle all the rest. */
	return EFORWARD;
}

errno_t virthub_base_get_null_status(usbvirt_device_t *dev,
    const usb_device_request_setup_packet_t *request, uint8_t *data,
    size_t *act_size)
{
	uint32_t zero = 0;
	if (request->length != sizeof(zero))
		return ESTALL;
	usbvirt_control_reply_helper(request, data, act_size,
	    &zero, sizeof(zero));
	return EOK;
}

/**
 * @}
 */
