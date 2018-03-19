/*
 * Copyright (c) 2013 Jan Vesely
 * Copyright (c) 2018 Ondrej Hlavaty, Jan Hrach
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
/**  @addtogroup libusbhost
 * @{
 */
/** @file
 */

#include <macros.h>
#include <str_error.h>
#include <usb/debug.h>
#include <usb/descriptor.h>
#include <usb/request.h>

#include "ddf_helpers.h"
#include "utility.h"

/**
 * Get initial max packet size for the control endpoint 0.
 *
 * For LS, HS, and SS devices this value is final and fixed.
 * For FS devices, the default value of 8 is returned. The caller needs
 * to fetch the first 8B of the device descriptor later in the initialization
 * process and determine whether it should be increased.
 *
 * @return Max packet size for EP 0 (in bytes)
 */
uint16_t hc_get_ep0_initial_mps(usb_speed_t speed)
{
	static const uint16_t mps_fixed [] = {
		[USB_SPEED_LOW] = 8,
		[USB_SPEED_HIGH] = 64,
		[USB_SPEED_SUPER] = 512,
	};

	if (speed < ARRAY_SIZE(mps_fixed) && mps_fixed[speed] != 0) {
		return mps_fixed[speed];
	}
	return 8; // USB_SPEED_FULL default
}

/**
 * Get max packet size for the control endpoint 0.
 *
 * For LS, HS, and SS devices the corresponding fixed value is obtained.
 * For FS devices the first 8B of the device descriptor are fetched to
 * determine it.
 *
 * @return Max packet size for EP 0 (in bytes)
 */
int hc_get_ep0_max_packet_size(uint16_t *mps, device_t *dev)
{
	assert(mps);

	*mps = hc_get_ep0_initial_mps(dev->speed);
	if (dev->speed != USB_SPEED_FULL) {
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
	size_t got;
	const errno_t err = bus_device_send_batch_sync(dev, control_ep,
	    USB_DIRECTION_IN, (char *) &desc, CTRL_PIPE_MIN_PACKET_SIZE,
	    *(uint64_t *)&get_device_desc_8,
	    "read first 8 bytes of dev descriptor", &got);

	if (got != CTRL_PIPE_MIN_PACKET_SIZE) {
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

int hc_get_device_desc(device_t *device, usb_standard_device_descriptor_t *desc)
{
	const usb_target_t control_ep = {{
		.address = device->address,
		.endpoint = 0,
	}};

	/* Get std device descriptor */
	const usb_device_request_setup_packet_t get_device_desc =
	    GET_DEVICE_DESC(sizeof(*desc));

	usb_log_debug("Device(%d): Requesting full device descriptor.",
	    device->address);
	size_t got;
	errno_t err = bus_device_send_batch_sync(device, control_ep,
	    USB_DIRECTION_IN, (char *) desc, sizeof(*desc),
	    *(uint64_t *)&get_device_desc, "read device descriptor", &got);

	if (!err && got != sizeof(*desc))
		err = EOVERFLOW;

	return err;
}

int hc_get_hub_desc(device_t *device, usb_hub_descriptor_header_t *desc)
{
	const usb_target_t control_ep = {{
		.address = device->address,
		.endpoint = 0,
	}};

	const usb_descriptor_type_t type = device->speed >= USB_SPEED_SUPER
		? USB_DESCTYPE_SSPEED_HUB : USB_DESCTYPE_HUB;

	const usb_device_request_setup_packet_t get_hub_desc = {
		.request_type = SETUP_REQUEST_TYPE_DEVICE_TO_HOST
		    | (USB_REQUEST_TYPE_CLASS << 5)
		    | USB_REQUEST_RECIPIENT_DEVICE,
		.request = USB_DEVREQ_GET_DESCRIPTOR,
		.value = uint16_host2usb(type << 8),
		.length = sizeof(*desc),
	};

	usb_log_debug("Device(%d): Requesting hub descriptor.",
	    device->address);

	size_t got;
	errno_t err = bus_device_send_batch_sync(device, control_ep,
	    USB_DIRECTION_IN, (char *) desc, sizeof(*desc),
	    *(uint64_t *)&get_hub_desc, "get hub descriptor", &got);

	if (!err && got != sizeof(*desc))
		err = EOVERFLOW;

	return err;
}

int hc_device_explore(device_t *device)
{
	int err;
	usb_standard_device_descriptor_t desc = { 0 };

	if ((err = hc_get_device_desc(device, &desc))) {
		usb_log_error("Device(%d): Failed to get dev descriptor: %s",
		    device->address, str_error(err));
		return err;
	}

	if ((err = hcd_ddf_setup_match_ids(device, &desc))) {
		usb_log_error("Device(%d): Failed to setup match ids: %s", device->address, str_error(err));
		return err;
	}

	return EOK;
}

/** Announce root hub to the DDF
 *
 * @param[in] device Host controller ddf device
 * @return Error code
 */
int hc_setup_virtual_root_hub(hc_device_t *hcd, usb_speed_t rh_speed)
{
	int err;

	assert(hcd);

	device_t *dev = hcd_ddf_fun_create(hcd, rh_speed);
	if (!dev) {
		usb_log_error("Failed to create function for the root hub.");
		return ENOMEM;
	}

	ddf_fun_set_name(dev->fun, "roothub");

	/* Assign an address to the device */
	if ((err = bus_device_enumerate(dev))) {
		usb_log_error("Failed to enumerate roothub device: %s", str_error(err));
		goto err_usb_dev;
	}

	if ((err = ddf_fun_bind(dev->fun))) {
		usb_log_error("Failed to register roothub: %s.", str_error(err));
		goto err_enumerated;
	}

	return EOK;

err_enumerated:
	bus_device_gone(dev);
err_usb_dev:
	hcd_ddf_fun_destroy(dev);
	return err;
}

/**
 * Check setup packet data for signs of toggle reset.
 *
 * @param[in] batch USB batch
 * @param[in] reset_cb Callback to reset an endpoint
 */
void hc_reset_toggles(const usb_transfer_batch_t *batch, endpoint_reset_toggle_t reset_cb)
{
	if (batch->ep->transfer_type != USB_TRANSFER_CONTROL
	    || batch->dir != USB_DIRECTION_OUT)
		return;

	const usb_device_request_setup_packet_t *request = &batch->setup.packet;
	device_t * const dev = batch->ep->device;

	switch (request->request)
	{
	/* Clear Feature ENPOINT_STALL */
	case USB_DEVREQ_CLEAR_FEATURE: /*resets only cleared ep */
		/* 0x2 ( HOST to device | STANDART | TO ENPOINT) */
		if ((request->request_type == 0x2) &&
		    (request->value == USB_FEATURE_ENDPOINT_HALT)) {
			const uint16_t index = uint16_usb2host(request->index);
			const usb_endpoint_t ep_num = index & 0xf;
			const usb_direction_t dir = (index >> 7) ? USB_DIRECTION_IN : USB_DIRECTION_OUT;

			endpoint_t *ep = bus_find_endpoint(dev, ep_num, dir);
			if (ep) {
				reset_cb(ep);
				endpoint_del_ref(ep);
			} else {
				usb_log_warning("Device(%u): Resetting unregistered endpoint %u %s.", dev->address, ep_num, usb_str_direction(dir));
			}
		}
		break;
	case USB_DEVREQ_SET_CONFIGURATION:
	case USB_DEVREQ_SET_INTERFACE:
		/* Recipient must be device, this resets all endpoints,
		 * In fact there should be no endpoints but EP 0 registered
		 * as different interfaces use different endpoints,
		 * unless you're changing configuration or alternative
		 * interface of an already setup device. */
		if (!(request->request_type & SETUP_REQUEST_TYPE_DEVICE_TO_HOST))
			for (usb_endpoint_t i = 0; i < 2 * USB_ENDPOINT_MAX; ++i)
				if (dev->endpoints[i])
					reset_cb(dev->endpoints[i]);
		break;
	default:
		break;
	}
}

typedef struct joinable_fibril {
	fid_t fid;
	void *arg;
	fibril_worker_t worker;

	bool running;
	fibril_mutex_t guard;
	fibril_condvar_t dead_cv;
} joinable_fibril_t;

static int joinable_fibril_worker(void *arg)
{
	joinable_fibril_t *jf = arg;

	jf->worker(jf->arg);

	fibril_mutex_lock(&jf->guard);
	jf->running = false;
	fibril_mutex_unlock(&jf->guard);
	fibril_condvar_broadcast(&jf->dead_cv);
	return 0;
}

/**
 * Create a fibril that is joinable. Similar to fibril_create.
 */
joinable_fibril_t *joinable_fibril_create(fibril_worker_t worker, void *arg)
{
	joinable_fibril_t *jf = calloc(1, sizeof(joinable_fibril_t));
	if (!jf)
		return NULL;

	jf->worker = worker;
	jf->arg = arg;
	fibril_mutex_initialize(&jf->guard);
	fibril_condvar_initialize(&jf->dead_cv);

	if (joinable_fibril_recreate(jf)) {
		free(jf);
		return NULL;
	}

	return jf;
}

/**
 * Start a joinable fibril. Similar to fibril_add_ready.
 */
void joinable_fibril_start(joinable_fibril_t *jf)
{
	assert(jf);
	assert(!jf->running);

	jf->running = true;
	fibril_add_ready(jf->fid);
}

/**
 * Join a joinable fibril. Not similar to anything, obviously.
 */
void joinable_fibril_join(joinable_fibril_t *jf)
{
	assert(jf);

	fibril_mutex_lock(&jf->guard);
	while (jf->running)
		fibril_condvar_wait(&jf->dead_cv, &jf->guard);
	fibril_mutex_unlock(&jf->guard);

	jf->fid = 0;
}

/**
 * Reinitialize a joinable fibril.
 */
errno_t joinable_fibril_recreate(joinable_fibril_t *jf)
{
	assert(!jf->fid);

	jf->fid = fibril_create(joinable_fibril_worker, jf);
	return jf->fid ? EOK : ENOMEM;
}

/**
 * Regular fibrils clean after themselves, joinable fibrils cannot.
 */
void joinable_fibril_destroy(joinable_fibril_t *jf)
{
	if (jf) {
		joinable_fibril_join(jf);
		free(jf);
	}
}

/**
 * @}
 */
