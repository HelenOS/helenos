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
/** @addtogroup drvusbuhcihc
 * @{
 */
/** @file
 * @brief UHCI driver USB transaction structure
 */
#ifndef LIBUSB_HOST_BATCH_H
#define LIBUSB_HOST_BATCH_H

#include <adt/list.h>

#include <usbhc_iface.h>
#include <usb/usb.h>

typedef struct batch
{
	link_t link;
	usb_target_t target;
	usb_transfer_type_t transfer_type;
	usb_speed_t speed;
	usbhc_iface_transfer_in_callback_t callback_in;
	usbhc_iface_transfer_out_callback_t callback_out;
	char *buffer;
	char *transport_buffer;
	size_t buffer_size;
	char *setup_buffer;
	size_t setup_size;
	size_t max_packet_size;
	size_t transfered_size;
	void (*next_step)(struct batch*);
	int error;
	ddf_fun_t *fun;
	void *arg;
	void *private_data;
} batch_t;

void batch_init(
    batch_t *instance,
    usb_target_t target,
    usb_transfer_type_t transfer_type,
    usb_speed_t speed,
    size_t max_packet_size,
    char *buffer,
    char *transport_buffer,
    size_t buffer_size,
    char *setup_buffer,
    size_t setup_size,
    usbhc_iface_transfer_in_callback_t func_in,
    usbhc_iface_transfer_out_callback_t func_out,
    void *arg,
    ddf_fun_t *fun,
    void *private_data
);

static inline batch_t *batch_from_link(link_t *link_ptr)
{
	assert(link_ptr);
	return list_get_instance(link_ptr, batch_t, link);
}

void batch_call_in(batch_t *instance);
void batch_call_out(batch_t *instance);
void batch_finish(batch_t *instance, int error);
#endif
/**
 * @}
 */
