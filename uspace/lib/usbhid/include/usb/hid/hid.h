/*
 * SPDX-FileCopyrightText: 2010 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
