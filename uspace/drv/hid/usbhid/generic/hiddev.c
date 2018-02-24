/*
 * Copyright (c) 2011 Lubos Slovak
 * Copyright (c) 2018 Ondrej Hlavaty
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

/** @addtogroup drvusbhid
 * @{
 */
/**
 * @file
 * USB HID driver API.
 */

#include <usb/debug.h>
#include <usb/classes/classes.h>
#include <errno.h>
#include <str_error.h>
#include <stdbool.h>

#include <usbhid_iface.h>

#include "hiddev.h"
#include "usbhid.h"

const usb_endpoint_description_t usb_hid_generic_poll_endpoint_description = {
	.transfer_type = USB_TRANSFER_INTERRUPT,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_HID,
	.interface_subclass = -1,
	.interface_protocol = -1,
	.flags = 0
};

const char *HID_GENERIC_FUN_NAME = "hid";
const char *HID_GENERIC_CATEGORY = "hid";


static size_t usb_generic_hid_get_event_length(ddf_fun_t *fun);
static errno_t usb_generic_hid_get_event(ddf_fun_t *fun, uint8_t *buffer,
    size_t size, size_t *act_size, int *event_nr, unsigned int flags);
static errno_t usb_generic_hid_client_connected(ddf_fun_t *fun);
static size_t usb_generic_get_report_descriptor_length(ddf_fun_t *fun);
static errno_t usb_generic_get_report_descriptor(ddf_fun_t *fun, uint8_t *desc,
    size_t size, size_t *actual_size);

static usbhid_iface_t usb_generic_iface = {
	.get_event = usb_generic_hid_get_event,
	.get_event_length = usb_generic_hid_get_event_length,
	.get_report_descriptor_length = usb_generic_get_report_descriptor_length,
	.get_report_descriptor = usb_generic_get_report_descriptor
};

static ddf_dev_ops_t usb_generic_hid_ops = {
	.interfaces[USBHID_DEV_IFACE] = &usb_generic_iface,
	.open = usb_generic_hid_client_connected
};

/** Return hid_dev_t * for generic HID function node.
 *
 * For the generic HID subdriver the 'hid' function has usb_hid_gen_fun_t
 * as soft state. Through that we can get to the usb_hid_dev_t.
 */
static usb_hid_dev_t *fun_hid_dev(ddf_fun_t *fun)
{
	return ((usb_hid_gen_fun_t *)ddf_fun_data_get(fun))->hid_dev;
}

static size_t usb_generic_hid_get_event_length(ddf_fun_t *fun)
{
	usb_log_debug2("Generic HID: Get event length (fun: %p, "
	    "fun->driver_data: %p.\n", fun, ddf_fun_data_get(fun));

	const usb_hid_dev_t *hid_dev = fun_hid_dev(fun);

	usb_log_debug2("hid_dev: %p, Max input report size (%zu).",
	    hid_dev, hid_dev->max_input_report_size);

	return hid_dev->max_input_report_size;
}

static errno_t usb_generic_hid_get_event(ddf_fun_t *fun, uint8_t *buffer,
    size_t size, size_t *act_size, int *event_nr, unsigned int flags)
{
	usb_log_debug2("Generic HID: Get event.");

	if (buffer == NULL || act_size == NULL || event_nr == NULL) {
		usb_log_debug("No function");
		return EINVAL;
	}

	const usb_hid_dev_t *hid_dev = fun_hid_dev(fun);

	if (hid_dev->input_report_size > size) {
		usb_log_debug("input_report_size > size (%zu, %zu)",
		    hid_dev->input_report_size, size);
		return EINVAL;	// TODO: other error code
	}

	/*! @todo This should probably be somehow atomic. */
	memcpy(buffer, hid_dev->input_report,
	    hid_dev->input_report_size);
	*act_size = hid_dev->input_report_size;
	*event_nr = usb_hid_report_number(hid_dev);

	usb_log_debug2("OK");

	return EOK;
}

static size_t usb_generic_get_report_descriptor_length(ddf_fun_t *fun)
{
	usb_log_debug("Generic HID: Get report descriptor length.");

	const usb_hid_dev_t *hid_dev = fun_hid_dev(fun);

	usb_log_debug2("hid_dev->report_desc_size = %zu",
	    hid_dev->report_desc_size);

	return hid_dev->report_desc_size;
}

static errno_t usb_generic_get_report_descriptor(ddf_fun_t *fun, uint8_t *desc,
    size_t size, size_t *actual_size)
{
	usb_log_debug2("Generic HID: Get report descriptor.");

	const usb_hid_dev_t *hid_dev = fun_hid_dev(fun);

	if (hid_dev->report_desc_size > size) {
		return EINVAL;
	}

	memcpy(desc, hid_dev->report_desc, hid_dev->report_desc_size);
	*actual_size = hid_dev->report_desc_size;

	return EOK;
}

static errno_t usb_generic_hid_client_connected(ddf_fun_t *fun)
{
	usb_log_debug("Generic HID: Client connected.");
	return EOK;
}

void usb_generic_hid_deinit(usb_hid_dev_t *hid_dev, void *data)
{
	ddf_fun_t *fun = data;
	if (fun == NULL)
		return;

	if (ddf_fun_unbind(fun) != EOK) {
		usb_log_error("Failed to unbind generic hid fun.");
		return;
	}
	usb_log_debug2("%s unbound.", ddf_fun_get_name(fun));
	ddf_fun_destroy(fun);
}

errno_t usb_generic_hid_init(usb_hid_dev_t *hid_dev, void **data)
{
	usb_hid_gen_fun_t *hid_fun;

	if (hid_dev == NULL) {
		return EINVAL;
	}

	/* Create the exposed function. */
	usb_log_debug("Creating DDF function %s...", HID_GENERIC_FUN_NAME);
	ddf_fun_t *fun = usb_device_ddf_fun_create(hid_dev->usb_dev,
	    fun_exposed, HID_GENERIC_FUN_NAME);
	if (fun == NULL) {
		usb_log_error("Could not create DDF function node.");
		return ENOMEM;
	}

	/* Create softstate */
	hid_fun = ddf_fun_data_alloc(fun, sizeof(usb_hid_gen_fun_t));
	hid_fun->hid_dev = hid_dev;
	ddf_fun_set_ops(fun, &usb_generic_hid_ops);

	errno_t rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		usb_log_error("Could not bind DDF function: %s.",
		    str_error(rc));
		ddf_fun_destroy(fun);
		return rc;
	}

	usb_log_debug("HID function created. Handle: %" PRIun "",
	    ddf_fun_get_handle(fun));
	*data = fun;

	return EOK;
}

bool usb_generic_hid_polling_callback(usb_hid_dev_t *hid_dev, void *data)
{
	/* Continue polling until the device is about to be removed. */
	return true;
}

/**
 * @}
 */
