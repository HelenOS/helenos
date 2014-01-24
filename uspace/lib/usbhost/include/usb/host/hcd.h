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
 *
 */

#ifndef LIBUSBHOST_HOST_HCD_H
#define LIBUSBHOST_HOST_HCD_H

#include <usb/host/endpoint.h>
#include <usb/host/usb_bus.h>
#include <usb/host/usb_transfer_batch.h>
#include <usb/usb.h>

#include <assert.h>
#include <usbhc_iface.h>
#include <sys/types.h>

typedef struct hcd hcd_t;

typedef int (*schedule_hook_t)(hcd_t *, usb_transfer_batch_t *);
typedef int (*ep_add_hook_t)(hcd_t *, endpoint_t *);
typedef void (*ep_remove_hook_t)(hcd_t *, endpoint_t *);
typedef void (*interrupt_hook_t)(hcd_t *, uint32_t);
typedef int (*status_hook_t)(hcd_t *, uint32_t *);

typedef struct {
	/** Device specific driver data. */
	void *data;
	/** Transfer scheduling, implement in device driver. */
	schedule_hook_t schedule;
	/** Hook called upon registering new endpoint. */
	ep_add_hook_t ep_add_hook;
	/** Hook called upon removing of an endpoint. */
	ep_remove_hook_t ep_remove_hook;
	/** Hook to be called on device interrupt, passes ARG1 */
	interrupt_hook_t irq_hook;
	/** Periodic polling hook */
	status_hook_t status_hook;
} hc_driver_t;

/** Generic host controller driver structure. */
struct hcd {
	/** Endpoint manager. */
	usb_bus_t bus;

	/** Driver implementation */
	hc_driver_t driver;

	/** Interrupt replacement fibril */
	fid_t polling_fibril;
};

void hcd_init(hcd_t *hcd, usb_speed_t max_speed, size_t bandwidth,
    bw_count_func_t bw_count);

static inline void hcd_set_implementation(hcd_t *hcd, void *data,
    schedule_hook_t schedule, ep_add_hook_t add_hook, ep_remove_hook_t rem_hook,
    interrupt_hook_t irq_hook, status_hook_t status_hook)
{
	assert(hcd);
	hcd->driver.data = data;
	hcd->driver.schedule = schedule;
	hcd->driver.ep_add_hook = add_hook;
	hcd->driver.ep_remove_hook = rem_hook;
	hcd->driver.irq_hook = irq_hook;
	hcd->driver.status_hook = status_hook;
}

usb_address_t hcd_request_address(hcd_t *hcd, usb_speed_t speed);

int hcd_release_address(hcd_t *hcd, usb_address_t address);

int hcd_reserve_default_address(hcd_t *hcd, usb_speed_t speed);

static inline int hcd_release_default_address(hcd_t *hcd)
{
	return hcd_release_address(hcd, USB_ADDRESS_DEFAULT);
}

int hcd_add_ep(hcd_t *hcd, usb_target_t target, usb_direction_t dir,
    usb_transfer_type_t type, size_t max_packet_size, unsigned packets,
    size_t size, usb_address_t tt_address, unsigned tt_port);

int hcd_remove_ep(hcd_t *hcd, usb_target_t target, usb_direction_t dir);

int hcd_send_batch(hcd_t *hcd, usb_target_t target, usb_direction_t direction,
    void *data, size_t size, uint64_t setup_data,
    usbhc_iface_transfer_in_callback_t in,
    usbhc_iface_transfer_out_callback_t out, void *arg, const char* name);

ssize_t hcd_send_batch_sync(hcd_t *hcd, usb_target_t target,
    usb_direction_t dir, void *data, size_t size, uint64_t setup_data,
    const char* name);

#endif
/**
 * @}
 */
