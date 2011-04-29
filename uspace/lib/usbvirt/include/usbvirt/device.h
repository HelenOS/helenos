/*
 * Copyright (c) 2011 Vojtech Horky
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
 * @brief Virtual USB device.
 */
#ifndef LIBUSBVIRT_DEVICE_H_
#define LIBUSBVIRT_DEVICE_H_

#include <usb/usb.h>
#include <usb/request.h>

#define USBVIRT_ENDPOINT_MAX 16

typedef struct usbvirt_device usbvirt_device_t;

typedef int (*usbvirt_on_data_to_device_t)(usbvirt_device_t *, usb_endpoint_t,
    usb_transfer_type_t, void *, size_t);
typedef int (*usbvirt_on_data_from_device_t)(usbvirt_device_t *, usb_endpoint_t,
    usb_transfer_type_t, void *, size_t, size_t *);
typedef int (*usbvirt_on_control_t)(usbvirt_device_t *,
    const usb_device_request_setup_packet_t *, uint8_t *, size_t *);

typedef struct {
	usb_direction_t req_direction;
	usb_request_recipient_t req_recipient;
	usb_request_type_t req_type;
	uint8_t request;
	const char *name;
	usbvirt_on_control_t callback;
} usbvirt_control_request_handler_t;

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
} usbvirt_descriptors_t;

/** Possible states of virtual USB device.
 * Notice that these are not 1:1 mappings to those in USB specification.
 */
typedef enum {
	/** Default state, device listens at default address. */
	USBVIRT_STATE_DEFAULT,
	/** Device has non-default address assigned. */
	USBVIRT_STATE_ADDRESS,
	/** Device is configured. */
	USBVIRT_STATE_CONFIGURED
} usbvirt_device_state_t;

typedef struct {
	usbvirt_on_data_to_device_t data_out[USBVIRT_ENDPOINT_MAX];
	usbvirt_on_data_from_device_t data_in[USBVIRT_ENDPOINT_MAX];
	usbvirt_control_request_handler_t *control;
	void (*state_changed)(usbvirt_device_t *dev,
	    usbvirt_device_state_t old_state, usbvirt_device_state_t new_state);
} usbvirt_device_ops_t;

struct usbvirt_device {
	const char *name;
	void *device_data;
	usbvirt_device_ops_t *ops;
	usbvirt_descriptors_t *descriptors;
	usb_address_t address;
	usbvirt_device_state_t state;
};

int usbvirt_device_plug(usbvirt_device_t *, const char *);

void usbvirt_control_reply_helper(const usb_device_request_setup_packet_t *,
    uint8_t *, size_t *, void *, size_t);

int usbvirt_control_write(usbvirt_device_t *, void *, size_t, void *, size_t);
int usbvirt_control_read(usbvirt_device_t *, void *, size_t, void *, size_t, size_t *);
int usbvirt_data_out(usbvirt_device_t *, usb_transfer_type_t, usb_endpoint_t,
    void *, size_t);
int usbvirt_data_in(usbvirt_device_t *, usb_transfer_type_t, usb_endpoint_t,
    void *, size_t, size_t *);


#endif
/**
 * @}
 */
