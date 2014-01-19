/*
 * Copyright (c) 2012 Jan Vesely
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

/** @addtogroup libusb
 * @{
 */
/** @file
 * Standard USB request format.
 */
#ifndef LIBUSB_REQUEST_H_
#define LIBUSB_REQUEST_H_

#include <sys/types.h>

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

/** Device request setup packet.
 * The setup packet describes the request.
 */
typedef struct {
	/** Request type.
	 * The type combines transfer direction, request type and
	 * intended recipient.
	 */
	uint8_t request_type;
#define SETUP_REQUEST_TYPE_DEVICE_TO_HOST (1 << 7)
#define SETUP_REQUEST_TYPE_HOST_TO_DEVICE (0 << 7)
#define SETUP_REQUEST_TYPE_GET_TYPE(rt) ((rt >> 5) & 0x3)
#define SETUP_REQUEST_TYPE_GET_RECIPIENT(rec) (rec & 0x1f)
#define SETUP_REQUEST_TO_HOST(type, recipient) \
    (uint8_t)((1 << 7) | ((type & 0x3) << 5) | (recipient & 0x1f))
#define SETUP_REQUEST_TO_DEVICE(type, recipient) \
    (uint8_t)(((type & 0x3) << 5) | (recipient & 0x1f))

	/** Request identification. */
	uint8_t request;
	/** Main parameter to the request. */
	union __attribute__ ((packed)) {
		uint16_t value;
		/* FIXME: add #ifdefs according to host endianness */
		struct __attribute__ ((packed)) {
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
} __attribute__ ((packed)) usb_device_request_setup_packet_t;

int assert[(sizeof(usb_device_request_setup_packet_t) == 8) ? 1: -1];

int usb_request_needs_toggle_reset(
    const usb_device_request_setup_packet_t *request);

#endif
/**
 * @}
 */
