/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libusbhost
 * @{
 */
/** @file
 * USB transfer transaction structures.
 */

#ifndef LIBUSBHOST_HOST_USB_TRANSFER_BATCH_H
#define LIBUSBHOST_HOST_USB_TRANSFER_BATCH_H

#include <errno.h>
#include <refcount.h>
#include <stddef.h>
#include <stdint.h>
#include <usb/dma_buffer.h>
#include <usb/request.h>
#include <usb/usb.h>
#include <usbhc_iface.h>

#include <usb/host/hcd.h>
#include <usb/host/endpoint.h>
#include <usb/host/bus.h>

typedef struct endpoint endpoint_t;
typedef struct bus bus_t;

/** Structure stores additional data needed for communication with EP */
typedef struct usb_transfer_batch {
	/** Target for communication */
	usb_target_t target;
	/** Direction of the transfer */
	usb_direction_t dir;

	/** Endpoint used for communication */
	endpoint_t *ep;

	/** Place to store SETUP data needed by control transfers */
	union {
		char buffer [USB_SETUP_PACKET_SIZE];
		usb_device_request_setup_packet_t packet;
		uint64_t packed;
	} setup;

	/** DMA buffer with enforced policy */
	dma_buffer_t dma_buffer;
	/** Size of memory buffer */
	size_t offset, size;

	/**
	 * In case a bounce buffer is allocated, the original buffer must to be
	 * stored to be filled after the IN transaction is finished.
	 */
	char *original_buffer;
	bool is_bounced;

	/** Indicates success/failure of the communication */
	errno_t error;
	/** Actually used portion of the buffer */
	size_t transferred_size;

	/** Function called on completion */
	usbhc_iface_transfer_callback_t on_complete;
	/** Arbitrary data for the handler */
	void *on_complete_data;
} usb_transfer_batch_t;

/**
 * Printf formatting string for dumping usb_transfer_batch_t.
 *  [address:endpoint speed transfer_type-direction buffer_sizeB/max_packet_size]
 */
#define USB_TRANSFER_BATCH_FMT "[%d:%d %s %s-%s %zuB/%zu]"

/** Printf arguments for dumping usb_transfer_batch_t.
 * @param batch USB transfer batch to be dumped.
 */
#define USB_TRANSFER_BATCH_ARGS(batch) \
	((batch).ep->device->address), ((batch).ep->endpoint), \
	usb_str_speed((batch).ep->device->speed), \
	usb_str_transfer_type_short((batch).ep->transfer_type), \
	usb_str_direction((batch).dir), \
	(batch).size, (batch).ep->max_packet_size

/** Wrapper for bus operation. */
usb_transfer_batch_t *usb_transfer_batch_create(endpoint_t *);

/** Batch initializer. */
void usb_transfer_batch_init(usb_transfer_batch_t *, endpoint_t *);

/** Buffer handling */
bool usb_transfer_batch_bounce_required(usb_transfer_batch_t *);
errno_t usb_transfer_batch_bounce(usb_transfer_batch_t *);

/** Batch finalization. */
void usb_transfer_batch_finish(usb_transfer_batch_t *);

/** To be called from outside only when the transfer is not going to be finished
 * (i.o.w. until successfuly scheduling)
 */
void usb_transfer_batch_destroy(usb_transfer_batch_t *);

#endif

/**
 * @}
 */
