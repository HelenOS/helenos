/*
 * Copyright (c) 2018 Ondrej Hlavaty, Petr Manek
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
 * The Bus is a structure that serves as an interface of the HC driver
 * implementation for the usbhost library. Every HC driver that uses libusbhost
 * must use a bus_t (or its child), fill it with bus_ops and present it to the
 * library. The library then handles the DDF interface and translates it to the
 * bus callbacks.
 */

#include <ddf/driver.h>
#include <errno.h>
#include <mem.h>
#include <macros.h>
#include <stdio.h>
#include <str_error.h>
#include <usb/debug.h>
#include <usb/dma_buffer.h>

#include "endpoint.h"
#include "bus.h"

/**
 * Initializes the base bus structure.
 */
void bus_init(bus_t *bus, size_t device_size)
{
	assert(bus);
	assert(device_size >= sizeof(device_t));
	memset(bus, 0, sizeof(bus_t));

	fibril_mutex_initialize(&bus->guard);
	bus->device_size = device_size;
}

/**
 * Initialize the device_t structure belonging to a bus.
 */
int bus_device_init(device_t *dev, bus_t *bus)
{
	assert(bus);

	memset(dev, 0, sizeof(*dev));

	dev->bus = bus;

	link_initialize(&dev->link);
	list_initialize(&dev->devices);
	fibril_mutex_initialize(&dev->guard);

	return EOK;
}

/**
 * Create a name of the ddf function node.
 */
int bus_device_set_default_name(device_t *dev)
{
	assert(dev);
	assert(dev->fun);

	char buf[10] = { 0 }; /* usbxyz-ss */
	snprintf(buf, sizeof(buf), "usb%u-%cs",
	    dev->address, usb_str_speed(dev->speed)[0]);

	return ddf_fun_set_name(dev->fun, buf);
}

/**
 * Setup devices Transaction Translation.
 *
 * This applies for Low/Full speed devices under High speed hub only. Other
 * devices just inherit TT from the hub.
 *
 * Roothub must be handled specially.
 */
static void device_setup_tt(device_t *dev)
{
	if (!dev->hub)
		return;

	if (dev->hub->speed == USB_SPEED_HIGH && usb_speed_is_11(dev->speed)) {
		/* For LS devices under HS hub */
		dev->tt.dev = dev->hub;
		dev->tt.port = dev->port;
	}
	else {
		/* Inherit hub's TT */
		dev->tt = dev->hub->tt;
	}
}

/**
 * Invoke the device_enumerate bus operation.
 *
 * There's no need to synchronize here, because no one knows the device yet.
 */
int bus_device_enumerate(device_t *dev)
{
	assert(dev);

	if (!dev->bus->ops->device_enumerate)
		return ENOTSUP;

	if (dev->online)
		return EINVAL;

	device_setup_tt(dev);

	const int r = dev->bus->ops->device_enumerate(dev);
	if (r)
		return r;

	dev->online = true;

	if (dev->hub) {
		fibril_mutex_lock(&dev->hub->guard);
		list_append(&dev->link, &dev->hub->devices);
		fibril_mutex_unlock(&dev->hub->guard);
	}

	return EOK;
}

/**
 * Clean endpoints and children that could have been left behind after
 * asking the driver of device to offline/remove a device.
 *
 * Note that EP0's lifetime is shared with the device, and as such is not
 * touched.
 */
static void device_clean_ep_children(device_t *dev, const char *op)
{
	assert(fibril_mutex_is_locked(&dev->guard));

	/* Unregister endpoints left behind. */
	for (usb_endpoint_t i = 1; i < USB_ENDPOINT_MAX; ++i) {
		if (!dev->endpoints[i])
			continue;

		usb_log_warning("USB device '%s' driver left endpoint %u registered after %s.",
		    ddf_fun_get_name(dev->fun), i, op);

		endpoint_t * const ep = dev->endpoints[i];
		endpoint_add_ref(ep);

		fibril_mutex_unlock(&dev->guard);
		const int err = bus_endpoint_remove(ep);
		if (err)
			usb_log_warning("Endpoint %u cannot be removed. "
			    "Some deffered cleanup was faster?", ep->endpoint);

		endpoint_del_ref(ep);
		fibril_mutex_lock(&dev->guard);
	}

	for (usb_endpoint_t i = 1; i < USB_ENDPOINT_MAX; ++i)
		assert(dev->endpoints[i] == NULL);

	/* Remove also orphaned children. */
	while (!list_empty(&dev->devices)) {
		device_t * const child = list_get_instance(list_first(&dev->devices), device_t, link);

		/*
		 * This is not an error condition, as devices cannot remove
		 * their children devices while they are removed, because for
		 * DDF, they are siblings.
		 */
		usb_log_debug("USB device '%s' driver left device '%s' behind after %s.",
		    ddf_fun_get_name(dev->fun), ddf_fun_get_name(child->fun), op);

		/*
		 * The child node won't disappear, because its parent's driver
		 * is already dead. And the child will need the guard to remove
		 * itself from the list.
		 */
		fibril_mutex_unlock(&dev->guard);
		bus_device_gone(child);
		fibril_mutex_lock(&dev->guard);
	}
	assert(list_empty(&dev->devices));
}

/**
 * Resolve a USB device that is gone.
 */
void bus_device_gone(device_t *dev)
{
	assert(dev);
	assert(dev->fun != NULL);

	const bus_ops_t *ops = dev->bus->ops;

	/* First, block new transfers and operations. */
	fibril_mutex_lock(&dev->guard);
	dev->online = false;

	/* Unbinding will need guard unlocked. */
	fibril_mutex_unlock(&dev->guard);

	/* Remove our device from our hub's children. */
	if (dev->hub) {
		fibril_mutex_lock(&dev->hub->guard);
		list_remove(&dev->link);
		fibril_mutex_unlock(&dev->hub->guard);
	}

	/*
	 * Unbind the DDF function. That will result in dev_gone called in
	 * driver, which shall destroy its pipes and remove its children.
	 */
	const int err = ddf_fun_unbind(dev->fun);
	if (err) {
		usb_log_error("Failed to unbind USB device '%s': %s",
		    ddf_fun_get_name(dev->fun), str_error(err));
		return;
	}

	/* Remove what driver left behind */
	fibril_mutex_lock(&dev->guard);
	device_clean_ep_children(dev, "removing");

	/* Tell the HC to release its resources. */
	if (ops->device_gone)
		ops->device_gone(dev);

	/* Check whether the driver didn't forgot EP0 */
	if (dev->endpoints[0]) {
		if (ops->endpoint_unregister)
			ops->endpoint_unregister(dev->endpoints[0]);
		/* Release the EP0 bus reference */
		endpoint_del_ref(dev->endpoints[0]);
	}

	/* Destroy the function, freeing also the device, unlocking mutex. */
	ddf_fun_destroy(dev->fun);
}

/**
 * The user wants this device back online.
 */
int bus_device_online(device_t *dev)
{
	int rc;
	assert(dev);

	fibril_mutex_lock(&dev->guard);
	if (dev->online) {
		rc = EINVAL;
		goto err_lock;
	}

	/* First, tell the HC driver. */
	const bus_ops_t *ops = dev->bus->ops;
	if (ops->device_online && (rc = ops->device_online(dev))) {
		usb_log_warning("Host controller failed to make device '%s' online: %s",
		    ddf_fun_get_name(dev->fun), str_error(rc));
		goto err_lock;
	}

	/* Allow creation of new endpoints and communication with the device. */
	dev->online = true;

	/* Onlining will need the guard */
	fibril_mutex_unlock(&dev->guard);

	if ((rc = ddf_fun_online(dev->fun))) {
		usb_log_warning("Failed to take device '%s' online: %s",
		    ddf_fun_get_name(dev->fun), str_error(rc));
		goto err;
	}

	usb_log_info("USB Device '%s' is now online.", ddf_fun_get_name(dev->fun));
	return EOK;

err_lock:
	fibril_mutex_unlock(&dev->guard);
err:
	return rc;
}

/**
 * The user requested to take this device offline.
 */
int bus_device_offline(device_t *dev)
{
	int rc;
	assert(dev);

	/* Make sure we're the one who offlines this device */
	if (!dev->online) {
		rc = ENOENT;
		goto err;
	}

	/*
	 * XXX: If the device is removed/offlined just now, this can fail on
	 * assertion. We most probably need some kind of enum status field to
	 * make the synchronization work.
	 */

	/* Tear down all drivers working with the device. */
	if ((rc = ddf_fun_offline(dev->fun))) {
		goto err;
	}

	fibril_mutex_lock(&dev->guard);
	dev->online = false;
	device_clean_ep_children(dev, "offlining");

	/* Tell also the HC driver. */
	const bus_ops_t *ops = dev->bus->ops;
	if (ops->device_offline)
		ops->device_offline(dev);

	fibril_mutex_unlock(&dev->guard);
	usb_log_info("USB Device '%s' is now offline.", ddf_fun_get_name(dev->fun));
	return EOK;

err:
	return rc;
}

/**
 * Calculate an index to the endpoint array.
 * For the default control endpoint 0, it must return 0.
 * For different arguments, the result is stable but not defined.
 */
static size_t bus_endpoint_index(usb_endpoint_t ep, usb_direction_t dir)
{
	return 2 * ep + (dir == USB_DIRECTION_OUT);
}

/**
 * Create and register new endpoint to the bus.
 *
 * @param[in] device The device of which the endpoint shall be created
 * @param[in] desc Endpoint descriptors as reported by the device
 * @param[out] out_ep The resulting new endpoint reference, if any. Can be NULL.
 */
int bus_endpoint_add(device_t *device, const usb_endpoint_descriptors_t *desc, endpoint_t **out_ep)
{
	int err;
	assert(device);

	bus_t *bus = device->bus;

	if (!bus->ops->endpoint_register)
		return ENOTSUP;

	endpoint_t *ep;
	if (bus->ops->endpoint_create) {
		ep = bus->ops->endpoint_create(device, desc);
		if (!ep)
			return ENOMEM;
	} else {
		ep = calloc(1, sizeof(endpoint_t));
		if (!ep)
			return ENOMEM;
		endpoint_init(ep, device, desc);
	}

	assert((ep->required_transfer_buffer_policy & ~ep->transfer_buffer_policy) == 0);

	/* Bus reference */
	endpoint_add_ref(ep);

	const size_t idx = bus_endpoint_index(ep->endpoint, ep->direction);
	if (idx >= ARRAY_SIZE(device->endpoints)) {
		usb_log_warning("Invalid endpoint description (ep no %u out of "
		    "bounds)", ep->endpoint);
		goto drop;
	}

	if (ep->max_transfer_size == 0) {
		usb_log_warning("Invalid endpoint description (mps %zu, "
			"%u packets)", ep->max_packet_size, ep->packets_per_uframe);
		goto drop;
	}

	usb_log_debug("Register endpoint %d:%d %s-%s %zuB.",
	    device->address, ep->endpoint,
	    usb_str_transfer_type(ep->transfer_type),
	    usb_str_direction(ep->direction),
	    ep->max_transfer_size);

	fibril_mutex_lock(&device->guard);
	if (!device->online && ep->endpoint != 0) {
		err = EAGAIN;
	} else if (device->endpoints[idx] != NULL) {
		err = EEXIST;
	} else {
		err = bus->ops->endpoint_register(ep);
		if (!err)
			device->endpoints[idx] = ep;
	}
	fibril_mutex_unlock(&device->guard);
	if (err) {
		endpoint_del_ref(ep);
		return err;
	}

	if (out_ep) {
		/* Exporting reference */
		endpoint_add_ref(ep);
		*out_ep = ep;
	}

	return EOK;
drop:
	/* Bus reference */
	endpoint_del_ref(ep);
	return EINVAL;
}

/**
 * Search for an endpoint. Returns a reference.
 */
endpoint_t *bus_find_endpoint(device_t *device, usb_endpoint_t endpoint,
    usb_direction_t dir)
{
	assert(device);

	const size_t idx = bus_endpoint_index(endpoint, dir);
	const size_t ctrl_idx = bus_endpoint_index(endpoint, USB_DIRECTION_BOTH);

	endpoint_t *ep = NULL;

	fibril_mutex_lock(&device->guard);
	if (idx < ARRAY_SIZE(device->endpoints))
		ep = device->endpoints[idx];
	/*
	 * If the endpoint was not found, it's still possible it is a control
	 * endpoint having direction BOTH.
	 */
	if (!ep && ctrl_idx < ARRAY_SIZE(device->endpoints)) {
		ep = device->endpoints[ctrl_idx];
		if (ep && ep->transfer_type != USB_TRANSFER_CONTROL)
			ep = NULL;
	}
	if (ep) {
		/* Exporting reference */
		endpoint_add_ref(ep);
	}
	fibril_mutex_unlock(&device->guard);
	return ep;
}

/**
 * Remove an endpoint from the device.
 */
int bus_endpoint_remove(endpoint_t *ep)
{
	assert(ep);
	assert(ep->device);

	device_t *device = ep->device;
	if (!device)
		return ENOENT;

	bus_t *bus = device->bus;

	if (!bus->ops->endpoint_unregister)
		return ENOTSUP;

	usb_log_debug("Unregister endpoint %d:%d %s-%s %zuB.",
	    device->address, ep->endpoint,
	    usb_str_transfer_type(ep->transfer_type),
	    usb_str_direction(ep->direction),
	    ep->max_transfer_size);

	const size_t idx = bus_endpoint_index(ep->endpoint, ep->direction);

	if (idx >= ARRAY_SIZE(device->endpoints))
		return EINVAL;

	fibril_mutex_lock(&device->guard);

	/* Check whether the endpoint is registered */
	if (device->endpoints[idx] != ep) {
		fibril_mutex_unlock(&device->guard);
		return EINVAL;
	}

	bus->ops->endpoint_unregister(ep);
	device->endpoints[idx] = NULL;
	fibril_mutex_unlock(&device->guard);

	/* Bus reference */
	endpoint_del_ref(ep);

	return EOK;
}

/**
 * Reserve the default address on the bus for the specified device (hub).
 */
int bus_reserve_default_address(bus_t *bus, device_t *dev)
{
	assert(bus);

	int err;
	fibril_mutex_lock(&bus->guard);
	if (bus->default_address_owner != NULL) {
		err = (bus->default_address_owner == dev) ? EINVAL : EAGAIN;
	} else {
		bus->default_address_owner = dev;
		err = EOK;
	}
	fibril_mutex_unlock(&bus->guard);
	return err;
}

/**
 * Release the default address.
 */
void bus_release_default_address(bus_t *bus, device_t *dev)
{
	assert(bus);

	fibril_mutex_lock(&bus->guard);
	if (bus->default_address_owner != dev) {
		usb_log_error("Device %d tried to release default address, "
		    "which is not reserved for it.", dev->address);
	} else {
		bus->default_address_owner = NULL;
	}
	fibril_mutex_unlock(&bus->guard);
}

/**
 * Assert some conditions on transfer request. As the request is an entity of
 * HC driver only, we can force these conditions harder. Invalid values from
 * devices shall be caught on DDF interface already.
 */
static void check_request(const transfer_request_t *request)
{
	assert(usb_target_is_valid(&request->target));
	assert(request->dir != USB_DIRECTION_BOTH);
	/* Non-zero offset => size is non-zero */
	assert(request->offset == 0 || request->size != 0);
	/* Non-zero size => buffer is set */
	assert(request->size == 0 || dma_buffer_is_set(&request->buffer));
	/* Non-null arg => callback is set */
	assert(request->arg == NULL || request->on_complete != NULL);
	assert(request->name);
}

/**
 * Initiate a transfer with given device.
 *
 * @return Error code.
 */
int bus_issue_transfer(device_t *device, const transfer_request_t *request)
{
	assert(device);
	assert(request);

	check_request(request);
	assert(device->address == request->target.address);

	/* Temporary reference */
	endpoint_t *ep = bus_find_endpoint(device, request->target.endpoint, request->dir);
	if (ep == NULL) {
		usb_log_error("Endpoint(%d:%d) not registered for %s.",
		    device->address, request->target.endpoint, request->name);
		return ENOENT;
	}

	assert(ep->device == device);

	const int err = endpoint_send_batch(ep, request);

	/* Temporary reference */
	endpoint_del_ref(ep);

	return err;
}

/**
 * A structure to pass data from the completion callback to the caller.
 */
typedef struct {
	fibril_mutex_t done_mtx;
	fibril_condvar_t done_cv;
	bool done;

	size_t transferred_size;
	int error;
} sync_data_t;

/**
 * Callback for finishing the transfer. Wake the issuing thread.
 */
static int sync_transfer_complete(void *arg, int error, size_t transferred_size)
{
	sync_data_t *d = arg;
	assert(d);
	d->transferred_size = transferred_size;
	d->error = error;
	fibril_mutex_lock(&d->done_mtx);
	d->done = true;
	fibril_condvar_broadcast(&d->done_cv);
	fibril_mutex_unlock(&d->done_mtx);
	return EOK;
}

/**
 * Issue a transfer on the bus, wait for the result.
 *
 * @param device Device for which to send the batch
 * @param target The target of the transfer.
 * @param direction A direction of the transfer.
 * @param data A pointer to the data buffer.
 * @param size Size of the data buffer.
 * @param setup_data Data to use in the setup stage (Control communication type)
 * @param name Communication identifier (for nicer output).
 */
errno_t bus_device_send_batch_sync(device_t *device, usb_target_t target,
    usb_direction_t direction, char *data, size_t size, uint64_t setup_data,
    const char *name, size_t *transferred_size)
{
	int err;
	sync_data_t sd = { .done = false };
	fibril_mutex_initialize(&sd.done_mtx);
	fibril_condvar_initialize(&sd.done_cv);

	transfer_request_t request = {
		.target = target,
		.dir = direction,
		.offset = ((uintptr_t) data) % PAGE_SIZE,
		.size = size,
		.setup = setup_data,
		.on_complete = sync_transfer_complete,
		.arg = &sd,
		.name = name,
	};

	if (data &&
	    (err = dma_buffer_lock(&request.buffer, data - request.offset, size)))
		return err;

	if ((err = bus_issue_transfer(device, &request))) {
		dma_buffer_unlock(&request.buffer, size);
		return err;
	}

	/*
	 * Note: There are requests that are completed synchronously. It is not
	 *       therefore possible to just lock the mutex before and wait.
	 */
	fibril_mutex_lock(&sd.done_mtx);
	while (!sd.done)
		fibril_condvar_wait(&sd.done_cv, &sd.done_mtx);
	fibril_mutex_unlock(&sd.done_mtx);

	dma_buffer_unlock(&request.buffer, size);

	if (transferred_size)
		*transferred_size = sd.transferred_size;

	return sd.error;
}

/**
 * @}
 */
