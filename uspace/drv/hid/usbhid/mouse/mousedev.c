/*
 * Copyright (c) 2011 Lubos Slovak, Vojtech Horky
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
 * USB Mouse driver API.
 */

#include <usb/debug.h>
#include <usb/classes/classes.h>
#include <usb/hid/hid.h>
#include <usb/hid/request.h>
#include <usb/hid/usages/core.h>
#include <errno.h>
#include <async.h>
#include <stdbool.h>
#include <str_error.h>
#include <ipc/mouseev.h>

#include <ipc/kbdev.h>
#include <io/keycode.h>

#include "mousedev.h"
#include "../usbhid.h"

#define NAME "mouse"

static void default_connection_handler(ddf_fun_t *, ipc_call_t *);

static ddf_dev_ops_t ops = { .default_handler = default_connection_handler };

const usb_endpoint_description_t usb_hid_mouse_poll_endpoint_description = {
	.transfer_type = USB_TRANSFER_INTERRUPT,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_HID,
	.interface_subclass = USB_HID_SUBCLASS_BOOT,
	.interface_protocol = USB_HID_PROTOCOL_MOUSE,
	.flags = 0
};

const char *HID_MOUSE_FUN_NAME = "mouse";
const char *HID_MOUSE_CATEGORY = "mouse";

/** Default idle rate for mouses. */
static const uint8_t IDLE_RATE = 0;

static const uint8_t USB_MOUSE_BOOT_REPORT_DESCRIPTOR[] = {
	0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
	0x09, 0x02,                    // USAGE (Mouse)
	0xa1, 0x01,                    // COLLECTION (Application)
	0x09, 0x01,                    //   USAGE (Pointer)
	0xa1, 0x00,                    //   COLLECTION (Physical)
	0x95, 0x03,                    //     REPORT_COUNT (3)
	0x75, 0x01,                    //     REPORT_SIZE (1)
	0x05, 0x09,                    //     USAGE_PAGE (Button)
	0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
	0x29, 0x03,                    //     USAGE_MAXIMUM (Button 3)
	0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
	0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
	0x81, 0x02,                    //     INPUT (Data,Var,Abs)
	0x95, 0x01,                    //     REPORT_COUNT (1)
	0x75, 0x05,                    //     REPORT_SIZE (5)
	0x81, 0x01,                    //     INPUT (Cnst)
	0x75, 0x08,                    //     REPORT_SIZE (8)
	0x95, 0x02,                    //     REPORT_COUNT (2)
	0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
	0x09, 0x30,                    //     USAGE (X)
	0x09, 0x31,                    //     USAGE (Y)
	0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
	0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
	0x81, 0x06,                    //     INPUT (Data,Var,Rel)
	0xc0,                          //   END_COLLECTION
	0xc0                           // END_COLLECTION
};

/** Default handler for IPC methods not handled by DDF.
 *
 * @param fun   Device function handling the call.
 * @param icall Call data.
 *
 */
static void default_connection_handler(ddf_fun_t *fun, ipc_call_t *icall)
{
	usb_mouse_t *mouse_dev = ddf_fun_data_get(fun);

	if (mouse_dev == NULL) {
		usb_log_debug("%s: Missing parameters.", __FUNCTION__);
		async_answer_0(icall, EINVAL);
		return;
	}

	usb_log_debug("%s: fun->name: %s", __FUNCTION__, ddf_fun_get_name(fun));
	usb_log_debug("%s: mouse_sess: %p",
	    __FUNCTION__, mouse_dev->mouse_sess);

	async_sess_t *sess =
	    async_callback_receive_start(EXCHANGE_SERIALIZE, icall);
	if (sess != NULL) {
		if (mouse_dev->mouse_sess == NULL) {
			mouse_dev->mouse_sess = sess;
			usb_log_debug("Console session to %s set ok (%p).",
			    ddf_fun_get_name(fun), sess);
			async_answer_0(icall, EOK);
		} else {
			usb_log_error("Console session to %s already set.",
			    ddf_fun_get_name(fun));
			async_answer_0(icall, ELIMIT);
			async_hangup(sess);
		}
	} else {
		usb_log_debug("%s: Invalid function.", __FUNCTION__);
		async_answer_0(icall, EINVAL);
	}
}

static const usb_hid_report_field_t *get_mouse_axis_move_field(uint8_t rid, usb_hid_report_t *report,
    int32_t usage)
{
	usb_hid_report_path_t *path = usb_hid_report_path();
	usb_hid_report_path_append_item(path, USB_HIDUT_PAGE_GENERIC_DESKTOP,
	    usage);

	usb_hid_report_path_set_report_id(path, rid);

	const usb_hid_report_field_t *field = usb_hid_report_get_sibling(
	    report, NULL, path, USB_HID_PATH_COMPARE_END,
	    USB_HID_REPORT_TYPE_INPUT);

	usb_hid_report_path_free(path);

	return field;
}

static void usb_mouse_process_report(usb_hid_dev_t *hid_dev,
    usb_mouse_t *mouse_dev)
{
	assert(mouse_dev != NULL);

	if (mouse_dev->mouse_sess == NULL) {
		usb_log_warning(NAME " No console session.");
		return;
	}

	const usb_hid_report_field_t *move_x = get_mouse_axis_move_field(
	    hid_dev->report_id, &hid_dev->report,
	    USB_HIDUT_USAGE_GENERIC_DESKTOP_X);
	const usb_hid_report_field_t *move_y = get_mouse_axis_move_field(
	    hid_dev->report_id, &hid_dev->report,
	    USB_HIDUT_USAGE_GENERIC_DESKTOP_Y);
	const usb_hid_report_field_t *wheel = get_mouse_axis_move_field(
	    hid_dev->report_id, &hid_dev->report,
	    USB_HIDUT_USAGE_GENERIC_DESKTOP_WHEEL);

	bool absolute_x = move_x && !USB_HID_ITEM_FLAG_RELATIVE(move_x->item_flags);
	bool absolute_y = move_y && !USB_HID_ITEM_FLAG_RELATIVE(move_y->item_flags);

	/* Tablet shall always report both X and Y */
	if (absolute_x != absolute_y) {
		usb_log_error(NAME " cannot handle mix of absolute and relative mouse move.");
		return;
	}

	int shift_x = move_x ? move_x->value : 0;
	int shift_y = move_y ? move_y->value : 0;
	int shift_z =  wheel ?  wheel->value : 0;

	if (absolute_x && absolute_y) {
		async_exch_t *exch =
		    async_exchange_begin(mouse_dev->mouse_sess);
		if (exch != NULL) {
			async_msg_4(exch, MOUSEEV_ABS_MOVE_EVENT,
			    shift_x, shift_y, move_x->logical_maximum, move_y->logical_maximum);
			async_exchange_end(exch);
		}

		// Even if we move the mouse absolutely, we need to resolve wheel
		shift_x = shift_y = 0;
	}

	if (shift_x || shift_y || shift_z) {
		async_exch_t *exch =
		    async_exchange_begin(mouse_dev->mouse_sess);
		if (exch != NULL) {
			async_msg_3(exch, MOUSEEV_MOVE_EVENT,
			    shift_x, shift_y, shift_z);
			async_exchange_end(exch);
		}
	}

	/* Buttons */
	usb_hid_report_path_t *path = usb_hid_report_path();
	if (path == NULL) {
		usb_log_warning("Failed to create USB HID report path.");
		return;
	}
	errno_t ret =
	    usb_hid_report_path_append_item(path, USB_HIDUT_PAGE_BUTTON, 0);
	if (ret != EOK) {
		usb_hid_report_path_free(path);
		usb_log_warning("Failed to add buttons to report path.");
		return;
	}
	usb_hid_report_path_set_report_id(path, hid_dev->report_id);

	usb_hid_report_field_t *field = usb_hid_report_get_sibling(
	    &hid_dev->report, NULL, path, USB_HID_PATH_COMPARE_END |
	    USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY, USB_HID_REPORT_TYPE_INPUT);

	while (field != NULL) {
		usb_log_debug2(NAME " VALUE(%X) USAGE(%X)", field->value,
		    field->usage);
		assert(field->usage > field->usage_minimum);
		const unsigned index = field->usage - field->usage_minimum;
		assert(index < mouse_dev->buttons_count);

		if (mouse_dev->buttons[index] != field->value) {
			async_exch_t *exch =
			    async_exchange_begin(mouse_dev->mouse_sess);
			if (exch != NULL) {
				async_req_2_0(exch, MOUSEEV_BUTTON_EVENT,
				    field->usage, (field->value != 0) ? 1 : 0);
				async_exchange_end(exch);
				mouse_dev->buttons[index] = field->value;
			}
		}

		field = usb_hid_report_get_sibling(
		    &hid_dev->report, field, path, USB_HID_PATH_COMPARE_END |
		    USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY,
		    USB_HID_REPORT_TYPE_INPUT);
	}

	usb_hid_report_path_free(path);
}

#define FUN_UNBIND_DESTROY(fun) \
if (fun) { \
	if (ddf_fun_unbind((fun)) == EOK) { \
		ddf_fun_destroy((fun)); \
	} else { \
		usb_log_error("Could not unbind function `%s', it " \
		    "will not be destroyed.\n", ddf_fun_get_name(fun)); \
	} \
} else (void) 0

/** Get highest index of a button mentioned in given report.
 *
 * @param report HID report.
 * @param report_id Report id we are interested in.
 * @return Highest button mentioned in the report.
 * @retval 1 No button was mentioned.
 *
 */
static size_t usb_mouse_get_highest_button(usb_hid_report_t *report, uint8_t report_id)
{
	size_t highest_button = 0;

	usb_hid_report_path_t *path = usb_hid_report_path();
	usb_hid_report_path_append_item(path, USB_HIDUT_PAGE_BUTTON, 0);
	usb_hid_report_path_set_report_id(path, report_id);

	usb_hid_report_field_t *field = NULL;

	/* Break from within. */
	while (true) {
		field = usb_hid_report_get_sibling(
		    report, field, path,
		    USB_HID_PATH_COMPARE_END | USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY,
		    USB_HID_REPORT_TYPE_INPUT);
		/* No more buttons? */
		if (field == NULL) {
			break;
		}

		size_t current_button = field->usage - field->usage_minimum;
		if (current_button > highest_button) {
			highest_button = current_button;
		}
	}

	usb_hid_report_path_free(path);

	return highest_button;
}

static errno_t mouse_dev_init(usb_mouse_t *mouse_dev, usb_hid_dev_t *hid_dev)
{
	// FIXME: This may not be optimal since stupid hardware vendor may
	// use buttons 1, 2, 3 and 6000 and we would allocate array of
	// 6001*4B and use only 4 items in it.
	// Since I doubt that hardware producers would do that, I think
	// that the current solution is good enough.
	/* Adding 1 because we will be accessing buttons[highest]. */
	mouse_dev->buttons_count = 1 + usb_mouse_get_highest_button(
	    &hid_dev->report, hid_dev->report_id);
	mouse_dev->buttons = calloc(mouse_dev->buttons_count, sizeof(int32_t));

	if (mouse_dev->buttons == NULL) {
		usb_log_error(NAME ": out of memory, giving up on device!");
		free(mouse_dev);
		return ENOMEM;
	}

	// TODO: how to know if the device supports the request???
	usbhid_req_set_idle(usb_device_get_default_pipe(hid_dev->usb_dev),
	    usb_device_get_iface_number(hid_dev->usb_dev), IDLE_RATE);
	return EOK;
}

errno_t usb_mouse_init(usb_hid_dev_t *hid_dev, void **data)
{
	usb_log_debug("Initializing HID/Mouse structure...");

	if (hid_dev == NULL) {
		usb_log_error("Failed to init mouse structure: no structure"
		    " given.\n");
		return EINVAL;
	}

	/* Create the exposed function. */
	usb_log_debug("Creating DDF function %s...", HID_MOUSE_FUN_NAME);
	ddf_fun_t *fun = usb_device_ddf_fun_create(hid_dev->usb_dev,
	    fun_exposed, HID_MOUSE_FUN_NAME);
	if (fun == NULL) {
		usb_log_error("Could not create DDF function node `%s'.",
		    HID_MOUSE_FUN_NAME);
		return ENOMEM;
	}

	usb_mouse_t *mouse_dev = ddf_fun_data_alloc(fun, sizeof(usb_mouse_t));
	if (mouse_dev == NULL) {
		usb_log_error("Failed to alloc HID mouse device structure.");
		ddf_fun_destroy(fun);
		return ENOMEM;
	}

	errno_t ret = mouse_dev_init(mouse_dev, hid_dev);
	if (ret != EOK) {
		usb_log_error("Failed to init HID mouse device structure.");
		return ret;
	}

	ddf_fun_set_ops(fun, &ops);

	ret = ddf_fun_bind(fun);
	if (ret != EOK) {
		usb_log_error("Could not bind DDF function `%s': %s.",
		    ddf_fun_get_name(fun), str_error(ret));
		ddf_fun_destroy(fun);
		return ret;
	}

	usb_log_debug("Adding DDF function `%s' to category %s...",
	    ddf_fun_get_name(fun), HID_MOUSE_CATEGORY);
	ret = ddf_fun_add_to_category(fun, HID_MOUSE_CATEGORY);
	if (ret != EOK) {
		usb_log_error(
		    "Could not add DDF function to category %s: %s.\n",
		    HID_MOUSE_CATEGORY, str_error(ret));
		FUN_UNBIND_DESTROY(fun);
		return ret;
	}
	mouse_dev->mouse_fun = fun;

	/* Save the Mouse device structure into the HID device structure. */
	*data = mouse_dev;

	return EOK;
}

bool usb_mouse_polling_callback(usb_hid_dev_t *hid_dev, void *data)
{
	if (hid_dev == NULL || data == NULL) {
		usb_log_error(
		    "Missing argument to the mouse polling callback.\n");
		return false;
	}

	usb_mouse_t *mouse_dev = data;
	usb_mouse_process_report(hid_dev, mouse_dev);

	/* Continue polling until the device is about to be removed. */
	return true;
}

void usb_mouse_deinit(usb_hid_dev_t *hid_dev, void *data)
{
	if (data == NULL)
		return;

	usb_mouse_t *mouse_dev = data;

	/* Hangup session to the console */
	if (mouse_dev->mouse_sess != NULL)
		async_hangup(mouse_dev->mouse_sess);

	free(mouse_dev->buttons);
	FUN_UNBIND_DESTROY(mouse_dev->mouse_fun);
}

errno_t usb_mouse_set_boot_protocol(usb_hid_dev_t *hid_dev)
{
	errno_t rc = usb_hid_parse_report_descriptor(
	    &hid_dev->report, USB_MOUSE_BOOT_REPORT_DESCRIPTOR,
	    sizeof(USB_MOUSE_BOOT_REPORT_DESCRIPTOR));

	if (rc != EOK) {
		usb_log_error("Failed to parse boot report descriptor: %s",
		    str_error(rc));
		return rc;
	}

	rc = usbhid_req_set_protocol(
	    usb_device_get_default_pipe(hid_dev->usb_dev),
	    usb_device_get_iface_number(hid_dev->usb_dev),
	    USB_HID_PROTOCOL_BOOT);

	if (rc != EOK) {
		usb_log_warning("Failed to set boot protocol to the device: "
		    "%s\n", str_error(rc));
		return rc;
	}

	return EOK;
}

/**
 * @}
 */
