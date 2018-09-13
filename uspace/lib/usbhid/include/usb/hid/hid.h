/*
 * Copyright (c) 2010 Vojtech Horky
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

/** @addtogroup libusbhid
 * @{
 */
/** @file
 * @brief USB HID device related types.
 */
#ifndef LIBUSBHID_HID_H_
#define LIBUSBHID_HID_H_

#include <usb/usb.h>
#include <usb/hid/hidparser.h>
#include <usb/descriptor.h>

/** USB/HID device requests. */
typedef enum {
	USB_HIDREQ_GET_REPORT = 1,
	USB_HIDREQ_GET_IDLE = 2,
	USB_HIDREQ_GET_PROTOCOL = 3,
	/* Values 4 to 8 are reserved. */
	USB_HIDREQ_SET_REPORT = 9,
	USB_HIDREQ_SET_IDLE = 10,
	USB_HIDREQ_SET_PROTOCOL = 11
} usb_hid_request_t;

typedef enum {
	USB_HID_PROTOCOL_BOOT = 0,
	USB_HID_PROTOCOL_REPORT = 1
} usb_hid_protocol_t;

/** USB/HID subclass constants. */
typedef enum {
	USB_HID_SUBCLASS_NONE = 0,
	USB_HID_SUBCLASS_BOOT = 1
} usb_hid_subclass_t;

/** USB/HID interface protocols. */
typedef enum {
	USB_HID_PROTOCOL_NONE = 0,
	USB_HID_PROTOCOL_KEYBOARD = 1,
	USB_HID_PROTOCOL_MOUSE = 2
} usb_hid_iface_protocol_t;

#endif
/**
 * @}
 */
