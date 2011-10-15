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

/** @addtogroup drvusbmouse
 * @{
 */
/**
 * @file
 * Initialization routines for USB mouse driver.
 */

#include "mouse.h"
#include <usb/debug.h>
#include <usb/classes/classes.h>
#include <usb/hid/hid.h>
#include <usb/dev/request.h>
#include <usb/hid/request.h>
#include <errno.h>

/** Mouse polling endpoint description for boot protocol subclass. */
const usb_endpoint_description_t poll_endpoint_description = {
	.transfer_type = USB_TRANSFER_INTERRUPT,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_HID,
	.interface_subclass = USB_HID_SUBCLASS_BOOT,
	.interface_protocol = USB_HID_PROTOCOL_MOUSE,
	.flags = 0
};

/** Default handler for IPC methods not handled by DDF.
 *
 * @param fun     Device function handling the call.
 * @param icallid Call ID.
 * @param icall   Call data.
 *
 */
static void default_connection_handler(ddf_fun_t *fun, ipc_callid_t icallid,
    ipc_call_t *icall)
{
	usb_mouse_t *mouse = (usb_mouse_t *) fun->driver_data;
	assert(mouse != NULL);
	
	async_sess_t *callback =
	    async_callback_receive_start(EXCHANGE_SERIALIZE, icall);
	
	if (callback) {
		if (mouse->console_sess == NULL) {
			mouse->console_sess = callback;
			async_answer_0(icallid, EOK);
		} else
			async_answer_0(icallid, ELIMIT);
	} else
		async_answer_0(icallid, EINVAL);
}

/** Device ops for USB mouse. */
static ddf_dev_ops_t mouse_ops = {
	.default_handler = default_connection_handler
};

/** Create USB mouse device.
 *
 * The mouse device is stored into <code>dev-&gt;driver_data</code>.
 *
 * @param dev Generic device.
 * @return Error code.
 */
int usb_mouse_create(usb_device_t *dev)
{
	usb_mouse_t *mouse = malloc(sizeof(usb_mouse_t));
	if (mouse == NULL)
		return ENOMEM;
	
	mouse->dev = dev;
	mouse->console_sess = NULL;
	
	int rc;
	
	/* Create DDF function. */
	mouse->mouse_fun = ddf_fun_create(dev->ddf_dev, fun_exposed, "mouse");
	if (mouse->mouse_fun == NULL) {
		rc = ENOMEM;
		goto leave;
	}
	
	mouse->mouse_fun->ops = &mouse_ops;
	
	rc = ddf_fun_bind(mouse->mouse_fun);
	if (rc != EOK)
		goto leave;
	
	/* Add the function to mouse class. */
	rc = ddf_fun_add_to_category(mouse->mouse_fun, "mouse");
	if (rc != EOK)
		goto leave;
	
	/* Set the boot protocol. */
	rc = usbhid_req_set_protocol(&dev->ctrl_pipe, dev->interface_no,
	    USB_HID_PROTOCOL_BOOT);
	if (rc != EOK)
		goto leave;
	
	/* Everything allright. */
	dev->driver_data = mouse;
	mouse->mouse_fun->driver_data = mouse;
	
	return EOK;
	
leave:
	free(mouse);
	return rc;
}

/**
 * @}
 */
