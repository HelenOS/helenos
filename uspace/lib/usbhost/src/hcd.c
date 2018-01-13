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

#include <usb/debug.h>
#include <usb/request.h>

#include <assert.h>
#include <async.h>
#include <errno.h>
#include <usb_iface.h>

#include "hcd.h"

/** Calls ep_add_hook upon endpoint registration.
 * @param ep Endpoint to be registered.
 * @param arg hcd_t in disguise.
 * @return Error code.
 */
static errno_t register_helper(endpoint_t *ep, void *arg)
{
	hcd_t *hcd = arg;
	assert(ep);
	assert(hcd);
	if (hcd->ops.ep_add_hook)
		return hcd->ops.ep_add_hook(hcd, ep);
	return EOK;
}

/** Calls ep_remove_hook upon endpoint removal.
 * @param ep Endpoint to be unregistered.
 * @param arg hcd_t in disguise.
 */
static void unregister_helper(endpoint_t *ep, void *arg)
{
	hcd_t *hcd = arg;
	assert(ep);
	assert(hcd);
	if (hcd->ops.ep_remove_hook)
		hcd->ops.ep_remove_hook(hcd, ep);
}

/** Calls ep_remove_hook upon endpoint removal. Prints warning.
 *  * @param ep Endpoint to be unregistered.
 *   * @param arg hcd_t in disguise.
 *    */
static void unregister_helper_warn(endpoint_t *ep, void *arg)
{
        assert(ep);
        usb_log_warning("Endpoint %d:%d %s was left behind, removing.\n",
            ep->address, ep->endpoint, usb_str_direction(ep->direction));
	unregister_helper(ep, arg);
}


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
	usb_bus_init(&hcd->bus, bandwidth, bw_count, max_speed);

	hcd_set_implementation(hcd, NULL, NULL);
}

errno_t hcd_request_address(hcd_t *hcd, usb_speed_t speed, usb_address_t *address)
{
	assert(hcd);
	return usb_bus_request_address(&hcd->bus, address, false, speed);
}

errno_t hcd_release_address(hcd_t *hcd, usb_address_t address)
{
	assert(hcd);
	return usb_bus_remove_address(&hcd->bus, address,
	    unregister_helper_warn, hcd);
}

errno_t hcd_reserve_default_address(hcd_t *hcd, usb_speed_t speed)
{
	assert(hcd);
	usb_address_t address = 0;
	return usb_bus_request_address(&hcd->bus, &address, true, speed);
}

errno_t hcd_add_ep(hcd_t *hcd, usb_target_t target, usb_direction_t dir,
    usb_transfer_type_t type, size_t max_packet_size, unsigned packets,
    size_t size, usb_address_t tt_address, unsigned tt_port)
{
	assert(hcd);
	return usb_bus_add_ep(&hcd->bus, target.address,
	    target.endpoint, dir, type, max_packet_size, packets, size,
	    register_helper, hcd, tt_address, tt_port);
}

errno_t hcd_remove_ep(hcd_t *hcd, usb_target_t target, usb_direction_t dir)
{
	assert(hcd);
	return usb_bus_remove_ep(&hcd->bus, target.address,
	    target.endpoint, dir, unregister_helper, hcd);
}


typedef struct {
	void *original_data;
	usbhc_iface_transfer_out_callback_t original_callback;
	usb_target_t target;
	hcd_t *hcd;
} toggle_t;

static void toggle_reset_callback(errno_t retval, void *arg)
{
	assert(arg);
	toggle_t *toggle = arg;
	if (retval == EOK) {
		usb_log_debug2("Reseting toggle on %d:%d.\n",
		    toggle->target.address, toggle->target.endpoint);
		usb_bus_reset_toggle(&toggle->hcd->bus,
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
errno_t hcd_send_batch(
    hcd_t *hcd, usb_target_t target, usb_direction_t direction,
    void *data, size_t size, uint64_t setup_data,
    usbhc_iface_transfer_in_callback_t in,
    usbhc_iface_transfer_out_callback_t out, void *arg, const char* name)
{
	assert(hcd);

	endpoint_t *ep = usb_bus_find_ep(&hcd->bus,
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
	if (!hcd->ops.schedule) {
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
		usb_log_error("Failed to create transfer batch.\n");
		return ENOMEM;
	}

	const errno_t ret = hcd->ops.schedule(hcd, batch);
	if (ret != EOK)
		usb_transfer_batch_destroy(batch);

	/* Drop our own reference to ep. */
	endpoint_del_ref(ep);

	return ret;
}

typedef struct {
	volatile unsigned done;
	errno_t ret;
	size_t size;
} sync_data_t;

static void transfer_in_cb(errno_t ret, size_t size, void* data)
{
	sync_data_t *d = data;
	assert(d);
	d->ret = ret;
	d->done = 1;
	d->size = size;
}

static void transfer_out_cb(errno_t ret, void* data)
{
	sync_data_t *d = data;
	assert(data);
	d->ret = ret;
	d->done = 1;
}

/** this is really ugly version of sync usb communication */
errno_t hcd_send_batch_sync(
    hcd_t *hcd, usb_target_t target, usb_direction_t dir,
    void *data, size_t size, uint64_t setup_data, const char* name, size_t *out_size)
{
	assert(hcd);
	sync_data_t sd = { .done = 0, .ret = EBUSY, .size = size };

	const errno_t ret = hcd_send_batch(hcd, target, dir, data, size, setup_data,
	    dir == USB_DIRECTION_IN ? transfer_in_cb : NULL,
	    dir == USB_DIRECTION_OUT ? transfer_out_cb : NULL, &sd, name);
	if (ret != EOK)
		return ret;

	while (!sd.done) {
		async_usleep(1000);
	}

	if (sd.ret == EOK)
		*out_size = sd.size;
	return sd.ret;
}


/**
 * @}
 */
