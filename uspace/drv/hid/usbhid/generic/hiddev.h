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
 * USB HID driver API.
 */

#ifndef USB_HID_HIDDDEV_H_
#define USB_HID_HIDDDEV_H_

#include <usb/dev/driver.h>
#include "../usbhid.h"

extern const usb_endpoint_description_t
    usb_hid_generic_poll_endpoint_description;

extern const char *HID_GENERIC_FUN_NAME;
extern const char *HID_GENERIC_CATEGORY;

/** The USB HID generic 'hid' function softstate */
typedef struct {
	usb_hid_dev_t *hid_dev;
} usb_hid_gen_fun_t;

extern errno_t usb_generic_hid_init(usb_hid_dev_t *, void **);
extern void usb_generic_hid_deinit(usb_hid_dev_t *, void *);
extern bool usb_generic_hid_polling_callback(usb_hid_dev_t *, void *);

#endif // USB_HID_HIDDDEV_H_

/**
 * @}
 */
