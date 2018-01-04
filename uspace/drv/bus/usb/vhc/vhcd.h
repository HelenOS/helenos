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

#include <usbvirt/device.h>
#include <usbhc_iface.h>
#include <async.h>

#include <usb/host/hcd.h>

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
	usbvirt_device_t hub;
} vhc_data_t;

typedef struct {
	link_t link;
	usb_transfer_batch_t *batch;
} vhc_transfer_t;

void on_client_close(ddf_fun_t *fun);
void default_connection_handler(ddf_fun_t *fun, ipc_callid_t icallid,
    ipc_call_t *icall);

errno_t vhc_virtdev_plug(vhc_data_t *, async_sess_t *, uintptr_t *);
errno_t vhc_virtdev_plug_local(vhc_data_t *, usbvirt_device_t *, uintptr_t *);
errno_t vhc_virtdev_plug_hub(vhc_data_t *, usbvirt_device_t *, uintptr_t *, usb_address_t address);
void vhc_virtdev_unplug(vhc_data_t *, uintptr_t);

errno_t vhc_init(vhc_data_t *instance);
errno_t vhc_schedule(hcd_t *hcd, usb_transfer_batch_t *batch);
errno_t vhc_transfer_queue_processor(void *arg);

#endif

/**
 * @}
 */
