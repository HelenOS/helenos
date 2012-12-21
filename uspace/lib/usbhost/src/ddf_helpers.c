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
#include <usb/debug.h>
#include <errno.h>
#include <str_error.h>

#include "ddf_helpers.h"

extern usbhc_iface_t hcd_iface;

typedef struct hc_dev {
	ddf_fun_t *hc_fun;
	list_t devices;
} hc_dev_t;

static hc_dev_t *dev_to_hc_dev(ddf_dev_t *dev)
{
	return ddf_dev_data_get(dev);
}

hcd_t *dev_to_hcd(ddf_dev_t *dev)
{
	hc_dev_t *hc_dev = dev_to_hc_dev(dev);
	if (!hc_dev || !hc_dev->hc_fun) {
		usb_log_error("Invalid OHCI device.\n");
		return NULL;
	}
	return ddf_fun_data_get(hc_dev->hc_fun);
}

typedef struct usb_dev {
	link_t link;
	ddf_fun_t *fun;
	usb_address_t address;
	usb_speed_t speed;
	devman_handle_t handle;
} usb_dev_t;

/** Get USB address assigned to root hub.
 *
 * @param[in] fun Root hub function.
 * @param[out] address Store the address here.
 * @return Error code.
 */
static int rh_get_my_address(ddf_fun_t *fun, usb_address_t *address)
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
static int rh_get_hc_handle(ddf_fun_t *fun, devman_handle_t *handle)
{
	assert(fun);

	if (handle != NULL) {
		usb_dev_t *usb_dev = ddf_fun_data_get(fun);
		*handle = usb_dev->handle;
	}
	return EOK;
}

/** Root hub USB interface */
static usb_iface_t usb_iface = {
	.get_hc_handle = rh_get_hc_handle,
	.get_my_address = rh_get_my_address,
};
/** Standard USB RH options (RH interface) */
static ddf_dev_ops_t usb_ops = {
	.interfaces[USB_DEV_IFACE] = &usb_iface,
};

/** Standard USB HC options (HC interface) */
static ddf_dev_ops_t hc_ops = {
	.interfaces[USBHC_DEV_IFACE] = &hcd_iface,
};

int hcd_ddf_add_usb_device(ddf_dev_t *parent,
    usb_address_t address, usb_speed_t speed, const char *name,
    const match_id_list_t *mids)
{
	assert(parent);
	hc_dev_t *hc_dev = dev_to_hc_dev(parent);
	devman_handle_t hc_handle = ddf_fun_get_handle(hc_dev->hc_fun);

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
	info->handle = hc_handle;
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

/** Announce root hub to the DDF
 *
 * @param[in] instance OHCI driver instance
 * @param[in] hub_fun DDF function representing OHCI root hub
 * @return Error code
 */
int hcd_ddf_setup_hub(hcd_t *instance, usb_address_t *address, ddf_dev_t *device)
{
	assert(instance);
	assert(address);
	assert(device);

	const usb_speed_t speed = instance->dev_manager.max_speed;

	int ret = usb_device_manager_request_address(&instance->dev_manager,
	    address, false, speed);
	if (ret != EOK) {
		usb_log_error("Failed to get root hub address: %s\n",
		    str_error(ret));
		return ret;
	}

#define CHECK_RET_UNREG_RETURN(ret, message...) \
if (ret != EOK) { \
	usb_log_error(message); \
	usb_endpoint_manager_remove_ep( \
	    &instance->ep_manager, *address, 0, \
	    USB_DIRECTION_BOTH, NULL, NULL); \
	usb_device_manager_release_address( \
	    &instance->dev_manager, *address); \
	return ret; \
} else (void)0

	ret = usb_endpoint_manager_add_ep(
	    &instance->ep_manager, *address, 0,
	    USB_DIRECTION_BOTH, USB_TRANSFER_CONTROL, speed, 64,
	    0, NULL, NULL);
	CHECK_RET_UNREG_RETURN(ret,
	    "Failed to add root hub control endpoint: %s.\n", str_error(ret));

	match_id_t mid = { .id = "usb&class=hub", .score = 100 };
	link_initialize(&mid.link);
	match_id_list_t mid_list;
	init_match_ids(&mid_list);
	add_match_id(&mid_list, &mid);

	ret = hcd_ddf_add_usb_device(device, *address, speed, "rh", &mid_list);
	CHECK_RET_UNREG_RETURN(ret,
	    "Failed to add hcd device: %s.\n", str_error(ret));

	return EOK;
#undef CHECK_RET_UNREG_RETURN
}

/** Initialize hc structures.
 *
 * @param[in] device DDF instance of the device to use.
 *
 * This function does all the preparatory work for hc driver implementation.
 *  - gets device hw resources
 *  - disables OHCI legacy support
 *  - asks for interrupt
 *  - registers interrupt handler
 */
int hcd_ddf_setup_device(ddf_dev_t *device, ddf_fun_t **hc_fun,
    usb_speed_t max_speed, size_t bw, bw_count_func_t bw_count)
{
	if (device == NULL)
		return EBADMEM;

	hc_dev_t *instance = ddf_dev_data_alloc(device, sizeof(hc_dev_t));
	if (instance == NULL) {
		usb_log_error("Failed to allocate OHCI driver.\n");
		return ENOMEM;
	}
	list_initialize(&instance->devices);

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
	    "Failed to create OHCI HC function: %s.\n", str_error(ret));
	ddf_fun_set_ops(instance->hc_fun, &hc_ops);
	hcd_t *hcd = ddf_fun_data_alloc(instance->hc_fun, sizeof(hcd_t));
	ret = hcd ? EOK : ENOMEM;
	CHECK_RET_DEST_FREE_RETURN(ret,
	    "Failed to allocate HCD structure: %s.\n", str_error(ret));

	hcd_init(hcd, max_speed, bw, bw_count);

	ret = ddf_fun_bind(instance->hc_fun);
	CHECK_RET_DEST_FREE_RETURN(ret,
	    "Failed to bind OHCI device function: %s.\n", str_error(ret));

#define CHECK_RET_UNBIND_FREE_RETURN(ret, message...) \
if (ret != EOK) { \
	ddf_fun_unbind(instance->hc_fun); \
	CHECK_RET_DEST_FREE_RETURN(ret, \
	    "Failed to add OHCI to HC class: %s.\n", str_error(ret)); \
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
