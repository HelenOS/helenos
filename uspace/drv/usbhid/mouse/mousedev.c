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
#include <usb/classes/hid.h>
#include <usb/classes/hidreq.h>
#include <errno.h>
#include <str_error.h>
#include <ipc/mouse.h>

#include "mousedev.h"
#include "../usbhid.h"

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
const char *HID_MOUSE_CLASS_NAME = "mouse";

/** Default idle rate for mouses. */
static const uint8_t IDLE_RATE = 0;

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
	
	usb_hid_dev_t *hid_dev = (usb_hid_dev_t *)fun->driver_data;
	
	if (hid_dev == NULL || hid_dev->data == NULL) {
		async_answer_0(icallid, EINVAL);
		return;
	}
	
	assert(hid_dev != NULL);
	assert(hid_dev->data != NULL);
	usb_mouse_t *mouse_dev = (usb_mouse_t *)hid_dev->data;
	
	if (method == IPC_M_CONNECT_TO_ME) {
		int callback = IPC_GET_ARG5(*icall);

		if (mouse_dev->console_phone != -1) {
			async_answer_0(icallid, ELIMIT);
			return;
		}

		mouse_dev->console_phone = callback;
		usb_log_debug("Console phone to mouse set ok (%d).\n", callback);
		async_answer_0(icallid, EOK);
		return;
	}

	async_answer_0(icallid, EINVAL);
}

/*----------------------------------------------------------------------------*/

static usb_mouse_t *usb_mouse_new(void)
{
	usb_mouse_t *mouse = calloc(1, sizeof(usb_mouse_t));
	if (mouse == NULL) {
		return NULL;
	}
	mouse->console_phone = -1;
	
	return mouse;
}

/*----------------------------------------------------------------------------*/

static void usb_mouse_free(usb_mouse_t **mouse_dev)
{
	if (mouse_dev == NULL || *mouse_dev == NULL) {
		return;
	}
	
	// hangup phone to the console
	async_hangup((*mouse_dev)->console_phone);
	
	free(*mouse_dev);
	*mouse_dev = NULL;
}

/*----------------------------------------------------------------------------*/

static bool usb_mouse_process_boot_report(usb_mouse_t *mouse_dev,
    uint8_t *buffer, size_t buffer_size)
{
	usb_log_debug2("got buffer: %s.\n",
	    usb_debug_str_buffer(buffer, buffer_size, 0));

	uint8_t butt = buffer[0];
	char str_buttons[4] = {
		butt & 1 ? '#' : '.',
		butt & 2 ? '#' : '.',
		butt & 4 ? '#' : '.',
		0
	};

	int shift_x = ((int) buffer[1]) - 127;
	int shift_y = ((int) buffer[2]) - 127;
	int wheel = ((int) buffer[3]) - 127;

	if (buffer[1] == 0) {
		shift_x = 0;
	}
	if (buffer[2] == 0) {
		shift_y = 0;
	}
	if (buffer[3] == 0) {
		wheel = 0;
	}
	
	if (mouse_dev->console_phone >= 0) {
		usb_log_debug("Console phone: %d\n", mouse_dev->console_phone);
		if ((shift_x != 0) || (shift_y != 0)) {
			/* FIXME: guessed for QEMU */
			async_req_2_0(mouse_dev->console_phone,
			    MEVENT_MOVE,
			    - shift_x / 10,  - shift_y / 10);
		} else {
			usb_log_error("No move reported\n");
		}
		if (butt) {
			/* FIXME: proper button clicking. */
			async_req_2_0(mouse_dev->console_phone,
			    MEVENT_BUTTON, 1, 1);
			async_req_2_0(mouse_dev->console_phone,
			    MEVENT_BUTTON, 1, 0);
		}
	} else {
		usb_log_error("No console phone in mouse!!\n");
	}

	usb_log_debug("buttons=%s  dX=%+3d  dY=%+3d  wheel=%+3d\n",
	    str_buttons, shift_x, shift_y, wheel);

	/* Guess. */
	//async_usleep(1000);
	// no sleep right now

	return true;
}

/*----------------------------------------------------------------------------*/

int usb_mouse_init(usb_hid_dev_t *hid_dev)
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
	
	// save the Mouse device structure into the HID device structure
	hid_dev->data = mouse_dev;
	
	// set handler for incoming calls
	hid_dev->ops.default_handler = default_connection_handler;
	
	// TODO: how to know if the device supports the request???
	usbhid_req_set_idle(&hid_dev->usb_dev->ctrl_pipe, 
	    hid_dev->usb_dev->interface_no, IDLE_RATE);
	
	return EOK;
}

/*----------------------------------------------------------------------------*/

bool usb_mouse_polling_callback(usb_hid_dev_t *hid_dev, uint8_t *buffer,
     size_t buffer_size)
{
	usb_log_debug("usb_mouse_polling_callback()\n");
	usb_debug_str_buffer(buffer, buffer_size, 0);
	
	if (hid_dev == NULL) {
		usb_log_error("Missing argument to the mouse polling callback."
		    "\n");
		return false;
	}
	
	if (hid_dev->data == NULL) {
		usb_log_error("Wrong argument to the mouse polling callback."
		    "\n");
		return false;
	}
	usb_mouse_t *mouse_dev = (usb_mouse_t *)hid_dev->data;
	
	return usb_mouse_process_boot_report(mouse_dev, buffer, buffer_size);
}

/*----------------------------------------------------------------------------*/

void usb_mouse_deinit(usb_hid_dev_t *hid_dev)
{
	usb_mouse_free((usb_mouse_t **)&hid_dev->data);
}

/*----------------------------------------------------------------------------*/

int usb_mouse_set_boot_protocol(usb_hid_dev_t *hid_dev)
{
	int rc = usb_hid_parse_report_descriptor(hid_dev->parser, 
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
