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
#ifndef LIBUSBHOST_HOST_BATCH_H
#define LIBUSBHOST_HOST_BATCH_H

#include <adt/list.h>

#include <usbhc_iface.h>
#include <usb/usb.h>
#include <usb/host/endpoint.h>

#define USB_SETUP_PACKET_SIZE 8

typedef struct usb_transfer_batch usb_transfer_batch_t;
struct usb_transfer_batch {
	endpoint_t *ep;
	usbhc_iface_transfer_in_callback_t callback_in;
	usbhc_iface_transfer_out_callback_t callback_out;
	void *arg;
	char *buffer;
	size_t buffer_size;
	char setup_buffer[USB_SETUP_PACKET_SIZE];
	size_t setup_size;
	size_t transfered_size;
	int error;
	ddf_fun_t *fun;
	void *private_data;
	void (*private_data_dtor)(void *p_data);
};

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


usb_transfer_batch_t * usb_transfer_batch_get(
    endpoint_t *ep,
    char *buffer,
    size_t buffer_size,
    char setup_buffer[USB_SETUP_PACKET_SIZE],
    usbhc_iface_transfer_in_callback_t func_in,
    usbhc_iface_transfer_out_callback_t func_out,
    void *arg,
    ddf_fun_t *fun,
    void *private_data,
    void (*private_data_dtor)(void *p_data)
);

void usb_transfer_batch_finish(usb_transfer_batch_t *instance,
    const void* data, size_t size);
void usb_transfer_batch_call_in(usb_transfer_batch_t *instance);
void usb_transfer_batch_call_out(usb_transfer_batch_t *instance);
void usb_transfer_batch_dispose(usb_transfer_batch_t *instance);

/** Helper function, calls callback and correctly destroys batch structure.
 *
 * @param[in] instance Batch structure to use.
 */
static inline void usb_transfer_batch_call_in_and_dispose(
    usb_transfer_batch_t *instance)
{
	assert(instance);
	usb_transfer_batch_call_in(instance);
	usb_transfer_batch_dispose(instance);
}
/*----------------------------------------------------------------------------*/
/** Helper function calls callback and correctly destroys batch structure.
 *
 * @param[in] instance Batch structure to use.
 */
static inline void usb_transfer_batch_call_out_and_dispose(
    usb_transfer_batch_t *instance)
{
	assert(instance);
	usb_transfer_batch_call_out(instance);
	usb_transfer_batch_dispose(instance);
}
/*----------------------------------------------------------------------------*/
static inline void usb_transfer_batch_finish_error(
    usb_transfer_batch_t *instance, const void* data, size_t size, int error)
{
	assert(instance);
	instance->error = error;
	usb_transfer_batch_finish(instance, data, size);
}
#endif
/**
 * @}
 */
