/*
 * Copyright (c) 2011 Vojtech Horky
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

/** @addtogroup drvusbmid
 * @{
 */

/**
 * @file
 * Helper functions.
 */

#include <errno.h>
#include <str_error.h>
#include <stdlib.h>
#include <usb_iface.h>
#include <usb/dev/pipes.h>
#include <usb/classes/classes.h>
#include <usb/dev/recognise.h>
#include "usbmid.h"

/** Get USB device handle by calling the parent usb_device_t.
 *
 * @param[in] fun Device function the operation is running on.
 * @param[out] handle Device handle.
 * @return Error code.
 */
static errno_t usb_iface_device_handle(ddf_fun_t *fun, devman_handle_t *handle)
{
	assert(fun);
	assert(handle);
	usb_device_t *usb_dev = usb_device_get(ddf_fun_get_dev(fun));
	*handle = usb_device_get_devman_handle(usb_dev);
	return EOK;
}

/** Callback for DDF USB get interface. */
static errno_t usb_iface_iface_no(ddf_fun_t *fun, int *iface_no)
{
	usbmid_interface_t *iface = ddf_fun_data_get(fun);
	assert(iface);

	if (iface_no)
		*iface_no = iface->interface_no;

	return EOK;
}

/** DDF interface of the child - USB functions. */
static usb_iface_t child_usb_iface = {
	.get_my_device_handle = usb_iface_device_handle,
	.get_my_interface = usb_iface_iface_no,
};

/** Operations for children - interface functions. */
static ddf_dev_ops_t child_device_ops = {
	.interfaces[USB_DEV_IFACE] = &child_usb_iface
};

errno_t usbmid_interface_destroy(usbmid_interface_t *mid_iface)
{
	assert(mid_iface);
	assert_link_not_used(&mid_iface->link);
	const errno_t ret = ddf_fun_unbind(mid_iface->fun);
	if (ret != EOK) {
		return ret;
	}
	ddf_fun_destroy(mid_iface->fun);
	return EOK;
}

/** Spawn new child device from one interface.
 *
 * @param parent Parent MID device.
 * @param iface Interface information.
 * @param device_descriptor Device descriptor.
 * @param interface_descriptor Interface descriptor.
 * @return Error code.
 */
errno_t usbmid_spawn_interface_child(usb_device_t *parent,
    usbmid_interface_t **iface_ret,
    const usb_standard_device_descriptor_t *device_descriptor,
    const usb_standard_interface_descriptor_t *interface_descriptor)
{
	ddf_fun_t *child = NULL;
	char *child_name = NULL;
	errno_t rc;

	/*
	 * Name is class name followed by interface number.
	 * The interface number shall provide uniqueness while the
	 * class name something humanly understandable.
	 */
	int ret = asprintf(&child_name, "%s%hhu",
	    usb_str_class(interface_descriptor->interface_class),
	    interface_descriptor->interface_number);
	if (ret < 0) {
		return ENOMEM;
	}

	/* Create the device. */
	child = usb_device_ddf_fun_create(parent, fun_inner, child_name);
	free(child_name);
	if (child == NULL) {
		return ENOMEM;
	}

	match_id_list_t match_ids;
	init_match_ids(&match_ids);

	rc = usb_device_create_match_ids_from_interface(device_descriptor,
	    interface_descriptor, &match_ids);
	if (rc != EOK) {
		ddf_fun_destroy(child);
		return rc;
	}

	list_foreach(match_ids.ids, link, match_id_t, match_id) {
		rc = ddf_fun_add_match_id(child, match_id->id, match_id->score);
		if (rc != EOK) {
			clean_match_ids(&match_ids);
			ddf_fun_destroy(child);
			return rc;
		}
	}
	clean_match_ids(&match_ids);
	ddf_fun_set_ops(child, &child_device_ops);

	usbmid_interface_t *iface = ddf_fun_data_alloc(child, sizeof(*iface));

	iface->fun = child;
	iface->interface_no = interface_descriptor->interface_number;
	link_initialize(&iface->link);

	rc = ddf_fun_bind(child);
	if (rc != EOK) {
		/* This takes care of match_id deallocation as well. */
		ddf_fun_destroy(child);
		return rc;
	}
	*iface_ret = iface;

	return EOK;
}

/**
 * @}
 */
