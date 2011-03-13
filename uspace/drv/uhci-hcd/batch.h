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
/** @addtogroup usb
 * @{
 */
/** @file
 * @brief UHCI driver
 */
#ifndef DRV_UHCI_BATCH_H
#define DRV_UHCI_BATCH_H

#include <adt/list.h>

#include <usbhc_iface.h>
#include <usb/usb.h>

#include "uhci_struct/transfer_descriptor.h"
#include "uhci_struct/queue_head.h"
#include "utils/device_keeper.h"

typedef struct batch
{
	link_t link;
	usb_speed_t speed;
	usb_target_t target;
	usb_transfer_type_t transfer_type;
	usbhc_iface_transfer_in_callback_t callback_in;
	usbhc_iface_transfer_out_callback_t callback_out;
	void *arg;
	char *transport_buffer;
	char *setup_buffer;
	size_t setup_size;
	char *buffer;
	size_t buffer_size;
	size_t max_packet_size;
	size_t packets;
	size_t transfered_size;
	int error;
	ddf_fun_t *fun;
	qh_t *qh;
	td_t *tds;
	void (*next_step)(struct batch*);
	device_keeper_t *manager;
} batch_t;

batch_t * batch_get(ddf_fun_t *fun, usb_target_t target,
    usb_transfer_type_t transfer_type, size_t max_packet_size,
    usb_speed_t speed, char *buffer, size_t size,
		char *setup_buffer, size_t setup_size,
    usbhc_iface_transfer_in_callback_t func_in,
    usbhc_iface_transfer_out_callback_t func_out, void *arg,
		device_keeper_t *manager
		);

void batch_dispose(batch_t *instance);

bool batch_is_complete(batch_t *instance);

void batch_control_write(batch_t *instance);

void batch_control_read(batch_t *instance);

void batch_interrupt_in(batch_t *instance);

void batch_interrupt_out(batch_t *instance);

void batch_bulk_in(batch_t *instance);

void batch_bulk_out(batch_t *instance);
#endif
/**
 * @}
 */
