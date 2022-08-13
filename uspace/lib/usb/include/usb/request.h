/*
 * SPDX-FileCopyrightText: 2012 Jan Vesely
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libusb
 * @{
 */
/** @file
 * Standard USB request format.
 */
#ifndef LIBUSB_REQUEST_H_
#define LIBUSB_REQUEST_H_

#include <stdint.h>
#include <assert.h>

/** Standard device request. */
typedef enum {
	USB_DEVREQ_GET_STATUS = 0,
	USB_DEVREQ_CLEAR_FEATURE = 1,
	USB_DEVREQ_SET_FEATURE = 3,
	USB_DEVREQ_SET_ADDRESS = 5,
	USB_DEVREQ_GET_DESCRIPTOR = 6,
	USB_DEVREQ_SET_DESCRIPTOR = 7,
	USB_DEVREQ_GET_CONFIGURATION = 8,
	USB_DEVREQ_SET_CONFIGURATION = 9,
	USB_DEVREQ_GET_INTERFACE = 10,
	USB_DEVREQ_SET_INTERFACE = 11,
	USB_DEVREQ_SYNCH_FRAME = 12,
	USB_DEVREQ_LAST_STD
} usb_stddevreq_t;

/** Standard device features */
typedef enum {
	USB_FEATURE_ENDPOINT_HALT = 0,
	USB_FEATURE_DEVICE_REMOTE_WAKEUP = 1,
	USB_FEATURE_TEST_MODE = 2
} usb_std_feature_t;

/** USB device status - device is self powered (opposed to bus powered). */
#define USB_DEVICE_STATUS_SELF_POWERED ((uint16_t)(1 << 0))

/** USB device status - remote wake-up signaling is enabled. */
#define USB_DEVICE_STATUS_REMOTE_WAKEUP ((uint16_t)(1 << 1))

/** USB endpoint status - endpoint is halted (stalled). */
#define USB_ENDPOINT_STATUS_HALTED ((uint16_t)(1 << 0))

/** Size of the USB setup packet */
#define USB_SETUP_PACKET_SIZE 8

#define SETUP_REQUEST_TYPE_DEVICE_TO_HOST (1 << 7)
#define SETUP_REQUEST_TYPE_HOST_TO_DEVICE (0 << 7)
#define SETUP_REQUEST_TYPE_IS_DEVICE_TO_HOST(rt) ((rt) & (1 << 7))
#define SETUP_REQUEST_TYPE_GET_TYPE(rt) ((rt >> 5) & 0x3)
#define SETUP_REQUEST_TYPE_GET_RECIPIENT(rec) (rec & 0x1f)
#define SETUP_REQUEST_TO_HOST(type, recipient) \
    (uint8_t)((1 << 7) | ((type & 0x3) << 5) | (recipient & 0x1f))
#define SETUP_REQUEST_TO_DEVICE(type, recipient) \
    (uint8_t)(((type & 0x3) << 5) | (recipient & 0x1f))

/** Device request setup packet.
 * The setup packet describes the request.
 */
typedef union {
	struct __attribute__((packed)) {
		/** Request type.
		 * The type combines transfer direction, request type and
		 * intended recipient.
		 */
		uint8_t request_type;

		/** Request identification. */
		uint8_t request;
		/** Main parameter to the request. */
		union __attribute__((packed)) {
			uint16_t value;
			/* FIXME: add #ifdefs according to host endianness */
			struct __attribute__((packed)) {
				uint8_t value_low;
				uint8_t value_high;
			};
		};
		/** Auxiliary parameter to the request.
		 * Typically, it is offset to something.
		 */
		uint16_t index;
		/** Length of extra data. */
		uint16_t length;
	};
	uint64_t raw;
} __attribute__((packed)) usb_device_request_setup_packet_t;

static_assert(sizeof(usb_device_request_setup_packet_t) == USB_SETUP_PACKET_SIZE, "");

#define GET_DEVICE_DESC(size) \
{ \
	.request_type = SETUP_REQUEST_TYPE_DEVICE_TO_HOST \
	    | (USB_REQUEST_TYPE_STANDARD << 5) \
	    | USB_REQUEST_RECIPIENT_DEVICE, \
	.request = USB_DEVREQ_GET_DESCRIPTOR, \
	.value = uint16_host2usb(USB_DESCTYPE_DEVICE << 8), \
	.index = uint16_host2usb(0), \
	.length = uint16_host2usb(size), \
};

#define SET_ADDRESS(address) \
{ \
	.request_type = SETUP_REQUEST_TYPE_HOST_TO_DEVICE \
	    | (USB_REQUEST_TYPE_STANDARD << 5) \
	    | USB_REQUEST_RECIPIENT_DEVICE, \
	.request = USB_DEVREQ_SET_ADDRESS, \
	.value = uint16_host2usb(address), \
	.index = uint16_host2usb(0), \
	.length = uint16_host2usb(0), \
};

#define CTRL_PIPE_MIN_PACKET_SIZE 8

#endif
/**
 * @}
 */
