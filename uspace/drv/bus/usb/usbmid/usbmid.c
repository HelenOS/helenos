/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

/**
 * Get USB device description by calling HC and altering the interface field.
 *
 * @param[in] fun Device function the operation is running on.
 * @param[out] desc Device descriptor.
 * @return Error code.
 */
static errno_t usb_iface_description(ddf_fun_t *fun, usb_device_desc_t *desc)
{
	usbmid_interface_t *iface = ddf_fun_data_get(fun);
	assert(iface);
	usb_device_t *usb_dev = ddf_dev_data_get(ddf_fun_get_dev(fun));
	assert(usb_dev);

	async_exch_t *exch = usb_device_bus_exchange_begin(usb_dev);
	if (!exch)
		return EPARTY;

	usb_device_desc_t tmp_desc;
	const errno_t ret = usb_get_my_description(exch, &tmp_desc);

	if (ret == EOK && desc) {
		*desc = tmp_desc;
		desc->iface = iface->interface_no;
	}

	usb_device_bus_exchange_end(exch);

	return EOK;
}

/** DDF interface of the child - USB functions. */
static usb_iface_t child_usb_iface = {
	.get_my_description = usb_iface_description,
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
	errno_t ret = asprintf(&child_name, "%s%hhu",
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
