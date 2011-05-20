/*
 * Copyright (c) 2011 Lubos Slovak, Vojtech Horky
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
#include <str_error.h>
#include <ipc/mouse.h>
#include <io/console.h>

#include <ipc/kbd.h>
#include <io/keycode.h>

#include "mousedev.h"
#include "../usbhid.h"

#define NAME "mouse"

/*----------------------------------------------------------------------------*/

usb_endpoint_description_t usb_hid_mouse_poll_endpoint_description = {
	.transfer_type = USB_TRANSFER_INTERRUPT,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_HID,
	.interface_subclass = USB_HID_SUBCLASS_BOOT,
	.interface_protocol = USB_HID_PROTOCOL_MOUSE,
	.flags = 0
};

const char *HID_MOUSE_FUN_NAME = "mouse";
const char *HID_MOUSE_WHEEL_FUN_NAME = "mouse-wheel";
const char *HID_MOUSE_CLASS_NAME = "mouse";
const char *HID_MOUSE_WHEEL_CLASS_NAME = "keyboard";

/** Default idle rate for mouses. */
static const uint8_t IDLE_RATE = 0;
static const size_t USB_MOUSE_BUTTON_COUNT = 3;

/*----------------------------------------------------------------------------*/

enum {
	USB_MOUSE_BOOT_REPORT_DESCRIPTOR_SIZE = 63
};

static const uint8_t USB_MOUSE_BOOT_REPORT_DESCRIPTOR[
    USB_MOUSE_BOOT_REPORT_DESCRIPTOR_SIZE] = {
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

/*----------------------------------------------------------------------------*/

/** Default handler for IPC methods not handled by DDF.
 *
 * @param fun Device function handling the call.
 * @param icallid Call id.
 * @param icall Call data.
 */
static void default_connection_handler(ddf_fun_t *fun,
    ipc_callid_t icallid, ipc_call_t *icall)
{
	sysarg_t method = IPC_GET_IMETHOD(*icall);
	
	usb_mouse_t *mouse_dev = (usb_mouse_t *)fun->driver_data;
	
	if (mouse_dev == NULL) {
		usb_log_debug("default_connection_handler: Missing "
		    "parameters.\n");
		async_answer_0(icallid, EINVAL);
		return;
	}
	
	usb_log_debug("default_connection_handler: fun->name: %s\n",
	              fun->name);
	usb_log_debug("default_connection_handler: mouse_phone: %d, wheel "
	    "phone: %d\n", mouse_dev->mouse_phone, mouse_dev->wheel_phone);
	
	int *phone = (str_cmp(fun->name, HID_MOUSE_FUN_NAME) == 0) 
		     ? &mouse_dev->mouse_phone : &mouse_dev->wheel_phone;
	
	if (method == IPC_M_CONNECT_TO_ME) {
		int callback = IPC_GET_ARG5(*icall);

		if (*phone != -1) {
			usb_log_debug("default_connection_handler: Console "
			    "phone to mouse already set.\n");
			async_answer_0(icallid, ELIMIT);
			return;
		}

		*phone = callback;
		usb_log_debug("Console phone to mouse set ok (%d).\n", *phone);
		async_answer_0(icallid, EOK);
		return;
	}

	usb_log_debug("default_connection_handler: Invalid function.\n");
	async_answer_0(icallid, EINVAL);
}

/*----------------------------------------------------------------------------*/

static usb_mouse_t *usb_mouse_new(void)
{
	usb_mouse_t *mouse = calloc(1, sizeof(usb_mouse_t));
	if (mouse == NULL) {
		return NULL;
	}
	mouse->mouse_phone = -1;
	mouse->wheel_phone = -1;
	
	return mouse;
}

/*----------------------------------------------------------------------------*/

static void usb_mouse_free(usb_mouse_t **mouse_dev)
{
	assert(mouse_dev != NULL && *mouse_dev != NULL);
	
	// hangup phone to the console
	if ((*mouse_dev)->mouse_phone >= 0) {
		async_hangup((*mouse_dev)->mouse_phone);
	}
	
	if ((*mouse_dev)->wheel_phone >= 0) {
		async_hangup((*mouse_dev)->wheel_phone);
	}
	
	free(*mouse_dev);
	*mouse_dev = NULL;
}

/*----------------------------------------------------------------------------*/

static void usb_mouse_send_wheel(const usb_mouse_t *mouse_dev, int wheel)
{
	console_event_t ev;
	
	ev.type = KEY_PRESS;
	ev.key = (wheel > 0) ? KC_UP : (wheel < 0) ? KC_DOWN : 0;
	ev.mods = 0;
	ev.c = 0;

	if (mouse_dev->wheel_phone < 0) {
		usb_log_warning(
		    "Connection to console not ready, key discarded.\n");
		return;
	}
	
	int count = (wheel < 0) ? -wheel : wheel;
	int i;
	
	for (i = 0; i < count * 3; ++i) {
		usb_log_debug2("Sending key %d to the console\n", ev.key);
		async_msg_4(mouse_dev->wheel_phone, KBD_EVENT, ev.type, 
		    ev.key, ev.mods, ev.c);
		// send key release right away
		async_msg_4(mouse_dev->wheel_phone, KBD_EVENT, KEY_RELEASE, 
		    ev.key, ev.mods, ev.c);
	}
}

/*----------------------------------------------------------------------------*/

static bool usb_mouse_process_report(usb_hid_dev_t *hid_dev, 
                                     usb_mouse_t *mouse_dev, uint8_t *buffer,
                                     size_t buffer_size)
{
	assert(mouse_dev != NULL);
	
	usb_log_debug2("got buffer: %s.\n",
	    usb_debug_str_buffer(buffer, buffer_size, 0));
	
	if (mouse_dev->mouse_phone < 0) {
		usb_log_error(NAME " No console phone.\n");
		return false;	// ??
	}

	/*
	 * parse the input report
	 */
	
	usb_log_debug(NAME " Calling usb_hid_parse_report() with "
	    "buffer %s\n", usb_debug_str_buffer(buffer, buffer_size, 0));
	
	uint8_t report_id;
	
	int rc = usb_hid_parse_report(hid_dev->report, buffer, buffer_size, 
	    &report_id);
	
	if (rc != EOK) {
		usb_log_warning(NAME "Error in usb_hid_parse_report(): %s\n", 
		    str_error(rc));
		return true;
	}
	
	/*
	 * X
	 */
	int shift_x = 0;
	
	usb_hid_report_path_t *path = usb_hid_report_path();
	usb_hid_report_path_append_item(path, USB_HIDUT_PAGE_GENERIC_DESKTOP, 
	    USB_HIDUT_USAGE_GENERIC_DESKTOP_X);

	usb_hid_report_path_set_report_id(path, report_id);

	usb_hid_report_field_t *field = usb_hid_report_get_sibling(
	    hid_dev->report, NULL, path, USB_HID_PATH_COMPARE_END, 
	    USB_HID_REPORT_TYPE_INPUT);

	if (field != NULL) {
		usb_log_debug(NAME " VALUE(%X) USAGE(%X)\n", field->value, 
		    field->usage);
		shift_x = field->value;
	}

	usb_hid_report_path_free(path);
	
	/*
	 * Y
	 */
	int shift_y = 0;
	
	path = usb_hid_report_path();
	usb_hid_report_path_append_item(path, USB_HIDUT_PAGE_GENERIC_DESKTOP, 
	    USB_HIDUT_USAGE_GENERIC_DESKTOP_Y);

	usb_hid_report_path_set_report_id(path, report_id);

	field = usb_hid_report_get_sibling(
	    hid_dev->report, NULL, path, USB_HID_PATH_COMPARE_END, 
	    USB_HID_REPORT_TYPE_INPUT);

	if (field != NULL) {
		usb_log_debug(NAME " VALUE(%X) USAGE(%X)\n", field->value, 
		    field->usage);
		shift_y = field->value;
	}

	usb_hid_report_path_free(path);
	
	if ((shift_x != 0) || (shift_y != 0)) {
		async_req_2_0(mouse_dev->mouse_phone,
		    MEVENT_MOVE, shift_x, shift_y);
	}
	
	/*
	 * Wheel
	 */
	int wheel = 0;
	
	path = usb_hid_report_path();
	usb_hid_report_path_append_item(path, USB_HIDUT_PAGE_GENERIC_DESKTOP, 
	    USB_HIDUT_USAGE_GENERIC_DESKTOP_WHEEL);

	usb_hid_report_path_set_report_id(path, report_id);
	
	field = usb_hid_report_get_sibling(
	    hid_dev->report, NULL, path, USB_HID_PATH_COMPARE_END, 
	    USB_HID_REPORT_TYPE_INPUT);

	if (field != NULL) {
		usb_log_debug(NAME " VALUE(%X) USAGE(%X)\n", field->value, 
		    field->usage);
		wheel = field->value;
	}

	usb_hid_report_path_free(path);
	
	// send arrow up for positive direction and arrow down for negative
	// direction; three arrows for difference of 1
	usb_mouse_send_wheel(mouse_dev, wheel);
	
	
	/*
	 * Buttons
	 */
	path = usb_hid_report_path();
	usb_hid_report_path_append_item(path, USB_HIDUT_PAGE_BUTTON, 0);
	usb_hid_report_path_set_report_id(path, report_id);
	
	field = usb_hid_report_get_sibling(
	    hid_dev->report, NULL, path, USB_HID_PATH_COMPARE_END
	    | USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY, 
	    USB_HID_REPORT_TYPE_INPUT);

	while (field != NULL) {
		usb_log_debug(NAME " VALUE(%X) USAGE(%X)\n", field->value, 
		    field->usage);
		
		if (mouse_dev->buttons[field->usage - field->usage_minimum] == 0
		    && field->value != 0) {
			async_req_2_0(mouse_dev->mouse_phone,
			    MEVENT_BUTTON, field->usage, 1);
			mouse_dev->buttons[field->usage - field->usage_minimum]
			    = field->value;
		} else if (
		    mouse_dev->buttons[field->usage - field->usage_minimum] != 0
		    && field->value == 0) {
		       async_req_2_0(mouse_dev->mouse_phone,
			   MEVENT_BUTTON, field->usage, 0);
		       mouse_dev->buttons[field->usage - field->usage_minimum]
			   = field->value;
	       }
		
		field = usb_hid_report_get_sibling(
		    hid_dev->report, field, path, USB_HID_PATH_COMPARE_END
		    | USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY, 
		    USB_HID_REPORT_TYPE_INPUT);
	}
	
	usb_hid_report_path_free(path);

	return true;
}

/*----------------------------------------------------------------------------*/

static int usb_mouse_create_function(usb_hid_dev_t *hid_dev, usb_mouse_t *mouse)
{
	assert(hid_dev != NULL);
	assert(mouse != NULL);
	
	/* Create the function exposed under /dev/devices. */
	usb_log_debug("Creating DDF function %s...\n", HID_MOUSE_FUN_NAME);
	ddf_fun_t *fun = ddf_fun_create(hid_dev->usb_dev->ddf_dev, fun_exposed, 
	    HID_MOUSE_FUN_NAME);
	if (fun == NULL) {
		usb_log_error("Could not create DDF function node.\n");
		return ENOMEM;
	}
	
	fun->ops = &mouse->ops;
	fun->driver_data = mouse;   // TODO: maybe change to hid_dev->data

	int rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		usb_log_error("Could not bind DDF function: %s.\n",
		    str_error(rc));
		ddf_fun_destroy(fun);
		return rc;
	}
	
	usb_log_debug("Adding DDF function to class %s...\n", 
	    HID_MOUSE_CLASS_NAME);
	rc = ddf_fun_add_to_class(fun, HID_MOUSE_CLASS_NAME);
	if (rc != EOK) {
		usb_log_error(
		    "Could not add DDF function to class %s: %s.\n",
		    HID_MOUSE_CLASS_NAME, str_error(rc));
		ddf_fun_destroy(fun);
		return rc;
	}
	
	/*
	 * Special function for acting as keyboard (wheel)
	 */
	usb_log_debug("Creating DDF function %s...\n", 
	              HID_MOUSE_WHEEL_FUN_NAME);
	fun = ddf_fun_create(hid_dev->usb_dev->ddf_dev, fun_exposed, 
	    HID_MOUSE_WHEEL_FUN_NAME);
	if (fun == NULL) {
		usb_log_error("Could not create DDF function node.\n");
		return ENOMEM;
	}
	
	/*
	 * Store the initialized HID device and HID ops
	 * to the DDF function.
	 */
	fun->ops = &mouse->ops;
	fun->driver_data = mouse;   // TODO: maybe change to hid_dev->data

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		usb_log_error("Could not bind DDF function: %s.\n",
		    str_error(rc));
		ddf_fun_destroy(fun);
		return rc;
	}
	
	usb_log_debug("Adding DDF function to class %s...\n", 
	    HID_MOUSE_WHEEL_CLASS_NAME);
	rc = ddf_fun_add_to_class(fun, HID_MOUSE_WHEEL_CLASS_NAME);
	if (rc != EOK) {
		usb_log_error(
		    "Could not add DDF function to class %s: %s.\n",
		    HID_MOUSE_WHEEL_CLASS_NAME, str_error(rc));
		ddf_fun_destroy(fun);
		return rc;
	}
	
	return EOK;
}

/*----------------------------------------------------------------------------*/

int usb_mouse_init(usb_hid_dev_t *hid_dev, void **data)
{
	usb_log_debug("Initializing HID/Mouse structure...\n");
	
	if (hid_dev == NULL) {
		usb_log_error("Failed to init keyboard structure: no structure"
		    " given.\n");
		return EINVAL;
	}
	
	usb_mouse_t *mouse_dev = usb_mouse_new();
	if (mouse_dev == NULL) {
		usb_log_error("Error while creating USB/HID Mouse device "
		    "structure.\n");
		return ENOMEM;
	}
	
	mouse_dev->buttons = (int32_t *)calloc(USB_MOUSE_BUTTON_COUNT, 
	    sizeof(int32_t));
	
	if (mouse_dev->buttons == NULL) {
		usb_log_fatal("No memory!\n");
		free(mouse_dev);
		return ENOMEM;
	}
	
	// save the Mouse device structure into the HID device structure
	*data = mouse_dev;
	
	// set handler for incoming calls
	// TODO: must be one for each subdriver!!
	mouse_dev->ops.default_handler = default_connection_handler;
	
	// TODO: how to know if the device supports the request???
//	usbhid_req_set_idle(&hid_dev->usb_dev->ctrl_pipe, 
//	    hid_dev->usb_dev->interface_no, IDLE_RATE);
	
	int rc = usb_mouse_create_function(hid_dev, mouse_dev);
	if (rc != EOK) {
		usb_mouse_free(&mouse_dev);
		return rc;
	}
	
	return EOK;
}

/*----------------------------------------------------------------------------*/

bool usb_mouse_polling_callback(usb_hid_dev_t *hid_dev, void *data, 
     uint8_t *buffer, size_t buffer_size)
{
	usb_log_debug("usb_mouse_polling_callback()\n");
	usb_debug_str_buffer(buffer, buffer_size, 0);
	
	if (hid_dev == NULL || data == NULL) {
		usb_log_error("Missing argument to the mouse polling callback."
		    "\n");
		return false;
	}
	
	usb_mouse_t *mouse_dev = (usb_mouse_t *)data;
		
	return usb_mouse_process_report(hid_dev, mouse_dev, buffer, 
	                                buffer_size);
}

/*----------------------------------------------------------------------------*/

void usb_mouse_deinit(usb_hid_dev_t *hid_dev, void *data)
{
	if (data != NULL) {
		usb_mouse_free((usb_mouse_t **)&data);
	}
}

/*----------------------------------------------------------------------------*/

int usb_mouse_set_boot_protocol(usb_hid_dev_t *hid_dev)
{
	int rc = usb_hid_parse_report_descriptor(hid_dev->report, 
	    USB_MOUSE_BOOT_REPORT_DESCRIPTOR, 
	    USB_MOUSE_BOOT_REPORT_DESCRIPTOR_SIZE);
	
	if (rc != EOK) {
		usb_log_error("Failed to parse boot report descriptor: %s\n",
		    str_error(rc));
		return rc;
	}
	
	rc = usbhid_req_set_protocol(&hid_dev->usb_dev->ctrl_pipe, 
	    hid_dev->usb_dev->interface_no, USB_HID_PROTOCOL_BOOT);
	
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
