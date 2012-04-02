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
 * Virtual USB device.
 */

#ifndef LIBUSBVIRT_DEVICE_H_
#define LIBUSBVIRT_DEVICE_H_

#include <usb/usb.h>
#include <usb/dev/request.h>
#include <async.h>

/** Maximum number of endpoints supported by virtual USB. */
#define USBVIRT_ENDPOINT_MAX 16

typedef struct usbvirt_device usbvirt_device_t;

/** Callback for data to device (OUT transaction).
 *
 * @param dev Virtual device to which the transaction belongs.
 * @param endpoint Target endpoint number.
 * @param transfer_type Transfer type.
 * @param buffer Data buffer.
 * @param buffer_size Size of the buffer in bytes.
 * @return Error code.
 */
typedef int (*usbvirt_on_data_to_device_t)(usbvirt_device_t *dev,
    usb_endpoint_t endpoint, usb_transfer_type_t transfer_type,
    void *buffer, size_t buffer_size);

/** Callback for data from device (IN transaction).
 *
 * @param dev Virtual device to which the transaction belongs.
 * @param endpoint Target endpoint number.
 * @param transfer_type Transfer type.
 * @param buffer Data buffer to write answer to.
 * @param buffer_size Size of the buffer in bytes.
 * @param act_buffer_size Write here how many bytes were actually written.
 * @return Error code.
 */
typedef int (*usbvirt_on_data_from_device_t)(usbvirt_device_t *dev,
    usb_endpoint_t endpoint, usb_transfer_type_t transfer_type,
    void *buffer, size_t buffer_size, size_t *act_buffer_size);

/** Callback for control transfer on endpoint zero.
 *
 * Notice that size of the data buffer is expected to be read from the
 * setup packet.
 *
 * @param dev Virtual device to which the transaction belongs.
 * @param setup_packet Standard setup packet.
 * @param data Data (might be NULL).
 * @param act_data_size Size of returned data in bytes.
 * @return Error code.
 */
typedef int (*usbvirt_on_control_t)(usbvirt_device_t *dev,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_data_size);

/** Callback for control request on a virtual USB device.
 *
 * See usbvirt_control_reply_helper() for simple way of answering
 * control read requests.
 */
typedef struct {
	/** Request direction (in or out). */
	usb_direction_t req_direction;
	/** Request recipient (device, interface or endpoint). */
	usb_request_recipient_t req_recipient;
	/** Request type (standard, class or vendor). */
	usb_request_type_t req_type;
	/** Actual request code. */
	uint8_t request;
	/** Request handler name for debugging purposes. */
	const char *name;
	/** Callback to be executed on matching request. */
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

/** Standard USB descriptors for virtual device. */
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

/** Ops structure for virtual USB device. */
typedef struct {
	/** Callbacks for data to device.
	 * Index zero is ignored.
	 */
	usbvirt_on_data_to_device_t data_out[USBVIRT_ENDPOINT_MAX];
	/** Callbacks for data from device.
	 * Index zero is ignored.
	 */
	usbvirt_on_data_from_device_t data_in[USBVIRT_ENDPOINT_MAX];
	/** Array of control handlers.
	 * Last handler is expected to have the @c callback field set to NULL
	 */
	usbvirt_control_request_handler_t *control;
	/** Callback when device changes state.
	 *
	 * The value of @c state attribute of @p dev device is not
	 * defined during call of this function.
	 *
	 * @param dev The virtual USB device.
	 * @param old_state Old device state.
	 * @param new_state New device state.
	 */
	void (*state_changed)(usbvirt_device_t *dev,
	    usbvirt_device_state_t old_state, usbvirt_device_state_t new_state);
} usbvirt_device_ops_t;

/** Virtual USB device. */
struct usbvirt_device {
	/** Name for debugging purposes. */
	const char *name;
	/** Custom device data. */
	void *device_data;
	/** Device ops. */
	usbvirt_device_ops_t *ops;
	/** Device descriptors. */
	usbvirt_descriptors_t *descriptors;
	/** Current device address.
	 * You shall treat this field as read only in your code.
	 */
	usb_address_t address;
	/** Current device state.
	 * You shall treat this field as read only in your code.
	 */
	usbvirt_device_state_t state;
	/** Session to the host controller.
	 * You shall treat this field as read only in your code.
	 */
	async_sess_t *vhc_sess;
};

int usbvirt_device_plug(usbvirt_device_t *, const char *);
void usbvirt_device_unplug(usbvirt_device_t *);

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
