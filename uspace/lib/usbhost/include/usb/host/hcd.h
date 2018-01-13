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
#include <stddef.h>
#include <stdint.h>

typedef struct hcd hcd_t;

typedef errno_t (*schedule_hook_t)(hcd_t *, usb_transfer_batch_t *);
typedef errno_t (*ep_add_hook_t)(hcd_t *, endpoint_t *);
typedef void (*ep_remove_hook_t)(hcd_t *, endpoint_t *);
typedef void (*interrupt_hook_t)(hcd_t *, uint32_t);
typedef errno_t (*status_hook_t)(hcd_t *, uint32_t *);

typedef struct {
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
} hcd_ops_t;

/** Generic host controller driver structure. */
struct hcd {
	/** Endpoint manager. */
	usb_bus_t bus;

	/** Interrupt replacement fibril */
	fid_t polling_fibril;

	/** Driver implementation */
	hcd_ops_t ops;
	/** Device specific driver data. */
	void * driver_data;
};

extern void hcd_init(hcd_t *, usb_speed_t, size_t, bw_count_func_t);

static inline void hcd_set_implementation(hcd_t *hcd, void *data,
    const hcd_ops_t *ops)
{
	assert(hcd);
	if (ops) {
		hcd->driver_data = data;
		hcd->ops = *ops;
	} else {
		memset(&hcd->ops, 0, sizeof(hcd->ops));
	}
}

static inline void * hcd_get_driver_data(hcd_t *hcd)
{
	assert(hcd);
	return hcd->driver_data;
}

extern errno_t hcd_request_address(hcd_t *, usb_speed_t, usb_address_t *);

extern errno_t hcd_release_address(hcd_t *, usb_address_t);

extern errno_t hcd_reserve_default_address(hcd_t *, usb_speed_t);

static inline errno_t hcd_release_default_address(hcd_t *hcd)
{
	return hcd_release_address(hcd, USB_ADDRESS_DEFAULT);
}

extern errno_t hcd_add_ep(hcd_t *, usb_target_t, usb_direction_t,
    usb_transfer_type_t, size_t, unsigned int, size_t, usb_address_t,
    unsigned int);

extern errno_t hcd_remove_ep(hcd_t *, usb_target_t, usb_direction_t);

extern errno_t hcd_send_batch(hcd_t *, usb_target_t, usb_direction_t, void *,
    size_t, uint64_t, usbhc_iface_transfer_in_callback_t,
    usbhc_iface_transfer_out_callback_t, void *, const char *);

extern errno_t hcd_send_batch_sync(hcd_t *, usb_target_t, usb_direction_t,
    void *, size_t, uint64_t, const char *, size_t *);

#endif

/**
 * @}
 */
