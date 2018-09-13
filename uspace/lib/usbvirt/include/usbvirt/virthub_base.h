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
 * Virtual USB device.
 */

#ifndef LIBUSBVIRT_VIRTHUB_BASE_H_
#define LIBUSBVIRT_VIRTHUB_BASE_H_

#include <usbvirt/device.h>
#include <usb/classes/hub.h>

enum {
	VIRTHUB_EXTR_DESC = 3,
};

typedef struct {
	usb_standard_configuration_descriptor_t config_descriptor;
	usb_standard_endpoint_descriptor_t endpoint_descriptor;
	usbvirt_device_configuration_extras_t extra[VIRTHUB_EXTR_DESC];
	usbvirt_device_configuration_t configuration;
	usbvirt_descriptors_t descriptors;
	usbvirt_device_t device;
	void *data;
} virthub_base_t;

void *virthub_get_data(usbvirt_device_t *dev);

errno_t virthub_base_init(virthub_base_t *instance,
    const char *name, usbvirt_device_ops_t *ops, void *data,
    const usb_standard_device_descriptor_t *device_desc,
    const usb_hub_descriptor_header_t *hub_desc, usb_endpoint_t ep);

usb_address_t virthub_base_get_address(virthub_base_t *instance);

errno_t virthub_base_request(virthub_base_t *instance, usb_target_t target,
    usb_direction_t dir, const usb_device_request_setup_packet_t *setup,
    void *buffer, size_t buffer_size, size_t *real_size);

errno_t virthub_base_get_hub_descriptor(usbvirt_device_t *dev,
    const usb_device_request_setup_packet_t *request, uint8_t *data,
    size_t *act_size);
errno_t virthub_base_get_null_status(usbvirt_device_t *dev,
    const usb_device_request_setup_packet_t *request, uint8_t *data,
    size_t *act_size);

#endif

/**
 * @}
 */
