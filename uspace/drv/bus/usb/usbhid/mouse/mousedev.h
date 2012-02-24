/*
 * Copyright (c) 2011 Lubos Slovak
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
/** @file
 * USB Mouse driver API.
 */

#ifndef USB_HID_MOUSEDEV_H_
#define USB_HID_MOUSEDEV_H_

#include <usb/dev/driver.h>
#include <async.h>

struct usb_hid_dev;



/** Container for USB mouse device. */
typedef struct {
	/** IPC session to consumer. */
	async_sess_t *mouse_sess;

	/* Mouse buttons statuses. */
	int32_t *buttons;
	size_t buttons_count;

	/* DDF mouse function */
	ddf_fun_t *mouse_fun;
} usb_mouse_t;



extern const usb_endpoint_description_t usb_hid_mouse_poll_endpoint_description;

const char *HID_MOUSE_FUN_NAME;
const char *HID_MOUSE_CATEGORY;



int usb_mouse_init(struct usb_hid_dev *hid_dev, void **data);

bool usb_mouse_polling_callback(struct usb_hid_dev *hid_dev, void *data);

void usb_mouse_deinit(struct usb_hid_dev *hid_dev, void *data);

int usb_mouse_set_boot_protocol(struct usb_hid_dev *hid_dev);



#endif // USB_HID_MOUSEDEV_H_

/**
 * @}
 */
