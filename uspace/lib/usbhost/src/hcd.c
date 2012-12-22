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

#include <errno.h>
#include <str_error.h>
#include <usb_iface.h>
#include <usb/debug.h>
#include <usb/request.h>

#include <usb/host/hcd.h>

/** Initialize hcd_t structure.
 * Initializes device and endpoint managers. Sets data and hook pointer to NULL.
 *
 * @param hcd hcd_t structure to initialize, non-null.
 * @param max_speed Maximum supported USB speed (full, high).
 * @param bandwidth Available bandwidth, passed to endpoint manager.
 * @param bw_count Bandwidth compute function, passed to endpoint manager.
 */
void hcd_init(hcd_t *hcd, usb_speed_t max_speed, size_t bandwidth,
    bw_count_func_t bw_count)
{
	assert(hcd);
	usb_device_manager_init(&hcd->dev_manager, max_speed);
	usb_endpoint_manager_init(&hcd->ep_manager, bandwidth, bw_count);

	hcd->private_data = NULL;
	hcd->schedule = NULL;
	hcd->ep_add_hook = NULL;
	hcd->ep_remove_hook = NULL;
}

typedef struct {
	void *original_data;
	usbhc_iface_transfer_out_callback_t original_callback;
	usb_target_t target;
	hcd_t *hcd;
} toggle_t;

static void toggle_reset_callback(int retval, void *arg)
{
	assert(arg);
	toggle_t *toggle = arg;
	if (retval == EOK) {
		usb_log_debug2("Reseting toggle on %d:%d.\n",
		    toggle->target.address, toggle->target.endpoint);
		usb_endpoint_manager_reset_toggle(&toggle->hcd->ep_manager,
		    toggle->target, toggle->target.endpoint == 0);
	}

	toggle->original_callback(retval, toggle->original_data);
}

/** Prepare generic usb_transfer_batch and schedule it.
 * @param hcd Host controller driver.
 * @param fun DDF fun
 * @param target address and endpoint number.
 * @param setup_data Data to use in setup stage (Control communication type)
 * @param in Callback for device to host communication.
 * @param out Callback for host to device communication.
 * @param arg Callback parameter.
 * @param name Communication identifier (for nicer output).
 * @return Error code.
 */
int hcd_send_batch(
    hcd_t *hcd, usb_target_t target, usb_direction_t direction,
    void *data, size_t size, uint64_t setup_data,
    usbhc_iface_transfer_in_callback_t in,
    usbhc_iface_transfer_out_callback_t out, void *arg, const char* name)
{
	assert(hcd);

	endpoint_t *ep = usb_endpoint_manager_find_ep(&hcd->ep_manager,
	    target.address, target.endpoint, direction);
	if (ep == NULL) {
		usb_log_error("Endpoint(%d:%d) not registered for %s.\n",
		    target.address, target.endpoint, name);
		return ENOENT;
	}

	usb_log_debug2("%s %d:%d %zu(%zu).\n",
	    name, target.address, target.endpoint, size, ep->max_packet_size);

	const size_t bw = bandwidth_count_usb11(
	    ep->speed, ep->transfer_type, size, ep->max_packet_size);
	/* Check if we have enough bandwidth reserved */
	if (ep->bandwidth < bw) {
		usb_log_error("Endpoint(%d:%d) %s needs %zu bw "
		    "but only %zu is reserved.\n",
		    ep->address, ep->endpoint, name, bw, ep->bandwidth);
		return ENOSPC;
	}
	if (!hcd->schedule) {
		usb_log_error("HCD does not implement scheduler.\n");
		return ENOTSUP;
	}

	/* Check for commands that reset toggle bit */
	if (ep->transfer_type == USB_TRANSFER_CONTROL) {
		const int reset_toggle = usb_request_needs_toggle_reset(
		    (usb_device_request_setup_packet_t *) &setup_data);
		if (reset_toggle >= 0) {
			assert(out);
			toggle_t *toggle = malloc(sizeof(toggle_t));
			if (!toggle)
				return ENOMEM;
			toggle->target.address = target.address;
			toggle->target.endpoint = reset_toggle;
			toggle->original_callback = out;
			toggle->original_data = arg;
			toggle->hcd = hcd;

			arg = toggle;
			out = toggle_reset_callback;
		}
	}

	usb_transfer_batch_t *batch = usb_transfer_batch_create(
	    ep, data, size, setup_data, in, out, arg);
	if (!batch) {
		return ENOMEM;
	}

	const int ret = hcd->schedule(hcd, batch);
	if (ret != EOK)
		usb_transfer_batch_destroy(batch);

	return ret;
}

/**
 * @}
 */
