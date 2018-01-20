/*
 * Copyright (c) 2013 Jan Vesely
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
 * Helpers to work with the DDF interface.
 */

#include <adt/list.h>
#include <assert.h>
#include <async.h>
#include <ddf/driver.h>
#include <ddf/interrupt.h>
#include <device/hw_res_parsed.h>
#include <errno.h>
#include <str_error.h>
#include <usb/classes/classes.h>
#include <usb/debug.h>
#include <usb/descriptor.h>
#include <usb/usb.h>
#include <usb_iface.h>
#include <usbhc_iface.h>

#include "bus.h"
#include "endpoint.h"

#include "ddf_helpers.h"

/**
 * DDF usbhc_iface callback. Passes the endpoint descriptors, fills the pipe
 * descriptor according to the contents of the endpoint.
 *
 * @param[in] fun DDF function of the device in question.
 * @param[out] pipe_desc The pipe descriptor to be filled.
 * @param[in] endpoint_desc Endpoint descriptors from the device.
 * @return Error code.
 */
static int register_endpoint(ddf_fun_t *fun, usb_pipe_desc_t *pipe_desc,
     const usb_endpoint_descriptors_t *ep_desc)
{
	assert(fun);
	hc_device_t *hcd = dev_to_hcd(ddf_fun_get_dev(fun));
	device_t *dev = ddf_fun_data_get(fun);
	assert(hcd);
	assert(hcd->bus);
	assert(dev);

	endpoint_t *ep;
	const int err = bus_endpoint_add(dev, ep_desc, &ep);
	if (err)
		return err;

	if (pipe_desc) {
		pipe_desc->endpoint_no = ep->endpoint;
		pipe_desc->direction = ep->direction;
		pipe_desc->transfer_type = ep->transfer_type;
		pipe_desc->max_transfer_size = ep->max_transfer_size;
	}
	endpoint_del_ref(ep);

	return EOK;
}

 /**
  * DDF usbhc_iface callback. Unregister endpoint that makes the other end of
  * the pipe described.
  *
  * @param fun DDF function of the device in question.
  * @param pipe_desc Pipe description.
  * @return Error code.
  */
static int unregister_endpoint(ddf_fun_t *fun, const usb_pipe_desc_t *pipe_desc)
{
	assert(fun);
	hc_device_t *hcd = dev_to_hcd(ddf_fun_get_dev(fun));
	device_t *dev = ddf_fun_data_get(fun);
	assert(hcd);
	assert(hcd->bus);
	assert(dev);

	endpoint_t *ep = bus_find_endpoint(dev, pipe_desc->endpoint_no, pipe_desc->direction);
	if (!ep)
		return ENOENT;

	return bus_endpoint_remove(ep);
}

/**
 * DDF usbhc_iface callback. Calls the respective bus operation directly.
 *
 * @param fun DDF function of the device (hub) requesting the address.
 */
static int default_address_reservation(ddf_fun_t *fun, bool reserve)
{
	assert(fun);
	hc_device_t *hcd = dev_to_hcd(ddf_fun_get_dev(fun));
	device_t *dev = ddf_fun_data_get(fun);
	assert(hcd);
	assert(hcd->bus);
	assert(dev);

	usb_log_debug("Device %d %s default address", dev->address, reserve ? "requested" : "releasing");
	if (reserve) {
		return bus_reserve_default_address(hcd->bus, dev);
	} else {
		bus_release_default_address(hcd->bus, dev);
		return EOK;
	}
}

/**
 * DDF usbhc_iface callback. Calls the bus operation directly.
 *
 * @param fun DDF function of the device (hub) requesting the address.
 * @param speed USB speed of the new device
 */
static int device_enumerate(ddf_fun_t *fun, unsigned port, usb_speed_t speed)
{
	assert(fun);
	ddf_dev_t *hc = ddf_fun_get_dev(fun);
	assert(hc);
	hc_device_t *hcd = dev_to_hcd(hc);
	assert(hcd);
	device_t *hub = ddf_fun_data_get(fun);
	assert(hub);

	int err;

	usb_log_debug("Hub %d reported a new %s speed device on port: %u",
	    hub->address, usb_str_speed(speed), port);

	device_t *dev = hcd_ddf_fun_create(hcd, speed);
	if (!dev) {
		usb_log_error("Failed to create USB device function.");
		return ENOMEM;
	}

	dev->hub = hub;
	dev->port = port;
	dev->speed = speed;

	if ((err = bus_device_enumerate(dev))) {
		usb_log_error("Failed to initialize USB dev memory structures.");
		goto err_usb_dev;
	}

	/* If the driver didn't name the dev when enumerating,
	 * do it in some generic way.
	 */
	if (!ddf_fun_get_name(dev->fun)) {
		bus_device_set_default_name(dev);
	}

	if ((err = ddf_fun_bind(dev->fun))) {
		usb_log_error("Device(%d): Failed to register: %s.", dev->address, str_error(err));
		goto err_usb_dev;
	}

	return EOK;

err_usb_dev:
	hcd_ddf_fun_destroy(dev);
	return err;
}

static int device_remove(ddf_fun_t *fun, unsigned port)
{
	assert(fun);
	device_t *hub = ddf_fun_data_get(fun);
	assert(hub);
	usb_log_debug("Hub `%s' reported removal of device on port %u",
	    ddf_fun_get_name(fun), port);

	device_t *victim = NULL;

	fibril_mutex_lock(&hub->guard);
	list_foreach(hub->devices, link, device_t, it) {
		if (it->port == port) {
			victim = it;
			break;
		}
	}
	fibril_mutex_unlock(&hub->guard);

	if (!victim) {
		usb_log_warning("Hub '%s' tried to remove non-existant"
		    " device.", ddf_fun_get_name(fun));
		return ENOENT;
	}

	assert(victim->fun);
	assert(victim->port == port);
	assert(victim->hub == hub);

	bus_device_gone(victim);
	return EOK;
}

/**
 * Gets description of the device that is calling.
 *
 * @param[in] fun Device function.
 * @param[out] desc Device descriptor to be filled.
 * @return Error code.
 */
static int get_device_description(ddf_fun_t *fun, usb_device_desc_t *desc)
{
	assert(fun);
	device_t *dev = ddf_fun_data_get(fun);
	assert(dev);

	if (!desc)
		return EOK;

	*desc = (usb_device_desc_t) {
		.address = dev->address,
		.speed = dev->speed,
		.handle = ddf_fun_get_handle(fun),
		.iface = -1,
	};
	return EOK;
}

/** Inbound communication interface function.
 * @param fun DDF function.
 * @param target Communication target.
 * @param setup_data Data to use in setup stage (control transfers).
 * @param data Pointer to data buffer.
 * @param size Size of the data buffer.
 * @param callback Function to call on communication end.
 * @param arg Argument passed to the callback function.
 * @return Error code.
 */
static int dev_read(ddf_fun_t *fun, usb_target_t target,
    uint64_t setup_data, char *data, size_t size,
    usbhc_iface_transfer_callback_t callback, void *arg)
{
	assert(fun);
	device_t *dev = ddf_fun_data_get(fun);
	assert(dev);

	target.address = dev->address;

	return bus_device_send_batch(dev, target, USB_DIRECTION_IN,
	    data, size, setup_data,
	    callback, arg, "READ");
}

/** Outbound communication interface function.
 * @param fun DDF function.
 * @param target Communication target.
 * @param setup_data Data to use in setup stage (control transfers).
 * @param data Pointer to data buffer.
 * @param size Size of the data buffer.
 * @param callback Function to call on communication end.
 * @param arg Argument passed to the callback function.
 * @return Error code.
 */
static int dev_write(ddf_fun_t *fun, usb_target_t target,
    uint64_t setup_data, const char *data, size_t size,
    usbhc_iface_transfer_callback_t callback, void *arg)
{
	assert(fun);
	device_t *dev = ddf_fun_data_get(fun);
	assert(dev);

	target.address = dev->address;

	return bus_device_send_batch(dev, target, USB_DIRECTION_OUT,
	    (char *) data, size, setup_data,
	    callback, arg, "WRITE");
}

/** USB device interface */
static usb_iface_t usb_iface = {
	.get_my_description = get_device_description,
};

/** USB host controller interface */
static usbhc_iface_t usbhc_iface = {
	.default_address_reservation = default_address_reservation,

	.device_enumerate = device_enumerate,
	.device_remove = device_remove,

	.register_endpoint = register_endpoint,
	.unregister_endpoint = unregister_endpoint,

	.read = dev_read,
	.write = dev_write,
};

/** Standard USB device interface) */
static ddf_dev_ops_t usb_ops = {
	.interfaces[USB_DEV_IFACE] = &usb_iface,
	.interfaces[USBHC_DEV_IFACE] = &usbhc_iface,
};


/* DDF HELPERS */

#define ADD_MATCHID_OR_RETURN(list, sc, str, ...) \
do { \
	match_id_t *mid = malloc(sizeof(match_id_t)); \
	if (!mid) { \
		clean_match_ids(list); \
		return ENOMEM; \
	} \
	char *id = NULL; \
	int ret = asprintf(&id, str, ##__VA_ARGS__); \
	if (ret < 0) { \
		clean_match_ids(list); \
		free(mid); \
		return ENOMEM; \
	} \
	mid->score = sc; \
	mid->id = id; \
	add_match_id(list, mid); \
} while (0)

/* This is a copy of lib/usbdev/src/recognise.c */
static int create_match_ids(match_id_list_t *l,
    usb_standard_device_descriptor_t *d)
{
	assert(l);
	assert(d);

	if (d->vendor_id != 0) {
		/* First, with release number. */
		ADD_MATCHID_OR_RETURN(l, 100,
		    "usb&vendor=%#04x&product=%#04x&release=%x.%x",
		    d->vendor_id, d->product_id, (d->device_version >> 8),
		    (d->device_version & 0xff));

		/* Next, without release number. */
		ADD_MATCHID_OR_RETURN(l, 90, "usb&vendor=%#04x&product=%#04x",
		    d->vendor_id, d->product_id);
	}

	/* Class match id */
	ADD_MATCHID_OR_RETURN(l, 50, "usb&class=%s",
	    usb_str_class(d->device_class));

	/* As a last resort, try fallback driver. */
	ADD_MATCHID_OR_RETURN(l, 10, "usb&fallback");

	return EOK;
}

device_t *hcd_ddf_fun_create(hc_device_t *hc, usb_speed_t speed)
{
	/* Create DDF function for the new device */
	ddf_fun_t *fun = ddf_fun_create(hc->ddf_dev, fun_inner, NULL);
	if (!fun)
		return NULL;

	ddf_fun_set_ops(fun, &usb_ops);

	/* Create USB device node for the new device */
	device_t *dev = ddf_fun_data_alloc(fun, hc->bus->device_size);
	if (!dev) {
		ddf_fun_destroy(fun);
		return NULL;
	}

	bus_device_init(dev, hc->bus);
	dev->fun = fun;
	dev->speed = speed;
	return dev;
}

void hcd_ddf_fun_destroy(device_t *dev)
{
	assert(dev);
	assert(dev->fun);
	ddf_fun_destroy(dev->fun);
}

int hcd_ddf_setup_match_ids(device_t *device, usb_standard_device_descriptor_t *desc)
{
	int err;
	match_id_list_t mids;

	init_match_ids(&mids);

	/* Create match ids from the device descriptor */
	usb_log_debug("Device(%d): Creating match IDs.", device->address);
	if ((err = create_match_ids(&mids, desc))) {
		return err;
	}

	list_foreach(mids.ids, link, const match_id_t, mid) {
		ddf_fun_add_match_id(device->fun, mid->id, mid->score);
	}

	return EOK;
}

/** Initialize hc structures.
 *
 * @param[in] device DDF instance of the device to use.
 * @param[in] max_speed Maximum supported USB speed.
 * @param[in] bw available bandwidth.
 * @param[in] bw_count Function to compute required ep bandwidth.
 *
 * @return Error code.
 * This function does all the ddf work for hc driver.
 */
int hcd_ddf_setup_hc(ddf_dev_t *device, size_t size)
{
	assert(device);

	hc_device_t *instance = ddf_dev_data_alloc(device, size);
	if (instance == NULL) {
		usb_log_error("Failed to allocate HCD ddf structure.");
		return ENOMEM;
	}
	instance->ddf_dev = device;

	int ret = ENOMEM;
	instance->ctl_fun = ddf_fun_create(device, fun_exposed, "ctl");
	if (!instance->ctl_fun) {
		usb_log_error("Failed to create HCD ddf fun.");
		goto err_destroy_fun;
	}

	ret = ddf_fun_bind(instance->ctl_fun);
	if (ret != EOK) {
		usb_log_error("Failed to bind ctl_fun: %s.", str_error(ret));
		goto err_destroy_fun;
	}

	ret = ddf_fun_add_to_category(instance->ctl_fun, USB_HC_CATEGORY);
	if (ret != EOK) {
		usb_log_error("Failed to add fun to category: %s.",
		    str_error(ret));
		ddf_fun_unbind(instance->ctl_fun);
		goto err_destroy_fun;
	}

	/* HC should be ok at this point (except it can't do anything) */
	return EOK;

err_destroy_fun:
	ddf_fun_destroy(instance->ctl_fun);
	instance->ctl_fun = NULL;
	return ret;
}

void hcd_ddf_clean_hc(hc_device_t *hcd)
{
	if (ddf_fun_unbind(hcd->ctl_fun) == EOK)
		ddf_fun_destroy(hcd->ctl_fun);
}

/** Call the parent driver with a request to enable interrupt
 *
 * @param[in] device Device asking for interrupts
 * @param[in] inum Interrupt number
 * @return Error code.
 */
int hcd_ddf_enable_interrupt(hc_device_t *hcd, int inum)
{
	async_sess_t *parent_sess = ddf_dev_parent_sess_get(hcd->ddf_dev);
	if (parent_sess == NULL)
		return EIO;

	return hw_res_enable_interrupt(parent_sess, inum);
}

int hcd_ddf_get_registers(hc_device_t *hcd, hw_res_list_parsed_t *hw_res)
{
	async_sess_t *parent_sess = ddf_dev_parent_sess_get(hcd->ddf_dev);
	if (parent_sess == NULL)
		return EIO;

	hw_res_list_parsed_init(hw_res);
	const int ret = hw_res_get_list_parsed(parent_sess, hw_res, 0);
	if (ret != EOK)
		hw_res_list_parsed_clean(hw_res);
	return ret;
}

/**
 * @}
 */
