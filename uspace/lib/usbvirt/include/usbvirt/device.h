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

/** @addtogroup libusbvirt
 * @{
 */
/** @file
 * @brief Virtual USB device.
 */
#ifndef LIBUSBVIRT_DEVICE_H_
#define LIBUSBVIRT_DEVICE_H_

#include <usb/usb.h>
#include <usb/descriptor.h>
#include <usb/devreq.h>

/** Request type of a control transfer. */
typedef enum {
	/** Standard USB request. */
	USBVIRT_REQUEST_TYPE_STANDARD = 0,
	/** Standard class USB request. */
	USBVIRT_REQUEST_TYPE_CLASS = 1
} usbvirt_request_type_t;

/** Recipient of control request. */
typedef enum {
	/** Device is the recipient of the control request. */
	USBVIRT_REQUEST_RECIPIENT_DEVICE = 0,
	/** Interface is the recipient of the control request. */
	USBVIRT_REQUEST_RECIPIENT_INTERFACE = 1,
	/** Endpoint is the recipient of the control request. */
	USBVIRT_REQUEST_RECIPIENT_ENDPOINT = 2,
	/** Other part of the device is the recipient of the control request. */
	USBVIRT_REQUEST_RECIPIENT_OTHER = 3
} usbvirt_request_recipient_t;

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

typedef struct usbvirt_device usbvirt_device_t;
struct usbvirt_control_transfer;

typedef int (*usbvirt_on_device_request_t)(usbvirt_device_t *dev,
	usb_device_request_setup_packet_t *request,
	uint8_t *data);

/** Callback for control request over pipe zero.
 *
 * @param dev Virtual device answering the call.
 * @param request Request setup packet.
 * @param data Data when DATA stage is present.
 * @return Error code.
 */
typedef int (*usbvirt_control_request_callback_t)(usbvirt_device_t *dev,
	usb_device_request_setup_packet_t *request,
	uint8_t *data);

/** Handler for control transfer on endpoint zero. */
typedef struct {
	/** Request type bitmap.
	 * Use USBVIRT_MAKE_CONTROL_REQUEST_TYPE for creating the bitmap.
	 */
	uint8_t request_type;
	/** Request code. */
	uint8_t request;
	/** Request name for debugging. */
	const char *name;
	/** Callback for the request.
	 * NULL value here announces end of a list.
	 */
	usbvirt_control_request_callback_t callback;
} usbvirt_control_transfer_handler_t;

/** Create control request type bitmap.
 *
 * @param direction Transfer direction (use usb_direction_t).
 * @param type Request type (use usbvirt_request_type_t).
 * @param recipient Recipient of the request (use usbvirt_request_recipient_t).
 * @return Request type bitmap.
 */
#define USBVIRT_MAKE_CONTROL_REQUEST_TYPE(direction, type, recipient) \
	((((direction) == USB_DIRECTION_IN) ? 1 : 0) << 7) \
	| (((type) & 3) << 5) \
	| (((recipient) & 31))

/** Create last item in an array of control request handlers. */
#define USBVIRT_CONTROL_TRANSFER_HANDLER_LAST { 0, 0, NULL, NULL }

/** Device operations. */
typedef struct {
	/** Callbacks for transfers over control pipe zero. */
	usbvirt_control_transfer_handler_t *control_transfer_handlers;

	int (*on_control_transfer)(usbvirt_device_t *dev,
	    usb_endpoint_t endpoint, struct usbvirt_control_transfer *transfer);
	
	/** Callback for all other incoming data. */
	int (*on_data)(usbvirt_device_t *dev,
	    usb_endpoint_t endpoint, void *buffer, size_t size);
	
	/** Callback for host request for data. */
	int (*on_data_request)(usbvirt_device_t *dev,
	    usb_endpoint_t endpoint, void *buffer, size_t size, size_t *actual_size);
	
	/** Decides direction of control transfer. */
	usb_direction_t (*decide_control_transfer_direction)(
	    usb_endpoint_t endpoint, void *buffer, size_t size);

	/** Callback when device changes its state.
	 *
	 * It is correct that this function is called when both states
	 * are equal (e.g. this function is called during SET_CONFIGURATION
	 * request done on already configured device).
	 *
	 * @warning The value of <code>dev->state</code> before calling
	 * this function is not specified (i.e. can be @p old_state or
	 * @p new_state).
	 */
	void (*on_state_change)(usbvirt_device_t *dev,
	    usbvirt_device_state_t old_state, usbvirt_device_state_t new_state);
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

/** Information about on-going control transfer.
 */
typedef struct usbvirt_control_transfer {
	/** Transfer direction (read/write control transfer). */
	usb_direction_t direction;
	/** Request data. */
	void *request;
	/** Size of request data. */
	size_t request_size;
	/** Payload. */
	void *data;
	/** Size of payload. */
	size_t data_size;
} usbvirt_control_transfer_t;

typedef enum {
	USBVIRT_DEBUGTAG_BASE = 1,
	USBVIRT_DEBUGTAG_TRANSACTION = 2,
	USBVIRT_DEBUGTAG_CONTROL_PIPE_ZERO = 4,
	USBVIRT_DEBUGTAG_ALL = 255
} usbvirt_debug_tags_t;

/** Virtual USB device. */
struct usbvirt_device {
	/** Callback device operations. */
	usbvirt_device_ops_t *ops;
	
	/** Custom device data. */
	void *device_data;

	/** Reply onto control transfer.
	 */
	int (*control_transfer_reply)(usbvirt_device_t *dev,
	    usb_endpoint_t endpoint, void *buffer, size_t size);
	
	/** Device name.
	 * Used in debug prints and sent to virtual host controller.
	 */
	const char *name;
	
	/** Standard descriptors. */
	usbvirt_descriptors_t *descriptors;
	
	/** Current device state. */
	usbvirt_device_state_t state;
	
	/** Device address. */
	usb_address_t address;
	/** New device address.
	 * This field is used during SET_ADDRESS request.
	 * On all other occasions, it holds invalid address (e.g. -1).
	 */
	usb_address_t new_address;
	
	/** Process OUT transaction. */
	int (*transaction_out)(usbvirt_device_t *dev,
	    usb_endpoint_t endpoint, void *buffer, size_t size);
	/** Process SETUP transaction. */
	int (*transaction_setup)(usbvirt_device_t *dev,
	    usb_endpoint_t endpoint, void *buffer, size_t size);
	/** Process IN transaction. */
	int (*transaction_in)(usbvirt_device_t *dev,
	    usb_endpoint_t endpoint, void *buffer, size_t size, size_t *data_size);
	
	/** State information on control-transfer endpoints. */
	usbvirt_control_transfer_t current_control_transfers[USB11_ENDPOINT_MAX];
	
	/* User debugging. */
	
	/** Debug print. */
	void (*debug)(usbvirt_device_t *dev, int level, uint8_t tag,
	    const char *format, ...);
	
	/** Current debug level. */
	int debug_level;
	
	/** Bitmap of currently enabled tags. */
	uint8_t debug_enabled_tags;
	
	/* Library debugging. */
	
	/** Debug print. */
	void (*lib_debug)(usbvirt_device_t *dev, int level, uint8_t tag,
	    const char *format, ...);
	
	/** Current debug level. */
	int lib_debug_level;
	
	/** Bitmap of currently enabled tags. */
	uint8_t lib_debug_enabled_tags;
};

#endif
/**
 * @}
 */
