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
#ifndef DRV_UHCI_TRACKER_H
#define DRV_UHCI_TRACKER_H

#include <adt/list.h>

#include <usbhc_iface.h>
#include <usb/usb.h>

#include "uhci_struct/transfer_descriptor.h"

typedef enum {
	LOW_SPEED,
	FULL_SPEED,
} dev_speed_t;

typedef struct tracker
{
	link_t link;
	usb_target_t target;
	usb_transfer_type_t transfer_type;
	union {
		usbhc_iface_transfer_in_callback_t callback_in;
		usbhc_iface_transfer_out_callback_t callback_out;
	};
	void *arg;
	char *buffer;
	char *packet;
	size_t buffer_size;
	size_t max_packet_size;
	size_t packet_size;
	size_t buffer_offset;
	dev_speed_t speed;
	device_t *dev;
	transfer_descriptor_t *td;
	void (*next_step)(struct tracker*);
	unsigned toggle:1;
} tracker_t;


tracker_t * tracker_get(device_t *dev, usb_target_t target,
    usb_transfer_type_t transfer_type, size_t max_packet_size,
    dev_speed_t speed, char *buffer, size_t size,
    usbhc_iface_transfer_in_callback_t func_in,
    usbhc_iface_transfer_out_callback_t func_out, void *arg);

void tracker_control_write(tracker_t *instance);

void tracker_control_read(tracker_t *instance);

void tracker_interrupt_in(tracker_t *instance);

void tracker_interrupt_out(tracker_t *instance);

/* DEPRECATED FUNCTIONS NEEDED BY THE OLD API */
void tracker_control_setup_old(tracker_t *instance);

void tracker_control_write_data_old(tracker_t *instance);

void tracker_control_read_data_old(tracker_t *instance);

void tracker_control_write_status_old(tracker_t *instance);

void tracker_control_read_status_old(tracker_t *instance);
#endif
/**
 * @}
 */
