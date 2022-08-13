/*
 * SPDX-FileCopyrightText: 2010 Vojtech Horky
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
#include <async.h>
#include <macros.h>
#include <member.h>

#include <usb/host/hcd.h>
#include <usb/host/usb2_bus.h>
#include <usb/host/usb_transfer_batch.h>

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
	hc_device_t base;

	bus_t bus;
	usb2_bus_helper_t bus_helper;

	ddf_fun_t *virtual_fun;
	list_t devices;
	fibril_mutex_t guard;
	usbvirt_device_t hub;
} vhc_data_t;

typedef struct {
	usb_transfer_batch_t batch;
	link_t link;
} vhc_transfer_t;

static inline vhc_data_t *hcd_to_vhc(hc_device_t *hcd)
{
	assert(hcd);
	return (vhc_data_t *) hcd;
}

static inline vhc_data_t *bus_to_vhc(bus_t *bus)
{
	assert(bus);
	return member_to_inst(bus, vhc_data_t, bus);
}

void on_client_close(ddf_fun_t *fun);
void default_connection_handler(ddf_fun_t *fun, ipc_call_t *icall);

errno_t vhc_virtdev_plug(vhc_data_t *, async_sess_t *, uintptr_t *);
errno_t vhc_virtdev_plug_local(vhc_data_t *, usbvirt_device_t *, uintptr_t *);
errno_t vhc_virtdev_plug_hub(vhc_data_t *, usbvirt_device_t *, uintptr_t *,
    usb_address_t address);
void vhc_virtdev_unplug(vhc_data_t *, uintptr_t);

errno_t vhc_init(vhc_data_t *);
errno_t vhc_schedule(usb_transfer_batch_t *);
errno_t vhc_transfer_queue_processor(void *arg);

#endif

/**
 * @}
 */
