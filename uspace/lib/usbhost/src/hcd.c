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

#include <assert.h>
#include <async.h>
#include <errno.h>
#include <macros.h>
#include <str_error.h>
#include <usb/debug.h>
#include <usb/descriptor.h>
#include <usb/request.h>
#include <usb_iface.h>

#include "bus.h"
#include "endpoint.h"
#include "usb_transfer_batch.h"

#include "hcd.h"


/** Initialize hcd_t structure.
 * Initializes device and endpoint managers. Sets data and hook pointer to NULL.
 *
 * @param hcd hcd_t structure to initialize, non-null.
 * @param max_speed Maximum supported USB speed (full, high).
 * @param bandwidth Available bandwidth, passed to endpoint manager.
 * @param bw_count Bandwidth compute function, passed to endpoint manager.
 */
void hcd_init(hcd_t *hcd) {
	assert(hcd);

	hcd_set_implementation(hcd, NULL, NULL, NULL);
}

/** Get max packet size for the control endpoint 0.
 *
 * For LS, HS, and SS devices this value is fixed. For FS devices we must fetch
 * the first 8B of the device descriptor to determine it.
 *
 * @return Max packet size for EP 0
 */
int hcd_get_ep0_max_packet_size(uint16_t *mps, hcd_t *hcd, device_t *dev)
{
	assert(mps);

	static const uint16_t mps_fixed [] = {
		[USB_SPEED_LOW] = 8,
		[USB_SPEED_HIGH] = 64,
		[USB_SPEED_SUPER] = 512,
	};

	if (dev->speed < ARRAY_SIZE(mps_fixed) && mps_fixed[dev->speed] != 0) {
		*mps = mps_fixed[dev->speed];
		return EOK;
	}

	const usb_target_t control_ep = {{
		.address = dev->address,
		.endpoint = 0,
	}};

	usb_standard_device_descriptor_t desc = { 0 };
	const usb_device_request_setup_packet_t get_device_desc_8 =
	    GET_DEVICE_DESC(CTRL_PIPE_MIN_PACKET_SIZE);

	usb_log_debug("Requesting first 8B of device descriptor to determine MPS.");
	ssize_t got = hcd_send_batch_sync(hcd, dev, control_ep, USB_DIRECTION_IN,
	    (char *) &desc, CTRL_PIPE_MIN_PACKET_SIZE, *(uint64_t *)&get_device_desc_8,
	    "read first 8 bytes of dev descriptor");

	if (got != CTRL_PIPE_MIN_PACKET_SIZE) {
		const int err = got < 0 ? got : EOVERFLOW;
		usb_log_error("Failed to get 8B of dev descr: %s.", str_error(err));
		return err;
	}

	if (desc.descriptor_type != USB_DESCTYPE_DEVICE) {
		usb_log_error("The device responded with wrong device descriptor.");
		return EIO;
	}

	uint16_t version = uint16_usb2host(desc.usb_spec_version);
	if (version < 0x0300) {
		/* USB 2 and below have MPS raw in the field */
		*mps = desc.max_packet_size;
	} else {
		/* USB 3 have MPS as an 2-based exponent */
		*mps = (1 << desc.max_packet_size);
	}
	return EOK;
}

/**
 * Setup devices Transaction Translation.
 *
 * This applies for Low/Full speed devices under High speed hub only. Other
 * devices just inherit TT from the hub.
 *
 * Roothub must be handled specially.
 */
void hcd_setup_device_tt(device_t *dev)
{
	if (!dev->hub)
		return;

	if (dev->hub->speed == USB_SPEED_HIGH && usb_speed_is_11(dev->speed)) {
		/* For LS devices under HS hub */
		dev->tt.address = dev->hub->address;
		dev->tt.port = dev->port;
	}
	else {
		/* Inherit hub's TT */
		dev->tt = dev->hub->tt;
	}
}

/** Check setup packet data for signs of toggle reset.
 *
 * @param[in] requst Setup requst data.
 *
 * @retval -1 No endpoints need reset.
 * @retval 0 All endpoints need reset.
 * @retval >0 Specified endpoint needs reset.
 *
 */
static toggle_reset_mode_t hcd_get_request_toggle_reset_mode(
    const usb_device_request_setup_packet_t *request)
{
	assert(request);
	switch (request->request)
	{
	/* Clear Feature ENPOINT_STALL */
	case USB_DEVREQ_CLEAR_FEATURE: /*resets only cleared ep */
		/* 0x2 ( HOST to device | STANDART | TO ENPOINT) */
		if ((request->request_type == 0x2) &&
		    (request->value == USB_FEATURE_ENDPOINT_HALT))
			return RESET_EP;
		break;
	case USB_DEVREQ_SET_CONFIGURATION:
	case USB_DEVREQ_SET_INTERFACE:
		/* Recipient must be device, this resets all endpoints,
		 * In fact there should be no endpoints but EP 0 registered
		 * as different interfaces use different endpoints,
		 * unless you're changing configuration or alternative
		 * interface of an already setup device. */
		if (!(request->request_type & SETUP_REQUEST_TYPE_DEVICE_TO_HOST))
			return RESET_ALL;
		break;
	default:
		break;
	}

	return RESET_NONE;
}

/** Prepare generic usb_transfer_batch and schedule it.
 * @param hcd Host controller driver.
 * @param target address and endpoint number.
 * @param setup_data Data to use in setup stage (Control communication type)
 * @param in Callback for device to host communication.
 * @param out Callback for host to device communication.
 * @param arg Callback parameter.
 * @param name Communication identifier (for nicer output).
 * @return Error code.
 */
int hcd_send_batch(hcd_t *hcd, device_t *device, usb_target_t target,
    usb_direction_t direction, char *data, size_t size, uint64_t setup_data,
    usbhc_iface_transfer_callback_t on_complete, void *arg, const char *name)
{
	assert(hcd);
	assert(device->address == target.address);

	if (!hcd->ops.schedule) {
		usb_log_error("HCD does not implement scheduler.\n");
		return ENOTSUP;
	}

	endpoint_t *ep = bus_find_endpoint(device, target, direction);
	if (ep == NULL) {
		usb_log_error("Endpoint(%d:%d) not registered for %s.\n",
		    device->address, target.endpoint, name);
		return ENOENT;
	}

	// TODO cut here aka provide helper to call with instance of endpoint_t in hand
	assert(ep->device == device);

	usb_log_debug2("%s %d:%d %zu(%zu).\n",
	    name, target.address, target.endpoint, size, ep->max_packet_size);

	const size_t bw = endpoint_count_bw(ep, size);
	/* Check if we have enough bandwidth reserved */
	if (ep->bandwidth < bw) {
		usb_log_error("Endpoint(%d:%d) %s needs %zu bw "
		    "but only %zu is reserved.\n",
		    device->address, ep->endpoint, name, bw, ep->bandwidth);
		return ENOSPC;
	}

	usb_transfer_batch_t *batch = usb_transfer_batch_create(ep);
	if (!batch) {
		usb_log_error("Failed to create transfer batch.\n");
		return ENOMEM;
	}

	batch->target = target;
	batch->buffer = data;
	batch->buffer_size = size;
	batch->setup.packed = setup_data;
	batch->dir = direction;
	batch->on_complete = on_complete;
	batch->on_complete_data = arg;

	/* Check for commands that reset toggle bit */
	if (ep->transfer_type == USB_TRANSFER_CONTROL)
		batch->toggle_reset_mode
			= hcd_get_request_toggle_reset_mode(&batch->setup.packet);

	const int ret = hcd->ops.schedule(hcd, batch);
	if (ret != EOK) {
		usb_log_warning("Batch %p failed to schedule: %s", batch, str_error(ret));
		usb_transfer_batch_destroy(batch);
	}

	/* Drop our own reference to ep. */
	endpoint_del_ref(ep);

	return ret;
}

typedef struct {
	fibril_mutex_t done_mtx;
	fibril_condvar_t done_cv;
	unsigned done;

	size_t transfered_size;
	int error;
} sync_data_t;

static int sync_transfer_complete(void *arg, int error, size_t transfered_size)
{
	sync_data_t *d = arg;
	assert(d);
	d->transfered_size = transfered_size;
	d->error = error;
	fibril_mutex_lock(&d->done_mtx);
	d->done = 1;
	fibril_condvar_broadcast(&d->done_cv);
	fibril_mutex_unlock(&d->done_mtx);
	return EOK;
}

ssize_t hcd_send_batch_sync(hcd_t *hcd, device_t *device, usb_target_t target,
    usb_direction_t direction, char *data, size_t size, uint64_t setup_data,
    const char *name)
{
	assert(hcd);
	sync_data_t sd = { .done = 0 };
	fibril_mutex_initialize(&sd.done_mtx);
	fibril_condvar_initialize(&sd.done_cv);

	const int ret = hcd_send_batch(hcd, device, target, direction,
	    data, size, setup_data,
	    sync_transfer_complete, &sd, name);
	if (ret != EOK)
		return ret;

	fibril_mutex_lock(&sd.done_mtx);
	while (!sd.done)
		fibril_condvar_wait(&sd.done_cv, &sd.done_mtx);
	fibril_mutex_unlock(&sd.done_mtx);

	return (sd.error == EOK)
		? (ssize_t) sd.transfered_size
		: (ssize_t) sd.error;
}


/**
 * @}
 */
