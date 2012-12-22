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
#include <adt/list.h>
#include <usbhc_iface.h>

#include <usb/host/usb_device_manager.h>
#include <usb/host/usb_endpoint_manager.h>
#include <usb/host/usb_transfer_batch.h>

typedef struct hcd hcd_t;

typedef int (*schedule_hook_t)(hcd_t *, usb_transfer_batch_t *);
typedef int (*ep_add_hook_t)(hcd_t *, endpoint_t *);
typedef void (*ep_remove_hook_t)(hcd_t *, endpoint_t *);

/** Generic host controller driver structure. */
struct hcd {
	/** Device manager storing handles and addresses. */
	usb_device_manager_t dev_manager;
	/** Endpoint manager. */
	usb_endpoint_manager_t ep_manager;

	/** Device specific driver data. */
	void *private_data;
	/** Transfer scheduling, implement in device driver. */
	schedule_hook_t schedule;
	/** Hook called upon registering new endpoint. */
	ep_add_hook_t ep_add_hook;
	/** Hook called upon removing of an endpoint. */
	ep_remove_hook_t ep_remove_hook;
};

void hcd_init(hcd_t *hcd, usb_speed_t max_speed, size_t bandwidth,
    bw_count_func_t bw_count);

static inline void hcd_set_implementation(hcd_t *hcd, void *data,
    schedule_hook_t schedule, ep_add_hook_t add_hook, ep_remove_hook_t rem_hook)
{
	assert(hcd);
	hcd->private_data = data;
	hcd->schedule = schedule;
	hcd->ep_add_hook = add_hook;
	hcd->ep_remove_hook = rem_hook;

}

int hcd_send_batch(hcd_t *hcd, usb_target_t target, usb_direction_t direction,
    void *data, size_t size, uint64_t setup_data,
    usbhc_iface_transfer_in_callback_t in,
    usbhc_iface_transfer_out_callback_t out, void *arg, const char* name);

/** Data retrieve wrapper.
 * @param fun ddf function, non-null.
 * @return pointer cast to hcd_t*.
 */
static inline hcd_t *fun_to_hcd(ddf_fun_t *fun)
{
	return ddf_fun_data_get(fun);
}


#endif

/**
 * @}
 */
