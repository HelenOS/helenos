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
 * @brief Virtual USB device.
 */
#ifndef LIBUSBVIRT_DEVICE_H_
#define LIBUSBVIRT_DEVICE_H_

#include <usb/hcd.h>
#include <usb/descriptor.h>
#include <usb/devreq.h>

struct usbvirt_device;
struct usbvirt_control_transfer;

typedef int (*usbvirt_on_device_request_t)(struct usbvirt_device *dev,
	usb_device_request_setup_packet_t *request,
	uint8_t *data);

/** Callbacks for standard device requests.
 * When these functions are NULL or return EFORWARD, this
 * framework will try to satisfy the request by itself.
 */
typedef struct {
	usbvirt_on_device_request_t on_get_status;
	usbvirt_on_device_request_t on_clear_feature;
	usbvirt_on_device_request_t on_set_feature;
	usbvirt_on_device_request_t on_set_address;
	usbvirt_on_device_request_t on_get_descriptor;
	usbvirt_on_device_request_t on_set_descriptor;
	usbvirt_on_device_request_t on_get_configuration;
	usbvirt_on_device_request_t on_set_configuration;
	usbvirt_on_device_request_t on_get_interface;
	usbvirt_on_device_request_t on_set_interface;
	usbvirt_on_device_request_t on_synch_frame;
} usbvirt_standard_device_request_ops_t;

/** Device operations. */
typedef struct {
	/** Callbacks for standard deivce requests. */
	usbvirt_standard_device_request_ops_t *standard_request_ops;
	/** Callback for class-specific USB request. */
	usbvirt_on_device_request_t on_class_device_request;
	
	int (*on_control_transfer)(struct usbvirt_device *dev,
	    usb_endpoint_t endpoint, struct usbvirt_control_transfer *transfer);
	
	/** Callback for all other incoming data. */
	int (*on_data)(struct usbvirt_device *dev,
	    usb_endpoint_t endpoint, void *buffer, size_t size);
	
	/** Callback for host request for data. */
	int (*on_data_request)(struct usbvirt_device *dev,
	    usb_endpoint_t endpoint, void *buffer, size_t size, size_t *actual_size);
	
	/** Decides direction of control transfer. */
	usb_direction_t (*decide_control_transfer_direction)(
	    usb_endpoint_t endpoint, void *buffer, size_t size);
} usbvirt_device_ops_t;

/** Extra configuration data for GET_CONFIGURATION request. */
typedef struct {
	/** Actual data. */
	uint8_t *data;
	/** Data length. */
	size_t length;
} usbvirt_device_configuration_extras_t;

/** Single device configuration. */
typedef struct {
	/** Standard configuration descriptor. */
	usb_standard_configuration_descriptor_t *descriptor;
	/** Array of extra data. */
	usbvirt_device_configuration_extras_t *extra;
	/** Length of @c extra array. */
	size_t extra_count;
} usbvirt_device_configuration_t;

/** Standard USB descriptors. */
typedef struct {
	/** Standard device descriptor.
	 * There is always only one such descriptor for the device.
	 */
	usb_standard_device_descriptor_t *device;
	
	/** Configurations. */
	usbvirt_device_configuration_t *configuration;
	/** Number of configurations. */
	size_t configuration_count;
	/** Index of currently selected configuration. */
	uint8_t current_configuration;
} usbvirt_descriptors_t;

/** Possible states of virtual USB device.
 * Notice that these are not 1:1 mappings to those in USB specification.
 */
typedef enum {
	USBVIRT_STATE_DEFAULT,
	USBVIRT_STATE_ADDRESS,
	USBVIRT_STATE_CONFIGURED
} usbvirt_device_state_t;

/** Information about on-going control transfer.
 */
typedef struct usbvirt_control_transfer {
	usb_direction_t direction;
	void *request;
	size_t request_size;
	void *data;
	size_t data_size;
} usbvirt_control_transfer_t;

/** Virtual USB device. */
typedef struct usbvirt_device {
	/** Callback device operations. */
	usbvirt_device_ops_t *ops;
	
	
	/** Reply onto control transfer.
	 */
	int (*control_transfer_reply)(struct usbvirt_device *dev,
	    usb_endpoint_t endpoint, void *buffer, size_t size);
	
	/* Device attributes. */
	
	/** Standard descriptors. */
	usbvirt_descriptors_t *descriptors;
	
	/** Current device state. */
	usbvirt_device_state_t state;
	/** Device address. */
	usb_address_t address;
	
	/* Private attributes. */
	
	/** Phone to HC.
	 * @warning Do not change, this is private variable.
	 */
	int vhcd_phone_;
	
	/** Device id.
	 * This item will be removed when device enumeration and
	 * recognition is implemented.
	 */
	int device_id_;
	
	int (*transaction_out)(struct usbvirt_device *dev,
	    usb_endpoint_t endpoint, void *buffer, size_t size);
	int (*transaction_setup)(struct usbvirt_device *dev,
	    usb_endpoint_t endpoint, void *buffer, size_t size);
	int (*transaction_in)(struct usbvirt_device *dev,
	    usb_endpoint_t endpoint, void *buffer, size_t size, size_t *data_size);
	
	usbvirt_control_transfer_t current_control_transfers[USB11_ENDPOINT_MAX];
} usbvirt_device_t;

#endif
/**
 * @}
 */
