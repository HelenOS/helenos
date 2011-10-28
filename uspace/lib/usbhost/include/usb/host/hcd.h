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

struct hcd {
	usb_device_manager_t dev_manager;
	usb_endpoint_manager_t ep_manager;
	void *private_data;

	int (*schedule)(hcd_t *, usb_transfer_batch_t *);
	int (*ep_add_hook)(hcd_t *, endpoint_t *);
	void (*ep_remove_hook)(hcd_t *, endpoint_t *);
};
/*----------------------------------------------------------------------------*/
static inline void hcd_init(hcd_t *hcd, size_t bandwidth,
    size_t (*bw_count)(usb_speed_t, usb_transfer_type_t, size_t, size_t))
{
	assert(hcd);
	usb_device_manager_init(&hcd->dev_manager);
	usb_endpoint_manager_init(&hcd->ep_manager, bandwidth, bw_count);
}
/*----------------------------------------------------------------------------*/
static inline void reset_ep_if_need(
    hcd_t *hcd, usb_target_t target, const char* setup_data)
{
	assert(hcd);
	usb_endpoint_manager_reset_eps_if_need(
	    &hcd->ep_manager, target, (const uint8_t *)setup_data);
}
/*----------------------------------------------------------------------------*/
static inline hcd_t * fun_to_hcd(ddf_fun_t *fun)
{
	assert(fun);
	return fun->driver_data;
}
/*----------------------------------------------------------------------------*/
extern usbhc_iface_t hcd_iface;

#endif
/**
 * @}
 */
