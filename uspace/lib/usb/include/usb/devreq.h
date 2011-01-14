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

/** @addtogroup libusb usb
 * @{
 */
/** @file
 * @brief Standard USB device requests.
 */
#ifndef LIBUSB_DEVREQ_H_
#define LIBUSB_DEVREQ_H_

#include <ipc/ipc.h>
#include <async.h>
#include <usb/usb.h>
#include <usb/descriptor.h>

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

/** Device request setup packet.
 * The setup packet describes the request.
 */
typedef struct {
	/** Request type.
	 * The type combines transfer direction, request type and
	 * intended recipient.
	 */
	uint8_t request_type;
	/** Request identification. */
	uint8_t request;
	/** Main parameter to the request. */
	union {
		/* FIXME: add #ifdefs according to host endianess */
		struct {
			uint8_t value_low;
			uint8_t value_high;
		};
		uint16_t value;
	};
	/** Auxiliary parameter to the request.
	 * Typically, it is offset to something.
	 */
	uint16_t index;
	/** Length of extra data. */
	uint16_t length;
} __attribute__ ((packed)) usb_device_request_setup_packet_t;


int usb_drv_req_get_status(int, usb_address_t, usb_request_recipient_t,
    uint16_t, uint16_t *);
int usb_drv_req_clear_feature(int, usb_address_t, usb_request_recipient_t,
    uint16_t, uint16_t);
int usb_drv_req_set_feature(int, usb_address_t, usb_request_recipient_t,
    uint16_t, uint16_t);
int usb_drv_req_set_address(int, usb_address_t, usb_address_t);
int usb_drv_req_get_descriptor(int, usb_address_t, usb_request_type_t,
    uint8_t, uint8_t, uint16_t, void *, size_t, size_t *);
int usb_drv_req_get_device_descriptor(int, usb_address_t,
    usb_standard_device_descriptor_t *);
int usb_drv_req_get_bare_configuration_descriptor(int, usb_address_t, int,
    usb_standard_configuration_descriptor_t *);
int usb_drv_req_get_full_configuration_descriptor(int, usb_address_t, int,
    void *, size_t, size_t *);
int usb_drv_req_set_descriptor(int, usb_address_t, uint8_t, uint8_t, uint16_t,
    void *, size_t);
int usb_drv_req_get_configuration(int, usb_address_t, uint8_t *);
int usb_drv_req_set_configuration(int, usb_address_t, uint8_t);
int usb_drv_req_get_interface(int, usb_address_t, uint16_t, uint8_t *);
int usb_drv_req_set_interface(int, usb_address_t, uint16_t, uint8_t);

#endif
/**
 * @}
 */
