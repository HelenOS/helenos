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

#include <usb/classes/classes.h>
#include <usb/debug.h>
#include <usb/descriptor.h>
#include <usb/request.h>
#include <usb/usb.h>

#include <adt/list.h>
#include <assert.h>
#include <async.h>
#include <ddf/driver.h>
#include <ddf/interrupt.h>
#include <device/hw_res_parsed.h>
#include <errno.h>
#include <fibril_synch.h>
#include <macros.h>
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>
#include <usb_iface.h>

#include "ddf_helpers.h"

#define CTRL_PIPE_MIN_PACKET_SIZE 8

typedef struct usb_dev {
	link_t link;
	list_t devices;
	fibril_mutex_t guard;
	ddf_fun_t *fun;
	usb_address_t address;
	usb_speed_t speed;
	usb_address_t tt_address;
	unsigned port;
} usb_dev_t;

typedef struct hc_dev {
	ddf_fun_t *ctl_fun;
	hcd_t hcd;
	usb_dev_t *root_hub;
} hc_dev_t;

static hc_dev_t *dev_to_hc_dev(ddf_dev_t *dev)
{
	return ddf_dev_data_get(dev);
}

hcd_t *dev_to_hcd(ddf_dev_t *dev)
{
	hc_dev_t *hc_dev = dev_to_hc_dev(dev);
	if (!hc_dev) {
		usb_log_error("Invalid HCD device.\n");
		return NULL;
	}
	return &hc_dev->hcd;
}


static errno_t hcd_ddf_new_device(ddf_dev_t *device, usb_dev_t *hub, unsigned port);
static errno_t hcd_ddf_remove_device(ddf_dev_t *device, usb_dev_t *hub, unsigned port);


/* DDF INTERFACE */

/** Register endpoint interface function.
 * @param fun DDF function.
 * @param address USB address of the device.
 * @param endpoint USB endpoint number to be registered.
 * @param transfer_type Endpoint's transfer type.
 * @param direction USB communication direction the endpoint is capable of.
 * @param max_packet_size Maximu size of packets the endpoint accepts.
 * @param interval Preferred timeout between communication.
 * @return Error code.
 */
static errno_t register_endpoint(
    ddf_fun_t *fun, usb_endpoint_t endpoint,
    usb_transfer_type_t transfer_type, usb_direction_t direction,
    size_t max_packet_size, unsigned packets, unsigned interval)
{
	assert(fun);
	hcd_t *hcd = dev_to_hcd(ddf_fun_get_dev(fun));
	usb_dev_t *dev = ddf_fun_data_get(fun);
	assert(hcd);
	assert(dev);
	const size_t size = max_packet_size;
	const usb_target_t target =
	    {{.address = dev->address, .endpoint = endpoint}};

	usb_log_debug("Register endpoint %d:%d %s-%s %zuB %ums.\n",
	    dev->address, endpoint, usb_str_transfer_type(transfer_type),
	    usb_str_direction(direction), max_packet_size, interval);

	return hcd_add_ep(hcd, target, direction, transfer_type,
	    max_packet_size, packets, size, dev->tt_address, dev->port);
}

/** Unregister endpoint interface function.
 * @param fun DDF function.
 * @param address USB address of the endpoint.
 * @param endpoint USB endpoint number.
 * @param direction Communication direction of the enpdoint to unregister.
 * @return Error code.
 */
static errno_t unregister_endpoint(
    ddf_fun_t *fun, usb_endpoint_t endpoint, usb_direction_t direction)
{
	assert(fun);
	hcd_t *hcd = dev_to_hcd(ddf_fun_get_dev(fun));
	usb_dev_t *dev = ddf_fun_data_get(fun);
	assert(hcd);
	assert(dev);
	const usb_target_t target =
	    {{.address = dev->address, .endpoint = endpoint}};
	usb_log_debug("Unregister endpoint %d:%d %s.\n",
	    dev->address, endpoint, usb_str_direction(direction));
	return hcd_remove_ep(hcd, target, direction);
}

static errno_t reserve_default_address(ddf_fun_t *fun, usb_speed_t speed)
{
	assert(fun);
	hcd_t *hcd = dev_to_hcd(ddf_fun_get_dev(fun));
	usb_dev_t *dev = ddf_fun_data_get(fun);
	assert(hcd);
	assert(dev);

	usb_log_debug("Device %d requested default address at %s speed\n",
	    dev->address, usb_str_speed(speed));
	return hcd_reserve_default_address(hcd, speed);
}

static errno_t release_default_address(ddf_fun_t *fun)
{
	assert(fun);
	hcd_t *hcd = dev_to_hcd(ddf_fun_get_dev(fun));
	usb_dev_t *dev = ddf_fun_data_get(fun);
	assert(hcd);
	assert(dev);

	usb_log_debug("Device %d released default address\n", dev->address);
	return hcd_release_default_address(hcd);
}

static errno_t device_enumerate(ddf_fun_t *fun, unsigned port)
{
	assert(fun);
	ddf_dev_t *ddf_dev = ddf_fun_get_dev(fun);
	usb_dev_t *dev = ddf_fun_data_get(fun);
	assert(ddf_dev);
	assert(dev);
	usb_log_debug("Hub %d reported a new USB device on port: %u\n",
	    dev->address, port);
	return hcd_ddf_new_device(ddf_dev, dev, port);
}

static errno_t device_remove(ddf_fun_t *fun, unsigned port)
{
	assert(fun);
	ddf_dev_t *ddf_dev = ddf_fun_get_dev(fun);
	usb_dev_t *dev = ddf_fun_data_get(fun);
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
static errno_t get_my_device_handle(ddf_fun_t *fun, devman_handle_t *handle)
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
static errno_t dev_read(ddf_fun_t *fun, usb_endpoint_t endpoint,
    uint64_t setup_data, uint8_t *data, size_t size,
    usbhc_iface_transfer_in_callback_t callback, void *arg)
{
	assert(fun);
	usb_dev_t *usb_dev = ddf_fun_data_get(fun);
	assert(usb_dev);
	const usb_target_t target = {{
	    .address =  usb_dev->address,
	    .endpoint = endpoint,
	}};
	return hcd_send_batch(dev_to_hcd(ddf_fun_get_dev(fun)), target,
	    USB_DIRECTION_IN, data, size, setup_data, callback, NULL, arg,
	    "READ");
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
static errno_t dev_write(ddf_fun_t *fun, usb_endpoint_t endpoint,
    uint64_t setup_data, const uint8_t *data, size_t size,
    usbhc_iface_transfer_out_callback_t callback, void *arg)
{
	assert(fun);
	usb_dev_t *usb_dev = ddf_fun_data_get(fun);
	assert(usb_dev);
	const usb_target_t target = {{
	    .address =  usb_dev->address,
	    .endpoint = endpoint,
	}};
	return hcd_send_batch(dev_to_hcd(ddf_fun_get_dev(fun)),
	    target, USB_DIRECTION_OUT, (uint8_t*)data, size, setup_data, NULL,
	    callback, arg, "WRITE");
}

/** USB device interface */
static usb_iface_t usb_iface = {
	.get_my_device_handle = get_my_device_handle,

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
};


/* DDF HELPERS */

#define GET_DEVICE_DESC(size) \
{ \
	.request_type = SETUP_REQUEST_TYPE_DEVICE_TO_HOST \
	    | (USB_REQUEST_TYPE_STANDARD << 5) \
	    | USB_REQUEST_RECIPIENT_DEVICE, \
	.request = USB_DEVREQ_GET_DESCRIPTOR, \
	.value = uint16_host2usb(USB_DESCTYPE_DEVICE << 8), \
	.index = uint16_host2usb(0), \
	.length = uint16_host2usb(size), \
};

#define SET_ADDRESS(address) \
{ \
	.request_type = SETUP_REQUEST_TYPE_HOST_TO_DEVICE \
	    | (USB_REQUEST_TYPE_STANDARD << 5) \
	    | USB_REQUEST_RECIPIENT_DEVICE, \
	.request = USB_DEVREQ_SET_ADDRESS, \
	.value = uint16_host2usb(address), \
	.index = uint16_host2usb(0), \
	.length = uint16_host2usb(0), \
};

static errno_t hcd_ddf_add_device(ddf_dev_t *parent, usb_dev_t *hub_dev,
    unsigned port, usb_address_t address, usb_speed_t speed, const char *name,
    const match_id_list_t *mids)
{
	assert(parent);

	char default_name[10] = { 0 }; /* usbxyz-ss */
	if (!name) {
		snprintf(default_name, sizeof(default_name) - 1,
		    "usb%u-%cs", address, usb_str_speed(speed)[0]);
		name = default_name;
	}

	ddf_fun_t *fun = ddf_fun_create(parent, fun_inner, name);
	if (!fun)
		return ENOMEM;
	usb_dev_t *info = ddf_fun_data_alloc(fun, sizeof(usb_dev_t));
	if (!info) {
		ddf_fun_destroy(fun);
		return ENOMEM;
	}
	info->address = address;
	info->speed = speed;
	info->fun = fun;
	info->port = port;
	info->tt_address = hub_dev ? hub_dev->tt_address : -1;
	link_initialize(&info->link);
	list_initialize(&info->devices);
	fibril_mutex_initialize(&info->guard);

	if (hub_dev && hub_dev->speed == USB_SPEED_HIGH && usb_speed_is_11(speed))
		info->tt_address = hub_dev->address;

	ddf_fun_set_ops(fun, &usb_ops);
	list_foreach(mids->ids, link, const match_id_t, mid) {
		ddf_fun_add_match_id(fun, mid->id, mid->score);
	}

	errno_t ret = ddf_fun_bind(fun);
	if (ret != EOK) {
		ddf_fun_destroy(fun);
		return ret;
	}

	if (hub_dev) {
		fibril_mutex_lock(&hub_dev->guard);
		list_append(&info->link, &hub_dev->devices);
		fibril_mutex_unlock(&hub_dev->guard);
	} else {
		hc_dev_t *hc_dev = dev_to_hc_dev(parent);
		assert(hc_dev->root_hub == NULL);
		hc_dev->root_hub = info;
	}
	return EOK;
}

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
static errno_t create_match_ids(match_id_list_t *l,
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

static errno_t hcd_ddf_remove_device(ddf_dev_t *device, usb_dev_t *hub,
    unsigned port)
{
	assert(device);

	hcd_t *hcd = dev_to_hcd(device);
	assert(hcd);

	hc_dev_t *hc_dev = dev_to_hc_dev(device);
	assert(hc_dev);

	fibril_mutex_lock(&hub->guard);

	usb_dev_t *victim = NULL;

	list_foreach(hub->devices, link, usb_dev_t, it) {
		if (it->port == port) {
			victim = it;
			break;
		}
	}
	if (victim) {
		assert(victim->port == port);
		list_remove(&victim->link);
		fibril_mutex_unlock(&hub->guard);
		const errno_t ret = ddf_fun_unbind(victim->fun);
		if (ret == EOK) {
			usb_address_t address = victim->address;
			ddf_fun_destroy(victim->fun);
			hcd_release_address(hcd, address);
		} else {
			usb_log_warning("Failed to unbind device `%s': %s\n",
			    ddf_fun_get_name(victim->fun), str_error(ret));
		}
		return EOK;
	}
	fibril_mutex_unlock(&hub->guard);
	return ENOENT;
}

static errno_t hcd_ddf_new_device(ddf_dev_t *device, usb_dev_t *hub, unsigned port)
{
	assert(device);

	hcd_t *hcd = dev_to_hcd(device);
	assert(hcd);

	usb_speed_t speed = USB_SPEED_MAX;

	/* This checks whether the default address is reserved and gets speed */
	errno_t ret = usb_bus_get_speed(&hcd->bus, USB_ADDRESS_DEFAULT, &speed);
	if (ret != EOK) {
		usb_log_error("Failed to verify speed: %s.", str_error(ret));
		return ret;
	}

	usb_log_debug("Found new %s speed USB device.", usb_str_speed(speed));

	static const usb_target_t default_target = {{
		.address = USB_ADDRESS_DEFAULT,
		.endpoint = 0,
	}};

	usb_address_t address;
	ret = hcd_request_address(hcd, speed, &address);
	if (ret != EOK) {
		usb_log_error("Failed to reserve new address: %s.",
		    str_error(ret));
		return ret;
	}

	usb_log_debug("Reserved new address: %d\n", address);

	const usb_target_t target = {{
		.address = address,
		.endpoint = 0,
	}};

	const usb_address_t tt_address = hub ? hub->tt_address : -1;

	/* Add default pipe on default address */
	usb_log_debug("Device(%d): Adding default target(0:0)\n", address);
	ret = hcd_add_ep(hcd,
	    default_target, USB_DIRECTION_BOTH, USB_TRANSFER_CONTROL,
	    CTRL_PIPE_MIN_PACKET_SIZE, CTRL_PIPE_MIN_PACKET_SIZE, 1,
	    tt_address, port);
	if (ret != EOK) {
		usb_log_error("Device(%d): Failed to add default target: %s.",
		    address, str_error(ret));
		hcd_release_address(hcd, address);
		return ret;
	}

	/* Get max packet size for default pipe */
	usb_standard_device_descriptor_t desc = { 0 };
	const usb_device_request_setup_packet_t get_device_desc_8 =
	    GET_DEVICE_DESC(CTRL_PIPE_MIN_PACKET_SIZE);

	// TODO CALLBACKS
	usb_log_debug("Device(%d): Requesting first 8B of device descriptor.",
	    address);
	size_t got;
	ret = hcd_send_batch_sync(hcd, default_target, USB_DIRECTION_IN,
	    &desc, CTRL_PIPE_MIN_PACKET_SIZE, *(uint64_t *)&get_device_desc_8,
	    "read first 8 bytes of dev descriptor", &got);

	if (ret == EOK && got != CTRL_PIPE_MIN_PACKET_SIZE) {
		ret = EOVERFLOW;
	}

	if (ret != EOK) {
		usb_log_error("Device(%d): Failed to get 8B of dev descr: %s.",
		    address, str_error(ret));
		hcd_remove_ep(hcd, default_target, USB_DIRECTION_BOTH);
		hcd_release_address(hcd, address);
		return ret;
	}

	/* Register EP on the new address */
	usb_log_debug("Device(%d): Registering control EP.", address);
	ret = hcd_add_ep(hcd, target, USB_DIRECTION_BOTH, USB_TRANSFER_CONTROL,
	    ED_MPS_PACKET_SIZE_GET(uint16_usb2host(desc.max_packet_size)),
	    ED_MPS_TRANS_OPPORTUNITIES_GET(uint16_usb2host(desc.max_packet_size)),
	    ED_MPS_PACKET_SIZE_GET(uint16_usb2host(desc.max_packet_size)),
	    tt_address, port);
	if (ret != EOK) {
		usb_log_error("Device(%d): Failed to register EP0: %s",
		    address, str_error(ret));
		hcd_remove_ep(hcd, default_target, USB_DIRECTION_BOTH);
		hcd_remove_ep(hcd, target, USB_DIRECTION_BOTH);
		hcd_release_address(hcd, address);
		return ret;
	}

	/* Set new address */
	const usb_device_request_setup_packet_t set_address =
	    SET_ADDRESS(target.address);

	usb_log_debug("Device(%d): Setting USB address.", address);
	ret = hcd_send_batch_sync(hcd, default_target, USB_DIRECTION_OUT,
	    NULL, 0, *(uint64_t *)&set_address, "set address", &got);

	usb_log_debug("Device(%d): Removing default (0:0) EP.", address);
	hcd_remove_ep(hcd, default_target, USB_DIRECTION_BOTH);

	if (ret != EOK) {
		usb_log_error("Device(%d): Failed to set new address: %s.",
		    address, str_error(ret));
		hcd_remove_ep(hcd, target, USB_DIRECTION_BOTH);
		hcd_release_address(hcd, address);
		return ret;
	}

	/* Get std device descriptor */
	const usb_device_request_setup_packet_t get_device_desc =
	    GET_DEVICE_DESC(sizeof(desc));

	usb_log_debug("Device(%d): Requesting full device descriptor.",
	    address);
	ret = hcd_send_batch_sync(hcd, target, USB_DIRECTION_IN,
	    &desc, sizeof(desc), *(uint64_t *)&get_device_desc,
	    "read device descriptor", &got);
	if (ret != EOK) {
		usb_log_error("Device(%d): Failed to set get dev descriptor: %s",
		    address, str_error(ret));
		hcd_remove_ep(hcd, target, USB_DIRECTION_BOTH);
		hcd_release_address(hcd, target.address);
		return ret;
	}

	/* Create match ids from the device descriptor */
	match_id_list_t mids;
	init_match_ids(&mids);

	usb_log_debug("Device(%d): Creating match IDs.", address);
	ret = create_match_ids(&mids, &desc);
	if (ret != EOK) {
		usb_log_error("Device(%d): Failed to create match ids: %s",
		    address, str_error(ret));
		hcd_remove_ep(hcd, target, USB_DIRECTION_BOTH);
		hcd_release_address(hcd, target.address);
		return ret;
	}

	/* Register device */
	usb_log_debug("Device(%d): Registering DDF device.", address);
	ret = hcd_ddf_add_device(device, hub, port, address, speed, NULL, &mids);
	clean_match_ids(&mids);
	if (ret != EOK) {
		usb_log_error("Device(%d): Failed to register: %s.",
		    address, str_error(ret));
		hcd_remove_ep(hcd, target, USB_DIRECTION_BOTH);
		hcd_release_address(hcd, target.address);
	}

	return ret;
}

/** Announce root hub to the DDF
 *
 * @param[in] device Host controller ddf device
 * @return Error code
 */
errno_t hcd_ddf_setup_root_hub(ddf_dev_t *device)
{
	assert(device);
	hcd_t *hcd = dev_to_hcd(device);
	assert(hcd);

	hcd_reserve_default_address(hcd, hcd->bus.max_speed);
	const errno_t ret = hcd_ddf_new_device(device, NULL, 0);
	hcd_release_default_address(hcd);
	return ret;
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
errno_t hcd_ddf_setup_hc(ddf_dev_t *device, usb_speed_t max_speed,
    size_t bw, bw_count_func_t bw_count)
{
	assert(device);

	hc_dev_t *instance = ddf_dev_data_alloc(device, sizeof(hc_dev_t));
	if (instance == NULL) {
		usb_log_error("Failed to allocate HCD ddf structure.\n");
		return ENOMEM;
	}
	instance->root_hub = NULL;
	hcd_init(&instance->hcd, max_speed, bw, bw_count);

	errno_t ret = ENOMEM;
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

void hcd_ddf_clean_hc(ddf_dev_t *device)
{
	assert(device);
	hc_dev_t *hc = dev_to_hc_dev(device);
	assert(hc);
	const errno_t ret = ddf_fun_unbind(hc->ctl_fun);
	if (ret == EOK)
		ddf_fun_destroy(hc->ctl_fun);
}

//TODO: Cache parent session in HCD
/** Call the parent driver with a request to enable interrupt
 *
 * @param[in] device Device asking for interrupts
 * @param[in] inum Interrupt number
 * @return Error code.
 */
errno_t hcd_ddf_enable_interrupt(ddf_dev_t *device, int inum)
{
	async_sess_t *parent_sess = ddf_dev_parent_sess_get(device);
	if (parent_sess == NULL)
		return EIO;

	return hw_res_enable_interrupt(parent_sess, inum);
}

//TODO: Cache parent session in HCD
errno_t hcd_ddf_get_registers(ddf_dev_t *device, hw_res_list_parsed_t *hw_res)
{
	async_sess_t *parent_sess = ddf_dev_parent_sess_get(device);
	if (parent_sess == NULL)
		return EIO;

	hw_res_list_parsed_init(hw_res);
	const errno_t ret = hw_res_get_list_parsed(parent_sess, hw_res, 0);
	if (ret != EOK)
		hw_res_list_parsed_clean(hw_res);
	return ret;
}

// TODO: move this someplace else
static inline void irq_code_clean(irq_code_t *code)
{
	if (code) {
		free(code->ranges);
		free(code->cmds);
		code->ranges = NULL;
		code->cmds = NULL;
		code->rangecount = 0;
		code->cmdcount = 0;
	}
}

/** Register interrupt handler
 *
 * @param[in] device Host controller DDF device
 * @param[in] regs Register range
 * @param[in] irq Interrupt number
 * @paran[in] handler Interrupt handler
 * @param[in] gen_irq_code IRQ code generator.
 *
 * @param[out] handle  IRQ capability handle on success.
 *
 * @return Error code.
 */
errno_t hcd_ddf_setup_interrupts(ddf_dev_t *device,
    const hw_res_list_parsed_t *hw_res,
    interrupt_handler_t handler,
    errno_t (*gen_irq_code)(irq_code_t *, const hw_res_list_parsed_t *, int *),
    cap_handle_t *handle)
{

	assert(device);
	if (!handler || !gen_irq_code)
		return ENOTSUP;

	irq_code_t irq_code = {0};

	int irq;
	errno_t ret = gen_irq_code(&irq_code, hw_res, &irq);
	if (ret != EOK) {
		usb_log_error("Failed to generate IRQ code: %s.\n",
		    str_error(ret));
		return ret;
	}

	/* Register handler to avoid interrupt lockup */
	ret = register_interrupt_handler(device, irq, handler,
	    &irq_code, handle);
	irq_code_clean(&irq_code);
	if (ret != EOK) {
		usb_log_error("Failed to register interrupt handler: %s.\n",
		    str_error(ret));
		return ret;
	}

	/* Enable interrupts */
	ret = hcd_ddf_enable_interrupt(device, irq);
	if (ret != EOK) {
		usb_log_error("Failed to register interrupt handler: %s.\n",
		    str_error(ret));
		unregister_interrupt_handler(device, *handle);
	}
	return ret;
}

/** IRQ handling callback, forward status from call to diver structure.
 *
 * @param[in] dev DDF instance of the device to use.
 * @param[in] call Pointer to the call from kernel.
 */
void ddf_hcd_gen_irq_handler(ipc_call_t *call, ddf_dev_t *dev)
{
	assert(dev);
	hcd_t *hcd = dev_to_hcd(dev);
	if (!hcd || !hcd->ops.irq_hook) {
		usb_log_error("Interrupt on not yet initialized device.\n");
		return;
	}
	const uint32_t status = IPC_GET_ARG1(*call);
	hcd->ops.irq_hook(hcd, status);
}

static errno_t interrupt_polling(void *arg)
{
	hcd_t *hcd = arg;
	assert(hcd);
	if (!hcd->ops.status_hook || !hcd->ops.irq_hook)
		return ENOTSUP;
	uint32_t status = 0;
	while (hcd->ops.status_hook(hcd, &status) == EOK) {
		hcd->ops.irq_hook(hcd, status);
		status = 0;
		/* We should wait 1 frame - 1ms here, but this polling is a
		 * lame crutch anyway so don't hog the system. 10ms is still
		 * good enough for emergency mode */
		async_usleep(10000);
	}
	return EOK;
}

/** Initialize hc and rh DDF structures and their respective drivers.
 *
 * @param device DDF instance of the device to use
 * @param speed Maximum supported speed
 * @param bw Available bandwidth (arbitrary units)
 * @param bw_count Bandwidth computing function
 * @param irq_handler IRQ handling function
 * @param gen_irq_code Function to generate IRQ pseudocode
 *                     (it needs to return used irq number)
 * @param driver_init Function to initialize HC driver
 * @param driver_fini Function to cleanup HC driver
 * @return Error code
 *
 * This function does all the preparatory work for hc and rh drivers:
 *  - gets device's hw resources
 *  - attempts to enable interrupts
 *  - registers interrupt handler
 *  - calls driver specific initialization
 *  - registers root hub
 */
errno_t hcd_ddf_add_hc(ddf_dev_t *device, const ddf_hc_driver_t *driver)
{
	assert(driver);
	static const struct { size_t bw; bw_count_func_t bw_count; }bw[] = {
	    [USB_SPEED_FULL] = { .bw = BANDWIDTH_AVAILABLE_USB11,
	                         .bw_count = bandwidth_count_usb11 },
	    [USB_SPEED_HIGH] = { .bw = BANDWIDTH_AVAILABLE_USB11,
	                         .bw_count = bandwidth_count_usb11 },
	};

	errno_t ret = EOK;
	const usb_speed_t speed = driver->hc_speed;
	if (speed >= ARRAY_SIZE(bw) || bw[speed].bw == 0) {
		usb_log_error("Driver `%s' reported unsupported speed: %s",
		    driver->name, usb_str_speed(speed));
		return ENOTSUP;
	}

	hw_res_list_parsed_t hw_res;
	ret = hcd_ddf_get_registers(device, &hw_res);
	if (ret != EOK) {
		usb_log_error("Failed to get register memory addresses "
		    "for `%s': %s.\n", ddf_dev_get_name(device),
		    str_error(ret));
		return ret;
	}

	ret = hcd_ddf_setup_hc(device, speed, bw[speed].bw, bw[speed].bw_count);
	if (ret != EOK) {
		usb_log_error("Failed to setup generic HCD.\n");
		hw_res_list_parsed_clean(&hw_res);
		return ret;
	}

	interrupt_handler_t *irq_handler =
	    driver->irq_handler ? driver->irq_handler : ddf_hcd_gen_irq_handler;
	int irq_cap;
	errno_t irq_ret = hcd_ddf_setup_interrupts(device, &hw_res,
	    irq_handler, driver->irq_code_gen, &irq_cap);
	bool irqs_enabled = (irq_ret == EOK);
	if (irqs_enabled) {
		usb_log_debug("Hw interrupts enabled.\n");
	}

	if (driver->claim) {
		ret = driver->claim(device);
		if (ret != EOK) {
			usb_log_error("Failed to claim `%s' for driver `%s'",
			    ddf_dev_get_name(device), driver->name);
			return ret;
		}
	}


	/* Init hw driver */
	hcd_t *hcd = dev_to_hcd(device);
	ret = driver->init(hcd, &hw_res, irqs_enabled);
	hw_res_list_parsed_clean(&hw_res);
	if (ret != EOK) {
		usb_log_error("Failed to init HCD: %s.\n", str_error(ret));
		goto irq_unregister;
	}

	/* Need working irq replacement to setup root hub */
	if (!irqs_enabled && hcd->ops.status_hook) {
		hcd->polling_fibril = fibril_create(interrupt_polling, hcd);
		if (hcd->polling_fibril == 0) {
			usb_log_error("Failed to create polling fibril\n");
			ret = ENOMEM;
			goto irq_unregister;
		}
		fibril_add_ready(hcd->polling_fibril);
		usb_log_warning("Failed to enable interrupts: %s."
		    " Falling back to polling.\n", str_error(irq_ret));
	}

	/*
	 * Creating root hub registers a new USB device so HC
	 * needs to be ready at this time.
	 */
	ret = hcd_ddf_setup_root_hub(device);
	if (ret != EOK) {
		usb_log_error("Failed to setup HC root hub: %s.\n",
		    str_error(ret));
		driver->fini(dev_to_hcd(device));
irq_unregister:
		/* Unregistering non-existent should be ok */
		unregister_interrupt_handler(device, irq_cap);
		hcd_ddf_clean_hc(device);
		return ret;
	}

	usb_log_info("Controlling new `%s' device `%s'.\n",
	    driver->name, ddf_dev_get_name(device));
	return EOK;
}
/**
 * @}
 */
