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
#include <usb/ddfiface.h>
#include <usb/dev/pipes.h>
#include <usb/classes/classes.h>
#include <usb/dev/recognise.h>
#include "usbmid.h"

/** Callback for DDF USB interface. */
static int usb_iface_get_interface_impl(ddf_fun_t *fun, int *iface_no)
{
	usbmid_interface_t *iface = ddf_fun_data_get(fun);
	assert(iface);

	if (iface_no != NULL) {
		*iface_no = iface->interface_no;
	}

	return EOK;
}

/** DDF interface of the child - interface function. */
static usb_iface_t child_usb_iface = {
	.get_hc_handle = usb_iface_get_hc_handle_device_impl,
	.get_my_address = usb_iface_get_my_address_forward_impl,
	.get_my_interface = usb_iface_get_interface_impl,
};

/** Operations for children - interface functions. */
static ddf_dev_ops_t child_device_ops = {
	.interfaces[USB_DEV_IFACE] = &child_usb_iface
};

int usbmid_interface_destroy(usbmid_interface_t *mid_iface)
{
	assert(mid_iface);
	assert_link_not_used(&mid_iface->link);
	const int ret = ddf_fun_unbind(mid_iface->fun);
	if (ret != EOK) {
		return ret;
	}
	/* NOTE: usbmid->interface points somewhere, but we did not
	 * allocate that space, so don't touch */
	ddf_fun_destroy(mid_iface->fun);
	/* NOTE: mid_iface is invalid at this point, it was assigned to
	 * mid_iface->fun->driver_data and freed in ddf_fun_destroy */
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
int usbmid_spawn_interface_child(usb_device_t *parent,
    usbmid_interface_t *iface,
    const usb_standard_device_descriptor_t *device_descriptor,
    const usb_standard_interface_descriptor_t *interface_descriptor)
{
	char *child_name = NULL;
	int rc;

	/*
	 * Name is class name followed by interface number.
	 * The interface number shall provide uniqueness while the
	 * class name something humanly understandable.
	 */
	rc = asprintf(&child_name, "%s%hhu",
	    usb_str_class(interface_descriptor->interface_class),
	    interface_descriptor->interface_number);
	if (rc < 0)
		return ENOMEM;

	rc = ddf_fun_set_name(iface->fun, child_name);
	free(child_name);
	if (rc != EOK)
		return ENOMEM;

	match_id_list_t match_ids;
	init_match_ids(&match_ids);

	rc = usb_device_create_match_ids_from_interface(device_descriptor,
	    interface_descriptor, &match_ids);
	if (rc != EOK)
		return rc;

	list_foreach(match_ids.ids, link, match_id_t, match_id) {
		rc = ddf_fun_add_match_id(iface->fun, match_id->id, match_id->score);
		if (rc != EOK) {
			clean_match_ids(&match_ids);
			return rc;
		}
	}
	clean_match_ids(&match_ids);

	ddf_fun_set_ops(iface->fun, &child_device_ops);

	rc = ddf_fun_bind(iface->fun);
	if (rc != EOK)
		return rc;

	return EOK;
}

/**
 * @}
 */
