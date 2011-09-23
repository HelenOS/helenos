/*
 * Copyright (c) 2010 Matus Dekanek
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
/** @file
 * @brief Hub driver private definitions
 */

#ifndef USBHUB_UTILS_H
#define USBHUB_UTILS_H

#include <adt/list.h>
#include <bool.h>
#include <ddf/driver.h>
#include <fibril_synch.h>

#include <usb/classes/hub.h>
#include <usb/usb.h>
#include <usb/debug.h>
#include <usb/dev/request.h>

#include "usbhub.h"

//************
//
// convenience define for malloc
//
//************

/**
 * Set the device request to be a get hub descriptor request.
 * @warning the size is allways set to USB_HUB_MAX_DESCRIPTOR_SIZE
 * @param request
 * @param addr
 */
static inline void usb_hub_set_descriptor_request(
    usb_device_request_setup_packet_t *request )
{
	request->index = 0;
	request->request_type = USB_HUB_REQ_TYPE_GET_DESCRIPTOR;
	request->request = USB_HUB_REQUEST_GET_DESCRIPTOR;
	request->value_high = USB_DESCTYPE_HUB;
	request->value_low = 0;
	request->length = USB_HUB_MAX_DESCRIPTOR_SIZE;
}

/**
 * Clear feature on hub port.
 *
 * @param hc Host controller telephone
 * @param address Hub address
 * @param port_index Port
 * @param feature Feature selector
 * @return Operation result
 */
static inline int usb_hub_clear_port_feature(usb_pipe_t *pipe,
    int port_index, usb_hub_class_feature_t feature)
{

	usb_device_request_setup_packet_t clear_request = {
		.request_type = USB_HUB_REQ_TYPE_CLEAR_PORT_FEATURE,
		.request = USB_DEVREQ_CLEAR_FEATURE,
		.length = 0,
		.index = port_index
	};
	clear_request.value = feature;
	return usb_pipe_control_write(pipe, &clear_request,
	    sizeof (clear_request), NULL, 0);
}

/**
 * Clear feature on hub port.
 *
 * @param hc Host controller telephone
 * @param address Hub address
 * @param port_index Port
 * @param feature Feature selector
 * @return Operation result
 */
static inline int usb_hub_set_port_feature(usb_pipe_t *pipe,
    int port_index, usb_hub_class_feature_t feature)
{

	usb_device_request_setup_packet_t clear_request = {
		.request_type = USB_HUB_REQ_TYPE_SET_PORT_FEATURE,
		.request = USB_DEVREQ_SET_FEATURE,
		.length = 0,
		.index = port_index
	};
	clear_request.value = feature;
	return usb_pipe_control_write(pipe, &clear_request,
	    sizeof (clear_request), NULL, 0);
}

/**
 * Clear feature on hub port.
 *
 * @param pipe pipe to hub control endpoint
 * @param feature Feature selector
 * @return Operation result
 */
static inline int usb_hub_clear_feature(usb_pipe_t *pipe,
    usb_hub_class_feature_t feature) {

	usb_device_request_setup_packet_t clear_request = {
		.request_type = USB_HUB_REQ_TYPE_CLEAR_HUB_FEATURE,
		.request = USB_DEVREQ_CLEAR_FEATURE,
		.length = 0,
		.index = 0
	};
	clear_request.value = feature;
	return usb_pipe_control_write(pipe, &clear_request,
	    sizeof (clear_request), NULL, 0);
}

/**
 * Clear feature on hub port.
 *
 * @param pipe pipe to hub control endpoint
 * @param feature Feature selector
 * @return Operation result
 */
static inline int usb_hub_set_feature(usb_pipe_t *pipe,
    usb_hub_class_feature_t feature)
{

	usb_device_request_setup_packet_t clear_request = {
		.request_type = USB_HUB_REQ_TYPE_CLEAR_HUB_FEATURE,
		.request = USB_DEVREQ_SET_FEATURE,
		.length = 0,
		.index = 0
	};
	clear_request.value = feature;
	return usb_pipe_control_write(pipe, &clear_request,
	    sizeof (clear_request), NULL, 0);
}

void * usb_create_serialized_hub_descriptor(usb_hub_descriptor_t *descriptor);

void usb_serialize_hub_descriptor(usb_hub_descriptor_t *descriptor,
    void *serialized_descriptor);

int usb_deserialize_hub_desriptor(
    void *serialized_descriptor, size_t size, usb_hub_descriptor_t *descriptor);

#endif
/**
 * @}
 */
