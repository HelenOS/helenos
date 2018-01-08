/*
 * Copyright (c) 2017 Ondrej Hlavaty <aearsis@eideo.cz>
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
#include <stdio.h>
#include <usb/debug.h>

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
	bus->default_address_speed = USB_SPEED_MAX;
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
	snprintf(buf, sizeof(buf) - 1, "usb%u-%cs",
	    dev->address, usb_str_speed(dev->speed)[0]);

	return ddf_fun_set_name(dev->fun, buf);
}

/**
 * Invoke the device_enumerate bus operation.
 */
int bus_device_enumerate(device_t *dev)
{
	assert(dev);

	const bus_ops_t *ops = BUS_OPS_LOOKUP(dev->bus->ops, device_enumerate);
	if (!ops)
		return ENOTSUP;

	return ops->device_enumerate(dev);
}

/**
 * Invoke the device_remove bus operation.
 */
int bus_device_remove(device_t *dev)
{
	assert(dev);

	const bus_ops_t *ops = BUS_OPS_LOOKUP(dev->bus->ops, device_remove);
	if (!ops)
		return ENOTSUP;

	return ops->device_remove(dev);
}

/**
 * Invoke the device_online bus operation.
 */
int bus_device_online(device_t *dev)
{
	assert(dev);

	const bus_ops_t *ops = BUS_OPS_LOOKUP(dev->bus->ops, device_online);
	if (!ops)
		return ENOTSUP;

	return ops->device_online(dev);
}

/**
 * Invoke the device_offline bus operation.
 */
int bus_device_offline(device_t *dev)
{
	assert(dev);

	const bus_ops_t *ops = BUS_OPS_LOOKUP(dev->bus->ops, device_offline);
	if (!ops)
		return ENOTSUP;

	return ops->device_offline(dev);
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

	const bus_ops_t *register_ops = BUS_OPS_LOOKUP(bus->ops, endpoint_register);
	if (!register_ops)
		return ENOTSUP;

	const bus_ops_t *create_ops = BUS_OPS_LOOKUP(bus->ops, endpoint_create);
	endpoint_t *ep;
	if (create_ops) {
		ep = create_ops->endpoint_create(device, desc);
		if (!ep)
			return ENOMEM;
	} else {
		ep = calloc(1, sizeof(endpoint_t));
		if (!ep)
			return ENOMEM;
		endpoint_init(ep, device, desc);
	}

	/* Bus reference */
	endpoint_add_ref(ep);

	if (ep->max_transfer_size == 0) {
		usb_log_warning("Invalid endpoint description (mps %zu, "
			"%u packets)", ep->max_packet_size, ep->packets_per_uframe);
		/* Bus reference */
		endpoint_del_ref(ep);
		return EINVAL;
	}

	usb_log_debug("Register endpoint %d:%d %s-%s %zuB.\n",
	    device->address, ep->endpoint,
	    usb_str_transfer_type(ep->transfer_type),
	    usb_str_direction(ep->direction),
	    ep->max_transfer_size);

	fibril_mutex_lock(&bus->guard);
	if (!device->online && ep->endpoint != 0) {
		err = EAGAIN;
	} else if (device->endpoints[ep->endpoint] != NULL) {
		err = EEXIST;
	} else {
		err = register_ops->endpoint_register(ep);
		if (!err)
			device->endpoints[ep->endpoint] = ep;
	}
	fibril_mutex_unlock(&bus->guard);
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
}

/**
 * Search for an endpoint. Returns a reference.
 */
endpoint_t *bus_find_endpoint(device_t *device, usb_endpoint_t endpoint)
{
	assert(device);

	bus_t *bus = device->bus;

	fibril_mutex_lock(&bus->guard);
	endpoint_t *ep = device->endpoints[endpoint];
	if (ep) {
		/* Exporting reference */
		endpoint_add_ref(ep);
	}
	fibril_mutex_unlock(&bus->guard);
	return ep;
}

/**
 * Remove an endpoint from the device. Consumes a reference.
 */
int bus_endpoint_remove(endpoint_t *ep)
{
	assert(ep);
	assert(ep->device);

	bus_t *bus = endpoint_get_bus(ep);

	const bus_ops_t *ops = BUS_OPS_LOOKUP(bus->ops, endpoint_unregister);
	if (!ops)
		return ENOTSUP;

	usb_log_debug("Unregister endpoint %d:%d %s-%s %zuB.\n",
	    ep->device->address, ep->endpoint,
	    usb_str_transfer_type(ep->transfer_type),
	    usb_str_direction(ep->direction),
	    ep->max_transfer_size);

	fibril_mutex_lock(&bus->guard);
	const int r = ops->endpoint_unregister(ep);
	if (!r)
		ep->device->endpoints[ep->endpoint] = NULL;
	fibril_mutex_unlock(&bus->guard);

	if (r)
		return r;

	/* Bus reference */
	endpoint_del_ref(ep);

	/* Given reference */
	endpoint_del_ref(ep);

	return EOK;
}

/**
 * Reserve the default address on the bus. Also, report the speed of the device
 * that is listening on the default address.
 *
 * The speed is then used for devices enumerated while the address is reserved.
 */
int bus_reserve_default_address(bus_t *bus, usb_speed_t speed)
{
	assert(bus);

	fibril_mutex_lock(&bus->guard);
	if (bus->default_address_speed != USB_SPEED_MAX) {
		fibril_mutex_unlock(&bus->guard);
		return EAGAIN;
	} else {
		bus->default_address_speed = speed;
		fibril_mutex_unlock(&bus->guard);
		return EOK;
	}
}

/**
 * Release the default address.
 */
void bus_release_default_address(bus_t *bus)
{
	assert(bus);
	bus->default_address_speed = USB_SPEED_MAX;
}

/**
 * Initiate a transfer on the bus. Finds the target endpoint, creates
 * a transfer batch and schedules it.
 *
 * @param device Device for which to send the batch
 * @param target The target of the transfer.
 * @param direction A direction of the transfer.
 * @param data A pointer to the data buffer.
 * @param size Size of the data buffer.
 * @param setup_data Data to use in the setup stage (Control communication type)
 * @param on_complete Callback which is called after the batch is complete
 * @param arg Callback parameter.
 * @param name Communication identifier (for nicer output).
 * @return Error code.
 */
int bus_device_send_batch(device_t *device, usb_target_t target,
    usb_direction_t direction, char *data, size_t size, uint64_t setup_data,
    usbhc_iface_transfer_callback_t on_complete, void *arg, const char *name)
{
	assert(device->address == target.address);

	/* Temporary reference */
	endpoint_t *ep = bus_find_endpoint(device, target.endpoint);
	if (ep == NULL) {
		usb_log_error("Endpoint(%d:%d) not registered for %s.\n",
		    device->address, target.endpoint, name);
		return ENOENT;
	}

	assert(ep->device == device);

	const int err = endpoint_send_batch(ep, target, direction, data, size, setup_data,
	    on_complete, arg, name);

	/* Temporary reference */
	endpoint_del_ref(ep);

	return err;
}

typedef struct {
	fibril_mutex_t done_mtx;
	fibril_condvar_t done_cv;
	unsigned done;

	size_t transfered_size;
	int error;
} sync_data_t;

/**
 * Callback for finishing the transfer. Wake the issuing thread.
 */
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

/**
 * Issue a transfer on the bus, wait for result.
 *
 * @param device Device for which to send the batch
 * @param target The target of the transfer.
 * @param direction A direction of the transfer.
 * @param data A pointer to the data buffer.
 * @param size Size of the data buffer.
 * @param setup_data Data to use in the setup stage (Control communication type)
 * @param name Communication identifier (for nicer output).
 */
ssize_t bus_device_send_batch_sync(device_t *device, usb_target_t target,
    usb_direction_t direction, char *data, size_t size, uint64_t setup_data,
    const char *name)
{
	sync_data_t sd = { .done = 0 };
	fibril_mutex_initialize(&sd.done_mtx);
	fibril_condvar_initialize(&sd.done_cv);

	const int ret = bus_device_send_batch(device, target, direction,
	    data, size, setup_data,
	    sync_transfer_complete, &sd, name);
	if (ret != EOK)
		return ret;

	fibril_mutex_lock(&sd.done_mtx);
	while (!sd.done) {
		fibril_condvar_wait(&sd.done_cv, &sd.done_mtx);
	}
	fibril_mutex_unlock(&sd.done_mtx);

	return (sd.error == EOK)
		? (ssize_t) sd.transfered_size
		: (ssize_t) sd.error;
}

/**
 * @}
 */
