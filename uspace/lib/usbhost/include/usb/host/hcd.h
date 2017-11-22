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

#include <assert.h>
#include <mem.h>
#include <stddef.h>
#include <stdint.h>
#include <usb/usb.h>
#include <usbhc_iface.h>

typedef struct hcd hcd_t;
typedef struct bus bus_t;
typedef struct device device_t;
typedef struct usb_transfer_batch usb_transfer_batch_t;

typedef int (*schedule_hook_t)(hcd_t *, usb_transfer_batch_t *);
typedef void (*interrupt_hook_t)(hcd_t *, uint32_t);
typedef int (*status_hook_t)(hcd_t *, uint32_t *);
typedef int (*address_device_hook_t)(hcd_t *, usb_speed_t, usb_tt_address_t, usb_address_t *);

typedef struct {
	/** Transfer scheduling, implement in device driver. */
	schedule_hook_t schedule;
	/** Hook to be called on device interrupt, passes ARG1 */
	interrupt_hook_t irq_hook;
	/** Periodic polling hook */
	status_hook_t status_hook;
	/** Hook to setup device address */
	address_device_hook_t address_device;
} hcd_ops_t;

/** Generic host controller driver structure. */
struct hcd {
	/** Endpoint manager. */
	bus_t *bus;

	/** Interrupt replacement fibril */
	fid_t polling_fibril;

	/** Driver implementation */
	hcd_ops_t ops;

	/** Device specific driver data. */
	void * driver_data;
};

extern void hcd_init(hcd_t *);

static inline void hcd_set_implementation(hcd_t *hcd, void *data,
    const hcd_ops_t *ops, bus_t *bus)
{
	assert(hcd);
	if (ops) {
		hcd->driver_data = data;
		hcd->ops = *ops;
		hcd->bus = bus;
	} else {
		memset(&hcd->ops, 0, sizeof(hcd->ops));
	}
}

static inline void * hcd_get_driver_data(hcd_t *hcd)
{
	assert(hcd);
	return hcd->driver_data;
}

extern int hcd_get_ep0_max_packet_size(uint16_t *, hcd_t *, device_t *);
extern void hcd_setup_device_tt(device_t *);

extern int hcd_send_batch(hcd_t *, device_t *, usb_target_t,
    usb_direction_t direction, char *, size_t, uint64_t,
    usbhc_iface_transfer_callback_t, void *, const char *);

extern ssize_t hcd_send_batch_sync(hcd_t *, device_t *, usb_target_t,
    usb_direction_t direction, char *, size_t, uint64_t,
    const char *);

#endif

/**
 * @}
 */
