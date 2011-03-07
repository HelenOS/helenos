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
/** @addtogroup usb
 * @{
 */
/** @file
 * @brief UHCI driver
 */
#include <assert.h>
#include <errno.h>
#include <str_error.h>
#include <stdio.h>
#include <ops/hw_res.h>

#include <usb_iface.h>
#include <usb/debug.h>

#include "root_hub.h"
#include "uhci.h"

/*----------------------------------------------------------------------------*/
/** Gets handle of the respective hc (parent device).
 *
 * @param[in] root_hub_fun Root hub function seeking hc handle.
 * @param[out] handle Place to write the handle.
 * @return Error code.
 */
static int usb_iface_get_hc_handle_rh_impl(
    ddf_fun_t *root_hub_fun, devman_handle_t *handle)
{
	/* TODO: Can't this use parent pointer? */
	ddf_fun_t *hc_fun = root_hub_fun->driver_data;
	assert(hc_fun != NULL);

	*handle = hc_fun->handle;

	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Gets USB address of the calling device.
 *
 * @param[in] fun Root hub function.
 * @param[in] handle Handle of the device seeking address.
 * @param[out] address Place to store found address.
 * @return Error code.
 */
static int usb_iface_get_address_rh_impl(
    ddf_fun_t *fun, devman_handle_t handle, usb_address_t *address)
{
	/* TODO: What does this do? Is it neccessary? Can't it use implemented
	 * hc function?*/
	assert(fun);
	ddf_fun_t *hc_fun = fun->driver_data;
	assert(hc_fun);
	uhci_t *hc = fun_to_uhci(hc_fun);
	assert(hc);

	usb_address_t addr = device_keeper_find(&hc->device_manager, handle);
	if (addr < 0) {
		return addr;
	}

	if (address != NULL) {
		*address = addr;
	}

	return EOK;
}
/*----------------------------------------------------------------------------*/
usb_iface_t usb_iface_root_hub_fun_impl = {
	.get_hc_handle = usb_iface_get_hc_handle_rh_impl,
	.get_address = usb_iface_get_address_rh_impl
};
/*----------------------------------------------------------------------------*/
/** Gets root hub hw resources.
 *
 * @param[in] fun Root hub function.
 * @return Pointer to the resource list used by the root hub.
 */
static hw_resource_list_t *get_resource_list(ddf_fun_t *dev)
{
	assert(dev);
	ddf_fun_t *hc_ddf_instance = dev->driver_data;
	assert(hc_ddf_instance);
	uhci_t *hc = hc_ddf_instance->driver_data;
	assert(hc);

	/* TODO: fix memory leak */
	hw_resource_list_t *resource_list = malloc(sizeof(hw_resource_list_t));
	assert(resource_list);
	resource_list->count = 1;
	resource_list->resources = malloc(sizeof(hw_resource_t));
	assert(resource_list->resources);
	resource_list->resources[0].type = IO_RANGE;
	resource_list->resources[0].res.io_range.address =
	    ((uintptr_t)hc->registers) + 0x10; // see UHCI design guide
	resource_list->resources[0].res.io_range.size = 4;
	resource_list->resources[0].res.io_range.endianness = LITTLE_ENDIAN;

	return resource_list;
}
/*----------------------------------------------------------------------------*/
static hw_res_ops_t hw_res_iface = {
	.get_resource_list = get_resource_list,
	.enable_interrupt = NULL
};
/*----------------------------------------------------------------------------*/
static ddf_dev_ops_t root_hub_ops = {
	.interfaces[USB_DEV_IFACE] = &usb_iface_root_hub_fun_impl,
	.interfaces[HW_RES_DEV_IFACE] = &hw_res_iface
};
/*----------------------------------------------------------------------------*/
int setup_root_hub(ddf_fun_t **fun, ddf_dev_t *hc)
{
	assert(fun);
	assert(hc);
	int ret;

	ddf_fun_t *hub = ddf_fun_create(hc, fun_inner, "root-hub");
	if (!hub) {
		usb_log_error("Failed to create root hub device structure.\n");
		return ENOMEM;
	}

	char *match_str;
	ret = asprintf(&match_str, "usb&uhci&root-hub");
	if (ret < 0) {
		usb_log_error("Failed to create root hub match string.\n");
		ddf_fun_destroy(hub);
		return ENOMEM;
	}

	ret = ddf_fun_add_match_id(hub, match_str, 100);
	if (ret != EOK) {
		usb_log_error("Failed(%d) to add root hub match id: %s\n",
		    ret, str_error(ret));
		ddf_fun_destroy(hub);
		return ret;
	}

	hub->ops = &root_hub_ops;

	*fun = hub;
	return EOK;
}
/**
 * @}
 */
