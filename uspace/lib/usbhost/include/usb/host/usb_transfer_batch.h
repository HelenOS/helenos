/*
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

/** @addtogroup libusbhost
 * @{
 */
/** @file
 * USB transfer transaction structures.
 */

#ifndef LIBUSBHOST_HOST_USB_TRANSFER_BATCH_H
#define LIBUSBHOST_HOST_USB_TRANSFER_BATCH_H

#include <usb/usb.h>
#include <usb/request.h>
#include <usb/host/bus.h>
#include <usbhc_iface.h>

#include <atomic.h>
#include <stddef.h>
#include <errno.h>
#include <stdint.h>

typedef struct endpoint endpoint_t;
typedef struct bus bus_t;

/** Structure stores additional data needed for communication with EP */
typedef struct usb_transfer_batch {
	/** Target for communication */
	usb_target_t target;

	/** Endpoint used for communication */
	endpoint_t *ep;

	/** Size reported to be sent */
	size_t expected_size;

	/** Direction of the transfer */
	usb_direction_t dir;

	/** Function called on completion */
	usbhc_iface_transfer_callback_t on_complete;
	/** Arbitrary data for the handler */
	void *on_complete_data;

	/** Place to store SETUP data needed by control transfers */
	union {
		char buffer [USB_SETUP_PACKET_SIZE];
		usb_device_request_setup_packet_t packet;
		uint64_t packed;
	} setup;

	/** Resetting the Toggle */
	toggle_reset_mode_t toggle_reset_mode;

	/** Place for data to send/receive */
	char *buffer;
	/** Size of memory pointed to by buffer member */
	size_t buffer_size;
	/** Actually used portion of the buffer */
	size_t transfered_size;

	/** Indicates success/failure of the communication */
	int error;
} usb_transfer_batch_t;

/** Printf formatting string for dumping usb_transfer_batch_t. */
#define USB_TRANSFER_BATCH_FMT "[%d:%d %s %s-%s %zuB/%zu]"

/** Printf arguments for dumping usb_transfer_batch_t.
 * @param batch USB transfer batch to be dumped.
 */
#define USB_TRANSFER_BATCH_ARGS(batch) \
	(batch).target.address, (batch).target.endpoint, \
	usb_str_speed((batch).ep->device->speed), \
	usb_str_transfer_type_short((batch).ep->transfer_type), \
	usb_str_direction((batch).ep->direction), \
	(batch).buffer_size, (batch).ep->max_packet_size

/** Wrapper for bus operation. */
usb_transfer_batch_t *usb_transfer_batch_create(endpoint_t *);

/** Batch initializer. */
void usb_transfer_batch_init(usb_transfer_batch_t *, endpoint_t *);

/** Call after status is known, but before releasing endpoint */
int usb_transfer_batch_reset_toggle(usb_transfer_batch_t *);

/** Batch finalization. */
void usb_transfer_batch_abort(usb_transfer_batch_t *);
void usb_transfer_batch_finish(usb_transfer_batch_t *);

/** To be called from outside only when the transfer is not going to be finished
 * (i.o.w. until successfuly scheduling)
 */
void usb_transfer_batch_destroy(usb_transfer_batch_t *);

#endif

/**
 * @}
 */
