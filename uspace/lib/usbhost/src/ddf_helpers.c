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

#include <usb_iface.h>
#include <usb/classes/classes.h>
#include <usb/debug.h>
#include <usb/descriptor.h>
#include <usb/request.h>
#include <errno.h>
#include <str_error.h>

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


static int hcd_ddf_new_device(ddf_dev_t *device, usb_dev_t *hub, unsigned port);
static int hcd_ddf_remove_device(ddf_dev_t *device, usb_dev_t *hub, unsigned port);


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
static int register_endpoint(
    ddf_fun_t *fun, usb_endpoint_t endpoint,
    usb_transfer_type_t transfer_type, usb_direction_t direction,
    size_t max_packet_size, unsigned interval)
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
	    max_packet_size, size);
}

/** Unregister endpoint interface function.
 * @param fun DDF function.
 * @param address USB address of the endpoint.
 * @param endpoint USB endpoint number.
 * @param direction Communication direction of the enpdoint to unregister.
 * @return Error code.
 */
static int unregister_endpoint(
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

static int reserve_default_address(ddf_fun_t *fun, usb_speed_t speed)
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

static int release_default_address(ddf_fun_t *fun)
{
	assert(fun);
	hcd_t *hcd = dev_to_hcd(ddf_fun_get_dev(fun));
	usb_dev_t *dev = ddf_fun_data_get(fun);
	assert(hcd);
	assert(dev);

	usb_log_debug("Device %d released default address\n", dev->address);
	return hcd_release_default_address(hcd);
}

static int device_enumerate(ddf_fun_t *fun, unsigned port)
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

static int device_remove(ddf_fun_t *fun, unsigned port)
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
static int dev_read(ddf_fun_t *fun, usb_endpoint_t endpoint,
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
static int dev_write(ddf_fun_t *fun, usb_endpoint_t endpoint,
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

static int hcd_ddf_add_device(ddf_dev_t *parent, usb_dev_t *hub_dev,
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

	if (hub_dev->speed == USB_SPEED_HIGH && usb_speed_is_11(speed))
		info->tt_address = hub_dev->address;

	ddf_fun_set_ops(fun, &usb_ops);
	list_foreach(mids->ids, iter) {
		match_id_t *mid = list_get_instance(iter, match_id_t, link);
		ddf_fun_add_match_id(fun, mid->id, mid->score);
	}

	int ret = ddf_fun_bind(fun);
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

static int hcd_ddf_remove_device(ddf_dev_t *device, usb_dev_t *hub,
    unsigned port)
{
	assert(device);

	hcd_t *hcd = dev_to_hcd(device);
	assert(hcd);

	hc_dev_t *hc_dev = dev_to_hc_dev(device);
	assert(hc_dev);

	fibril_mutex_lock(&hub->guard);

	usb_dev_t *victim = NULL;

	list_foreach(hub->devices, it) {
		victim = list_get_instance(it, usb_dev_t, link);
		if (victim->port == port)
			break;
	}
	if (victim && victim->port == port) {
		list_remove(&victim->link);
		fibril_mutex_unlock(&hub->guard);
		const int ret = ddf_fun_unbind(victim->fun);
		if (ret == EOK) {
			ddf_fun_destroy(victim->fun);
			hcd_release_address(hcd, victim->address);
		} else {
			usb_log_warning("Failed to unbind device `%s': %s\n",
			    ddf_fun_get_name(victim->fun), str_error(ret));
		}
		return EOK;
	}
	return ENOENT;
}

static int hcd_ddf_new_device(ddf_dev_t *device, usb_dev_t *hub, unsigned port)
{
	assert(device);

	hcd_t *hcd = dev_to_hcd(device);
	assert(hcd);

	usb_speed_t speed = USB_SPEED_MAX;

	/* This checks whether the default address is reserved and gets speed */
	int ret = usb_endpoint_manager_get_info_by_address(&hcd->ep_manager,
		USB_ADDRESS_DEFAULT, &speed);
	if (ret != EOK) {
		return ret;
	}

	static const usb_target_t default_target = {{
		.address = USB_ADDRESS_DEFAULT,
		.endpoint = 0,
	}};

	const usb_address_t address = hcd_request_address(hcd, speed);
	if (address < 0)
		return address;

	const usb_target_t target = {{
		.address = address,
		.endpoint = 0,
	}};

	/* Add default pipe on default address */
	ret = hcd_add_ep(hcd,
	    default_target, USB_DIRECTION_BOTH, USB_TRANSFER_CONTROL,
	    CTRL_PIPE_MIN_PACKET_SIZE, CTRL_PIPE_MIN_PACKET_SIZE);

	if (ret != EOK) {
		hcd_release_address(hcd, address);
		return ret;
	}

	/* Get max packet size for default pipe */
	usb_standard_device_descriptor_t desc = { 0 };
	static const usb_device_request_setup_packet_t get_device_desc_8 =
	    GET_DEVICE_DESC(CTRL_PIPE_MIN_PACKET_SIZE);

	// TODO CALLBACKS
	ssize_t got = hcd_send_batch_sync(hcd, default_target, USB_DIRECTION_IN,
	    &desc, CTRL_PIPE_MIN_PACKET_SIZE, *(uint64_t *)&get_device_desc_8,
	    "read first 8 bytes of dev descriptor");

	if (got != CTRL_PIPE_MIN_PACKET_SIZE) {
		hcd_remove_ep(hcd, default_target, USB_DIRECTION_BOTH);
		hcd_release_address(hcd, address);
		return got < 0 ? got : EOVERFLOW;
	}

	/* Register EP on the new address */
	ret = hcd_add_ep(hcd, target, USB_DIRECTION_BOTH, USB_TRANSFER_CONTROL,
	    desc.max_packet_size, desc.max_packet_size);
	if (ret != EOK) {
		hcd_remove_ep(hcd, default_target, USB_DIRECTION_BOTH);
		hcd_remove_ep(hcd, target, USB_DIRECTION_BOTH);
		hcd_release_address(hcd, address);
		return ret;
	}

	/* Set new address */
	const usb_device_request_setup_packet_t set_address =
	    SET_ADDRESS(target.address);

	got = hcd_send_batch_sync(hcd, default_target, USB_DIRECTION_OUT,
	    NULL, 0, *(uint64_t *)&set_address, "set address");

	hcd_remove_ep(hcd, default_target, USB_DIRECTION_BOTH);

	if (got != 0) {
		hcd_remove_ep(hcd, target, USB_DIRECTION_BOTH);
		hcd_release_address(hcd, address);
		return got;
	}
	
	/* Get std device descriptor */
	static const usb_device_request_setup_packet_t get_device_desc =
	    GET_DEVICE_DESC(sizeof(desc));

	got = hcd_send_batch_sync(hcd, target, USB_DIRECTION_IN,
	    &desc, sizeof(desc), *(uint64_t *)&get_device_desc,
	    "read device descriptor");
	if (ret != EOK) {
		hcd_remove_ep(hcd, target, USB_DIRECTION_BOTH);
		hcd_release_address(hcd, target.address);
		return got < 0 ? got : EOVERFLOW;
	}

	/* Create match ids from the device descriptor */
	match_id_list_t mids;
	init_match_ids(&mids);

	ret = create_match_ids(&mids, &desc);
	if (ret != EOK) {
		hcd_remove_ep(hcd, target, USB_DIRECTION_BOTH);
		hcd_release_address(hcd, target.address);
		return ret;
	}

	/* Register device */
	ret = hcd_ddf_add_device(device, hub, port, address, speed, NULL, &mids);
	clean_match_ids(&mids);
	if (ret != EOK) {
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
int hcd_ddf_setup_root_hub(ddf_dev_t *device)
{
	assert(device);
	hcd_t *hcd = dev_to_hcd(device);
	assert(hcd);

	const usb_speed_t speed = hcd->ep_manager.max_speed;

	hcd_reserve_default_address(hcd, speed);
	const int ret = hcd_ddf_new_device(device, NULL, 0);
	hcd_release_default_address(hcd);
	return ret;
}

/** Initialize hc structures.
 *
 * @param[in] device DDF instance of the device to use.
 *
 * This function does all the ddf work for hc driver.
 */
int hcd_ddf_setup_hc(ddf_dev_t *device, usb_speed_t max_speed,
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

void hcd_ddf_clean_hc(ddf_dev_t *device)
{
	assert(device);
	hc_dev_t *hc = dev_to_hc_dev(device);
	assert(hc);
	const int ret = ddf_fun_unbind(hc->ctl_fun);
	if (ret == EOK)
		ddf_fun_destroy(hc->ctl_fun);
}
/**
 * @}
 */
