/*
 * Copyright (c) 2012 Jan Vesely
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

extern usbhc_iface_t hcd_iface;

typedef struct hc_dev {
	ddf_fun_t *hc_fun;
	list_t devices;
	fibril_mutex_t guard;
} hc_dev_t;

static hc_dev_t *dev_to_hc_dev(ddf_dev_t *dev)
{
	return ddf_dev_data_get(dev);
}

hcd_t *dev_to_hcd(ddf_dev_t *dev)
{
	hc_dev_t *hc_dev = dev_to_hc_dev(dev);
	if (!hc_dev || !hc_dev->hc_fun) {
		usb_log_error("Invalid HCD device.\n");
		return NULL;
	}
	return ddf_fun_data_get(hc_dev->hc_fun);
}

typedef struct usb_dev {
	link_t link;
	ddf_fun_t *fun;
	usb_address_t address;
	usb_speed_t speed;
	devman_handle_t hc_handle;
} usb_dev_t;

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

static int device_enumerate(ddf_fun_t *fun, usb_device_handle_t *handle)
{
	assert(fun);
	ddf_dev_t *ddf_dev = ddf_fun_get_dev(fun);
	usb_dev_t *dev = ddf_fun_data_get(fun);
	assert(ddf_dev);
	assert(dev);
	usb_address_t address;
	usb_log_debug("Device %d reported a new USB device\n", dev->address);
	const int ret = hcd_ddf_new_device(ddf_dev, &address);
	if (ret == EOK && handle)
		*handle = address;
	return ret;
}

static int device_remove(ddf_fun_t *fun, usb_device_handle_t handle)
{
	assert(fun);
	ddf_dev_t *ddf_dev = ddf_fun_get_dev(fun);
	usb_dev_t *dev = ddf_fun_data_get(fun);
	assert(ddf_dev);
	assert(dev);
	usb_log_debug("Device %d reported removal of device %d\n",
	    dev->address, (int)handle);
	return hcd_ddf_remove_device(ddf_dev, (usb_address_t)handle);
}

/** Get USB address assigned to root hub.
 *
 * @param[in] fun Root hub function.
 * @param[out] address Store the address here.
 * @return Error code.
 */
static int get_my_address(ddf_fun_t *fun, usb_address_t *address)
{
	assert(fun);
	if (address != NULL) {
		usb_dev_t *usb_dev = ddf_fun_data_get(fun);
		*address = usb_dev->address;
	}
	return EOK;
}

/** Gets handle of the respective hc (this device, hc function).
 *
 * @param[in] root_hub_fun Root hub function seeking hc handle.
 * @param[out] handle Place to write the handle.
 * @return Error code.
 */
static int get_hc_handle(ddf_fun_t *fun, devman_handle_t *handle)
{
	assert(fun);

	if (handle != NULL) {
		usb_dev_t *usb_dev = ddf_fun_data_get(fun);
		*handle = usb_dev->hc_handle;
	}
	return EOK;
}

/** Root hub USB interface */
static usb_iface_t usb_iface = {
	.get_hc_handle = get_hc_handle,
	.get_my_address = get_my_address,

	.reserve_default_address = reserve_default_address,
	.release_default_address = release_default_address,
	.device_enumerate = device_enumerate,
	.device_remove = device_remove,
};
/** Standard USB RH options (RH interface) */
static ddf_dev_ops_t usb_ops = {
	.interfaces[USB_DEV_IFACE] = &usb_iface,
};

/** Standard USB HC options (HC interface) */
static ddf_dev_ops_t hc_ops = {
	.interfaces[USBHC_DEV_IFACE] = &hcd_iface,
};

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

int hcd_ddf_add_usb_device(ddf_dev_t *parent,
    usb_address_t address, usb_speed_t speed, const char *name,
    const match_id_list_t *mids)
{
	assert(parent);
	hc_dev_t *hc_dev = dev_to_hc_dev(parent);
	devman_handle_t hc_handle = ddf_fun_get_handle(hc_dev->hc_fun);

	char default_name[10] = { 0 }; /* usbxyz-ss */
	if (!name) {
		snprintf(default_name, sizeof(default_name) - 1,
		    "usb%u-%cs", address, usb_str_speed(speed)[0]);
		name = default_name;
	}

	//TODO more checks
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
	info->hc_handle = hc_handle;
	info->fun = fun;
	link_initialize(&info->link);

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

	ret = usb_device_manager_bind_address(&dev_to_hcd(parent)->dev_manager,
	    address, ddf_fun_get_handle(fun));
	if (ret != EOK)
		usb_log_warning("Failed to bind address: %s.\n",
		    str_error(ret));

	list_append(&info->link, &hc_dev->devices);
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

int hcd_ddf_remove_device(ddf_dev_t *device, usb_address_t id)
{
	assert(device);

	hcd_t *hcd = dev_to_hcd(device);
	assert(hcd);

	hc_dev_t *hc_dev = dev_to_hc_dev(device);
	assert(hc_dev);

	fibril_mutex_lock(&hc_dev->guard);

	usb_dev_t *victim = NULL;

	list_foreach(hc_dev->devices, it) {
		victim = list_get_instance(it, usb_dev_t, link);
		if (victim->address == id)
			break;
	}
	if (victim && victim->address == id) {
		list_remove(&victim->link);
		fibril_mutex_unlock(&hc_dev->guard);
		const int ret = ddf_fun_unbind(victim->fun);
		if (ret == EOK) {
			ddf_fun_destroy(victim->fun);
			hcd_release_address(hcd, id);
		} else {
			usb_log_warning("Failed to unbind device %d: %s\n",
			    id, str_error(ret));
		}
		return EOK;
	}
	return ENOENT;
}

int hcd_ddf_new_device(ddf_dev_t *device, usb_address_t *id)
{
	assert(device);

	hcd_t *hcd = dev_to_hcd(device);
	assert(hcd);

	usb_speed_t speed = USB_SPEED_MAX;

	/* This checks whether the default address is reserved and gets speed */
	int ret = usb_device_manager_get_info_by_address(&hcd->dev_manager,
		USB_ADDRESS_DEFAULT, NULL, &speed);
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
	ret = hcd_ddf_add_usb_device(device, address, speed, NULL, &mids);
	clean_match_ids(&mids);
	if (ret != EOK) {
		hcd_remove_ep(hcd, target, USB_DIRECTION_BOTH);
		hcd_release_address(hcd, target.address);
		return ret;
	}
	if (ret == EOK && id)
		*id = target.address;

	return ret;
}

/** Announce root hub to the DDF
 *
 * @param[in] device Host controller ddf device
 * @param[in] speed roothub communication speed
 * @return Error code
 */
int hcd_ddf_setup_root_hub(ddf_dev_t *device, usb_speed_t speed)
{
	assert(device);
	hcd_t *hcd = dev_to_hcd(device);
	assert(hcd);

	hcd_reserve_default_address(hcd, speed);
	const int ret = hcd_ddf_new_device(device, NULL);
	hcd_release_default_address(hcd);
	return ret;
}

/** Initialize hc structures.
 *
 * @param[in] device DDF instance of the device to use.
 *
 * This function does all the ddf work for hc driver.
 */
int hcd_ddf_setup_device(ddf_dev_t *device, ddf_fun_t **hc_fun,
    usb_speed_t max_speed, size_t bw, bw_count_func_t bw_count)
{
	if (device == NULL)
		return EBADMEM;

	hc_dev_t *instance = ddf_dev_data_alloc(device, sizeof(hc_dev_t));
	if (instance == NULL) {
		usb_log_error("Failed to allocate HCD ddf structure.\n");
		return ENOMEM;
	}
	list_initialize(&instance->devices);
	fibril_mutex_initialize(&instance->guard);

#define CHECK_RET_DEST_FREE_RETURN(ret, message...) \
if (ret != EOK) { \
	if (instance->hc_fun) { \
		ddf_fun_destroy(instance->hc_fun); \
	} \
	usb_log_error(message); \
	return ret; \
} else (void)0

	instance->hc_fun = ddf_fun_create(device, fun_exposed, "hc");
	int ret = instance->hc_fun ? EOK : ENOMEM;
	CHECK_RET_DEST_FREE_RETURN(ret,
	    "Failed to create HCD HC function: %s.\n", str_error(ret));
	ddf_fun_set_ops(instance->hc_fun, &hc_ops);
	hcd_t *hcd = ddf_fun_data_alloc(instance->hc_fun, sizeof(hcd_t));
	ret = hcd ? EOK : ENOMEM;
	CHECK_RET_DEST_FREE_RETURN(ret,
	    "Failed to allocate HCD structure: %s.\n", str_error(ret));

	hcd_init(hcd, max_speed, bw, bw_count);

	ret = ddf_fun_bind(instance->hc_fun);
	CHECK_RET_DEST_FREE_RETURN(ret,
	    "Failed to bind HCD device function: %s.\n", str_error(ret));

#define CHECK_RET_UNBIND_FREE_RETURN(ret, message...) \
if (ret != EOK) { \
	ddf_fun_unbind(instance->hc_fun); \
	CHECK_RET_DEST_FREE_RETURN(ret, message); \
} else (void)0
	ret = ddf_fun_add_to_category(instance->hc_fun, USB_HC_CATEGORY);
	CHECK_RET_UNBIND_FREE_RETURN(ret,
	    "Failed to add hc to category: %s\n", str_error(ret));

	/* HC should be ok at this point (except it can't do anything) */
	if (hc_fun)
		*hc_fun = instance->hc_fun;

	return EOK;
}

/**
 * @}
 */
