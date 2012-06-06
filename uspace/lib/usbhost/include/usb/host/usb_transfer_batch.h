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

#include <adt/list.h>

#include <usbhc_iface.h>
#include <usb/usb.h>
#include <usb/host/endpoint.h>

#define USB_SETUP_PACKET_SIZE 8

/** Structure stores additional data needed for communication with EP */
typedef struct usb_transfer_batch {
	/** Endpoint used for communication */
	endpoint_t *ep;
	/** Function called on completion (IN version) */
	usbhc_iface_transfer_in_callback_t callback_in;
	/** Function called on completion (OUT version) */
	usbhc_iface_transfer_out_callback_t callback_out;
	/** Argument to pass to the completion function */
	void *arg;
	/** Place for data to send/receive */
	char *buffer;
	/** Size of memory pointed to by buffer member */
	size_t buffer_size;
	/** Place to store SETUP data needed by control transfers */
	char setup_buffer[USB_SETUP_PACKET_SIZE];
	/** Used portion of setup_buffer member
	 *
	 * SETUP buffer must be 8 bytes for control transfers and is left
	 * unused for all other transfers. Thus, this field is either 0 or 8.
	 */
	size_t setup_size;
	/** Host controller function, passed to callback function */
	ddf_fun_t *fun;

	/** Actually used portion of the buffer
	 * This member is never accessed by functions provided in this header,
	 * with the exception of usb_transfer_batch_finish. For external use.
	 */
	size_t transfered_size;
	/** Indicates success/failure of the communication
	 * This member is never accessed by functions provided in this header,
	 * with the exception of usb_transfer_batch_finish. For external use.
	 */
	int error;

	/** Driver specific data */
	void *private_data;
	/** Callback to properly remove driver data during destruction */
	void (*private_data_dtor)(void *p_data);
} usb_transfer_batch_t;

/** Printf formatting string for dumping usb_transfer_batch_t. */
#define USB_TRANSFER_BATCH_FMT "[%d:%d %s %s-%s %zuB/%zu]"

/** Printf arguments for dumping usb_transfer_batch_t.
 * @param batch USB transfer batch to be dumped.
 */
#define USB_TRANSFER_BATCH_ARGS(batch) \
	(batch).ep->address, (batch).ep->endpoint, \
	usb_str_speed((batch).ep->speed), \
	usb_str_transfer_type_short((batch).ep->transfer_type), \
	usb_str_direction((batch).ep->direction), \
	(batch).buffer_size, (batch).ep->max_packet_size


usb_transfer_batch_t * usb_transfer_batch_create(
    endpoint_t *ep,
    char *buffer,
    size_t buffer_size,
    uint64_t setup_buffer,
    usbhc_iface_transfer_in_callback_t func_in,
    usbhc_iface_transfer_out_callback_t func_out,
    void *arg,
    ddf_fun_t *fun,
    void *private_data,
    void (*private_data_dtor)(void *p_data)
);
void usb_transfer_batch_destroy(const usb_transfer_batch_t *instance);

void usb_transfer_batch_finish_error(const usb_transfer_batch_t *instance,
    const void* data, size_t size, int error);

/** Finish batch using stored error value and transferred size.
 *
 * @param[in] instance Batch structure to use.
 * @param[in] data Data to copy to the output buffer.
 */
static inline void usb_transfer_batch_finish(
    const usb_transfer_batch_t *instance, const void* data)
{
	assert(instance);
	usb_transfer_batch_finish_error(
	    instance, data, instance->transfered_size, instance->error);
}

/** Determine batch direction based on the callbacks present
 * @param[in] instance Batch structure to use, non-null.
 * @return USB_DIRECTION_IN, or USB_DIRECTION_OUT.
 */
static inline usb_direction_t usb_transfer_batch_direction(
    const usb_transfer_batch_t *instance)
{
	assert(instance);
	if (instance->callback_in) {
		assert(instance->callback_out == NULL);
		assert(instance->ep == NULL
		    || instance->ep->transfer_type == USB_TRANSFER_CONTROL
		    || instance->ep->direction == USB_DIRECTION_IN);
		return USB_DIRECTION_IN;
	}
	if (instance->callback_out) {
		assert(instance->callback_in == NULL);
		assert(instance->ep == NULL
		    || instance->ep->transfer_type == USB_TRANSFER_CONTROL
		    || instance->ep->direction == USB_DIRECTION_OUT);
		return USB_DIRECTION_OUT;
	}
	assert(false);
}

#endif

/**
 * @}
 */
