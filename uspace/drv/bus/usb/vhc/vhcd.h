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

/** @addtogroup drvusbvhc
 * @{
 */
/** @file
 * @brief Virtual USB host controller common definitions.
 */
#ifndef VHCD_VHCD_H_
#define VHCD_VHCD_H_

#include <usb/debug.h>
#include <usbvirt/device.h>
#include <usb/host/usb_endpoint_manager.h>
#include <usb/host/usb_device_manager.h>
#include <usbhc_iface.h>
#include <async.h>

#define NAME "vhc"

typedef struct {
	link_t link;
	async_sess_t *dev_sess;
	usbvirt_device_t *dev_local;
	bool plugged;
	usb_address_t address;
	fibril_mutex_t guard;
	list_t transfer_queue;
} vhc_virtdev_t;

typedef struct {
	uint32_t magic;
	list_t devices;
	fibril_mutex_t guard;
	usb_endpoint_manager_t ep_manager;
	usb_device_manager_t dev_manager;
	usbvirt_device_t *hub;
	ddf_fun_t *hc_fun;
} vhc_data_t;

typedef struct {
	link_t link;
	usb_address_t address;
	usb_endpoint_t endpoint;
	usb_direction_t direction;
	usb_transfer_type_t transfer_type;
	void *setup_buffer;
	size_t setup_buffer_size;
	void *data_buffer;
	size_t data_buffer_size;
	ddf_fun_t *ddf_fun;
	void *callback_arg;
	usbhc_iface_transfer_in_callback_t callback_in;
	usbhc_iface_transfer_out_callback_t callback_out;
} vhc_transfer_t;

vhc_transfer_t *vhc_transfer_create(usb_address_t, usb_endpoint_t,
    usb_direction_t, usb_transfer_type_t, ddf_fun_t *, void *);
int vhc_virtdev_plug(vhc_data_t *, async_sess_t *, uintptr_t *);
int vhc_virtdev_plug_local(vhc_data_t *, usbvirt_device_t *, uintptr_t *);
int vhc_virtdev_plug_hub(vhc_data_t *, usbvirt_device_t *, uintptr_t *);
void vhc_virtdev_unplug(vhc_data_t *, uintptr_t);
int vhc_virtdev_add_transfer(vhc_data_t *, vhc_transfer_t *);

int vhc_transfer_queue_processor(void *arg);


#endif
/**
 * @}
 */
