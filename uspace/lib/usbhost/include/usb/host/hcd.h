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
#include <usbhc_iface.h>

#include <usb/host/usb_device_manager.h>
#include <usb/host/usb_endpoint_manager.h>
#include <usb/host/usb_transfer_batch.h>

typedef struct hcd hcd_t;

/** Generic host controller driver structure. */
struct hcd {
	/** Device manager storing handles and addresses. */
	usb_device_manager_t dev_manager;
	/** Endpoint manager. */
	usb_endpoint_manager_t ep_manager;

	/** Device specific driver data. */
	void *private_data;
	/** Transfer scheduling, implement in device driver. */
	int (*schedule)(hcd_t *, usb_transfer_batch_t *);
	/** Hook called upon registering new endpoint. */
	int (*ep_add_hook)(hcd_t *, endpoint_t *);
	/** Hook called upon removing of an endpoint. */
	void (*ep_remove_hook)(hcd_t *, endpoint_t *);
};

/** Initialize hcd_t structure.
 * Initializes device and endpoint managers. Sets data and hook pointer to NULL.
 * @param hcd hcd_t structure to initialize, non-null.
 * @param bandwidth Available bandwidth, passed to endpoint manager.
 * @param bw_count Bandwidth compute function, passed to endpoint manager.
 */
static inline void hcd_init(hcd_t *hcd, usb_speed_t max_speed, size_t bandwidth,
    size_t (*bw_count)(usb_speed_t, usb_transfer_type_t, size_t, size_t))
{
	assert(hcd);
	usb_device_manager_init(&hcd->dev_manager, max_speed);
	usb_endpoint_manager_init(&hcd->ep_manager, bandwidth, bw_count);
	hcd->private_data = NULL;
	hcd->schedule = NULL;
	hcd->ep_add_hook = NULL;
	hcd->ep_remove_hook = NULL;
}

/** Check registered endpoints and reset toggle bit if necessary.
 * @param hcd hcd_t structure, non-null.
 * @param target Control communication target.
 * @param setup_data Setup packet of the control communication.
 */
static inline void reset_ep_if_need(hcd_t *hcd, usb_target_t target,
    const char setup_data[8])
{
	assert(hcd);
	usb_endpoint_manager_reset_eps_if_need(
	    &hcd->ep_manager, target, (const uint8_t *)setup_data);
}

/** Data retrieve wrapper.
 * @param fun ddf function, non-null.
 * @return pointer cast to hcd_t*.
 */
static inline hcd_t *fun_to_hcd(ddf_fun_t *fun)
{
	return ddf_fun_data_get(fun);
}

extern usbhc_iface_t hcd_iface;

#endif

/**
 * @}
 */
