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
 *
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

#include "ddf_helpers.h"


static int hcd_ddf_new_device(hc_device_t *hcd, ddf_dev_t *hc, device_t *hub_dev, unsigned port);
static int hcd_ddf_remove_device(ddf_dev_t *device, device_t *hub, unsigned port);


/* DDF INTERFACE */

/** Register endpoint interface function.
 * @param fun DDF function.
 * @param endpoint_desc Endpoint description.
 * @return Error code.
 */
static int register_endpoint(
	ddf_fun_t *fun, usb_endpoint_desc_t *endpoint_desc)
{
	assert(fun);
	hc_device_t *hcd = dev_to_hcd(ddf_fun_get_dev(fun));
	device_t *dev = ddf_fun_data_get(fun);
	assert(hcd);
	assert(hcd->bus);
	assert(dev);

	usb_log_debug("Register endpoint %d:%d %s-%s %zuB %ums.\n",
		dev->address, endpoint_desc->endpoint_no,
		usb_str_transfer_type(endpoint_desc->transfer_type),
		usb_str_direction(endpoint_desc->direction),
		endpoint_desc->max_packet_size, endpoint_desc->usb2.polling_interval);

	return bus_endpoint_add(dev, endpoint_desc, NULL);
}

 /** Unregister endpoint interface function.
  * @param fun DDF function.
  * @param endpoint_desc Endpoint description.
  * @return Error code.
  */
static int unregister_endpoint(
	ddf_fun_t *fun, usb_endpoint_desc_t *endpoint_desc)
{
	assert(fun);
	hc_device_t *hcd = dev_to_hcd(ddf_fun_get_dev(fun));
	device_t *dev = ddf_fun_data_get(fun);
	assert(hcd);
	assert(hcd->bus);
	assert(dev);

	const usb_target_t target = {{
		.address = dev->address,
		.endpoint = endpoint_desc->endpoint_no
	}};

	usb_log_debug("Unregister endpoint %d:%d %s.\n",
		dev->address, endpoint_desc->endpoint_no,
		usb_str_direction(endpoint_desc->direction));

	endpoint_t *ep = bus_find_endpoint(dev, target, endpoint_desc->direction);
	if (!ep)
		return ENOENT;

	return bus_endpoint_remove(ep);
}

static int reserve_default_address(ddf_fun_t *fun, usb_speed_t speed)
{
	assert(fun);
	hc_device_t *hcd = dev_to_hcd(ddf_fun_get_dev(fun));
	device_t *dev = ddf_fun_data_get(fun);
	assert(hcd);
	assert(hcd->bus);
	assert(dev);

	usb_log_debug("Device %d requested default address at %s speed\n",
	    dev->address, usb_str_speed(speed));
	return bus_reserve_default_address(hcd->bus, speed);
}

static int release_default_address(ddf_fun_t *fun)
{
	assert(fun);
	hc_device_t *hcd = dev_to_hcd(ddf_fun_get_dev(fun));
	device_t *dev = ddf_fun_data_get(fun);
	assert(hcd);
	assert(hcd->bus);
	assert(dev);

	usb_log_debug("Device %d released default address\n", dev->address);
	return bus_release_default_address(hcd->bus);
}

static int device_enumerate(ddf_fun_t *fun, unsigned port)
{
	assert(fun);
	ddf_dev_t *hc = ddf_fun_get_dev(fun);
	assert(hc);
	hc_device_t *hcd = dev_to_hcd(hc);
	assert(hcd);
	device_t *hub = ddf_fun_data_get(fun);
	assert(hub);

	usb_log_debug("Hub %d reported a new USB device on port: %u\n",
	    hub->address, port);
	return hcd_ddf_new_device(hcd, hc, hub, port);
}

static int device_remove(ddf_fun_t *fun, unsigned port)
{
	assert(fun);
	ddf_dev_t *ddf_dev = ddf_fun_get_dev(fun);
	device_t *dev = ddf_fun_data_get(fun);
	assert(ddf_dev);
	assert(dev);
	usb_log_debug("Hub `%s' reported removal of device on port %u\n",
	    ddf_fun_get_name(fun), port);
	return hcd_ddf_remove_device(ddf_dev, dev, port);
}

/** Gets handle of the respective device.
 *
 * @param[in] fun Device function.
 * @param[out] handle Place to write the handle.
 * @return Error code.
 */
static int get_my_device_handle(ddf_fun_t *fun, devman_handle_t *handle)
{
	assert(fun);
	if (handle)
		*handle = ddf_fun_get_handle(fun);
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
	.get_my_device_handle = get_my_device_handle,
};

/** USB host controller interface */
static usbhc_iface_t usbhc_iface = {
	.reserve_default_address = reserve_default_address,
	.release_default_address = release_default_address,

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

static int hcd_ddf_remove_device(ddf_dev_t *device, device_t *hub,
    unsigned port)
{
	assert(device);

	hc_device_t *hcd = dev_to_hcd(device);
	assert(hcd);
	assert(hcd->bus);

	fibril_mutex_lock(&hub->guard);

	device_t *victim = NULL;

	list_foreach(hub->devices, link, device_t, it) {
		if (it->port == port) {
			victim = it;
			break;
		}
	}
	if (victim) {
		assert(victim->fun);
		assert(victim->port == port);
		assert(victim->hub == hub);
		list_remove(&victim->link);
		fibril_mutex_unlock(&hub->guard);
		const int ret = ddf_fun_unbind(victim->fun);
		if (ret == EOK) {
			bus_device_remove(victim);
			ddf_fun_destroy(victim->fun);
		} else {
			usb_log_warning("Failed to unbind device `%s': %s\n",
			    ddf_fun_get_name(victim->fun), str_error(ret));
		}
		return EOK;
	}
	fibril_mutex_unlock(&hub->guard);
	return ENOENT;
}

device_t *hcd_ddf_fun_create(hc_device_t *hc)
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
	return dev;
}

void hcd_ddf_fun_destroy(device_t *dev)
{
	assert(dev);
	assert(dev->fun);
	ddf_fun_destroy(dev->fun);
}

int hcd_device_explore(device_t *device)
{
	int err;
	match_id_list_t mids;
	usb_standard_device_descriptor_t desc = { 0 };

	init_match_ids(&mids);

	const usb_target_t control_ep = {{
		.address = device->address,
		.endpoint = 0,
	}};

	/* Get std device descriptor */
	const usb_device_request_setup_packet_t get_device_desc =
	    GET_DEVICE_DESC(sizeof(desc));

	usb_log_debug("Device(%d): Requesting full device descriptor.",
	    device->address);
	ssize_t got = bus_device_send_batch_sync(device, control_ep, USB_DIRECTION_IN,
	    (char *) &desc, sizeof(desc), *(uint64_t *)&get_device_desc,
	    "read device descriptor");
	if (got < 0) {
		err = got < 0 ? got : EOVERFLOW;
		usb_log_error("Device(%d): Failed to set get dev descriptor: %s",
		    device->address, str_error(err));
		goto out;
	}

	/* Create match ids from the device descriptor */
	usb_log_debug("Device(%d): Creating match IDs.", device->address);
	if ((err = create_match_ids(&mids, &desc))) {
		usb_log_error("Device(%d): Failed to create match ids: %s", device->address, str_error(err));
		goto out;
	}

	list_foreach(mids.ids, link, const match_id_t, mid) {
		ddf_fun_add_match_id(device->fun, mid->id, mid->score);
	}

out:
	clean_match_ids(&mids);
	return err;
}

static int hcd_ddf_new_device(hc_device_t *hcd, ddf_dev_t *hc, device_t *hub, unsigned port)
{
	int err;
	assert(hcd);
	assert(hcd->bus);
	assert(hub);
	assert(hc);

	device_t *dev = hcd_ddf_fun_create(hcd);
	if (!dev) {
		usb_log_error("Failed to create USB device function.");
		return ENOMEM;
	}

	dev->hub = hub;
	dev->port = port;

	if ((err = bus_device_enumerate(dev))) {
		usb_log_error("Failed to initialize USB dev memory structures.");
		return err;
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

	fibril_mutex_lock(&hub->guard);
	list_append(&dev->link, &hub->devices);
	fibril_mutex_unlock(&hub->guard);

	return EOK;

err_usb_dev:
	hcd_ddf_fun_destroy(dev);
	return err;
}

/** Announce root hub to the DDF
 *
 * @param[in] device Host controller ddf device
 * @return Error code
 */
int hcd_setup_virtual_root_hub(hc_device_t *hcd)
{
	int err;

	assert(hcd);

	if ((err = bus_reserve_default_address(hcd->bus, USB_SPEED_MAX))) {
		usb_log_error("Failed to reserve default address for roothub setup: %s", str_error(err));
		return err;
	}

	device_t *dev = hcd_ddf_fun_create(hcd);
	if (!dev) {
		usb_log_error("Failed to create function for the root hub.");
		goto err_default_address;
	}

	ddf_fun_set_name(dev->fun, "roothub");

	/* Assign an address to the device */
	if ((err = bus_device_enumerate(dev))) {
		usb_log_error("Failed to enumerate roothub device: %s", str_error(err));
		goto err_usb_dev;
	}

	if ((err = ddf_fun_bind(dev->fun))) {
		usb_log_error("Failed to register roothub: %s.", str_error(err));
		goto err_usb_dev;
	}

	bus_release_default_address(hcd->bus);
	return EOK;

err_usb_dev:
	hcd_ddf_fun_destroy(dev);
err_default_address:
	bus_release_default_address(hcd->bus);
	return err;
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
		usb_log_error("Failed to allocate HCD ddf structure.\n");
		return ENOMEM;
	}
	instance->ddf_dev = device;

	int ret = ENOMEM;
	instance->ctl_fun = ddf_fun_create(device, fun_exposed, "ctl");
	if (!instance->ctl_fun) {
		usb_log_error("Failed to create HCD ddf fun.\n");
		goto err_destroy_fun;
	}

	ret = ddf_fun_bind(instance->ctl_fun);
	if (ret != EOK) {
		usb_log_error("Failed to bind ctl_fun: %s.\n", str_error(ret));
		goto err_destroy_fun;
	}

	ret = ddf_fun_add_to_category(instance->ctl_fun, USB_HC_CATEGORY);
	if (ret != EOK) {
		usb_log_error("Failed to add fun to category: %s.\n",
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
