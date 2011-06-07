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

typedef struct usb_transfer_batch usb_transfer_batch_t;
struct usb_transfer_batch {
	endpoint_t *ep;
	link_t link;
	usbhc_iface_transfer_in_callback_t callback_in;
	usbhc_iface_transfer_out_callback_t callback_out;
	void *arg;
	char *buffer;
	char *data_buffer;
	size_t buffer_size;
	char *setup_buffer;
	size_t setup_size;
	size_t transfered_size;
	void (*next_step)(usb_transfer_batch_t *);
	int error;
	ddf_fun_t *fun;
	void *private_data;
	void (*private_data_dtor)(void *p_data);
};

void usb_transfer_batch_init(
    usb_transfer_batch_t *instance,
    endpoint_t *ep,
    char *buffer,
    char *data_buffer,
    size_t buffer_size,
    char *setup_buffer,
    size_t setup_size,
    usbhc_iface_transfer_in_callback_t func_in,
    usbhc_iface_transfer_out_callback_t func_out,
    void *arg,
    ddf_fun_t *fun,
    void *private_data,
    void (*private_data_dtor)(void *p_data)
);

void usb_transfer_batch_call_in_and_dispose(usb_transfer_batch_t *instance);
void usb_transfer_batch_call_out_and_dispose(usb_transfer_batch_t *instance);
void usb_transfer_batch_finish(usb_transfer_batch_t *instance);
void usb_transfer_batch_dispose(usb_transfer_batch_t *instance);

static inline void usb_transfer_batch_finish_error(
    usb_transfer_batch_t *instance, int error)
{
	assert(instance);
	instance->error = error;
	usb_transfer_batch_finish(instance);
}

static inline usb_transfer_batch_t *usb_transfer_batch_from_link(link_t *l)
{
	assert(l);
	return list_get_instance(l, usb_transfer_batch_t, link);
}

#endif
/**
 * @}
 */
