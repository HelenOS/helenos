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
 * @brief Base USB types.
 */
#ifndef LIBUSB_USB_H_
#define LIBUSB_USB_H_

#include <sys/types.h>
#include <ipc/ipc.h>

/** USB transfer type. */
typedef enum {
	USB_TRANSFER_CONTROL = 0,
	USB_TRANSFER_ISOCHRONOUS = 1,
	USB_TRANSFER_BULK = 2,
	USB_TRANSFER_INTERRUPT = 3
} usb_transfer_type_t;

const char * usb_str_transfer_type(usb_transfer_type_t t);

/** USB data transfer direction. */
typedef enum {
	USB_DIRECTION_IN,
	USB_DIRECTION_OUT
} usb_direction_t;

/** USB transaction outcome. */
typedef enum {
	USB_OUTCOME_OK,
	USB_OUTCOME_CRCERROR,
	USB_OUTCOME_BABBLE
} usb_transaction_outcome_t;

const char * usb_str_transaction_outcome(usb_transaction_outcome_t o);

/** USB address type.
 * Negative values could be used to indicate error.
 */
typedef int usb_address_t;

/** Default USB address. */
#define USB_ADDRESS_DEFAULT 0
/** Maximum address number in USB 1.1. */
#define USB11_ADDRESS_MAX 128

/** USB endpoint number type.
 * Negative values could be used to indicate error.
 */
typedef int usb_endpoint_t;

/** Maximum endpoint number in USB 1.1.
 */
#define USB11_ENDPOINT_MAX 16


/** USB complete address type. 
 * Pair address + endpoint is identification of transaction recipient.
 */
typedef struct {
	usb_address_t address;
	usb_endpoint_t endpoint;
} usb_target_t;

static inline int usb_target_same(usb_target_t a, usb_target_t b)
{
	return (a.address == b.address)
	    && (a.endpoint == b.endpoint);
}

/** General handle type.
 * Used by various USB functions as opaque handle.
 */
typedef ipcarg_t usb_handle_t;

/** USB packet identifier. */
typedef enum {
#define _MAKE_PID_NIBBLE(tag, type) \
	((uint8_t)(((tag) << 2) | (type)))
#define _MAKE_PID(tag, type) \
	( \
	    _MAKE_PID_NIBBLE(tag, type) \
	    | ((~_MAKE_PID_NIBBLE(tag, type)) << 4) \
	)
	USB_PID_OUT = _MAKE_PID(0, 1),
	USB_PID_IN = _MAKE_PID(2, 1),
	USB_PID_SOF = _MAKE_PID(1, 1),
	USB_PID_SETUP = _MAKE_PID(3, 1),

	USB_PID_DATA0 = _MAKE_PID(0 ,3),
	USB_PID_DATA1 = _MAKE_PID(2 ,3),

	USB_PID_ACK = _MAKE_PID(0 ,2),
	USB_PID_NAK = _MAKE_PID(2 ,2),
	USB_PID_STALL = _MAKE_PID(3 ,2),

	USB_PID_PRE = _MAKE_PID(3 ,0),
	/* USB_PID_ = _MAKE_PID( ,), */
#undef _MAKE_PID
#undef _MAKE_PID_NIBBLE
} usb_packet_id;

#endif
/**
 * @}
 */
