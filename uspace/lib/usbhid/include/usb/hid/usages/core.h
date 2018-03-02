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
 * @brief USB HID Usage Tables.
 */
#ifndef LIBUSBHID_HIDUT_H_
#define LIBUSBHID_HIDUT_H_

/** USB/HID Usage Pages. */
typedef enum {
	USB_HIDUT_PAGE_GENERIC_DESKTOP = 1,
	USB_HIDUT_PAGE_SIMULATION = 2,
	USB_HIDUT_PAGE_VR = 3,
	USB_HIDUT_PAGE_SPORT = 4,
	USB_HIDUT_PAGE_GAME = 5,
	USB_HIDUT_PAGE_GENERIC_DEVICE = 6,
	USB_HIDUT_PAGE_KEYBOARD = 7,
	USB_HIDUT_PAGE_LED = 8,
	USB_HIDUT_PAGE_BUTTON = 9,
	USB_HIDUT_PAGE_ORDINAL = 0x0a,
	USB_HIDUT_PAGE_TELEPHONY_DEVICE = 0x0b,
	USB_HIDUT_PAGE_CONSUMER = 0x0c
} usb_hidut_usage_page_t;

/** Usages for Generic Desktop Page. */
typedef enum {
	USB_HIDUT_USAGE_GENERIC_DESKTOP_POINTER = 1,
	USB_HIDUT_USAGE_GENERIC_DESKTOP_MOUSE = 2,
	USB_HIDUT_USAGE_GENERIC_DESKTOP_JOYSTICK = 4,
	USB_HIDUT_USAGE_GENERIC_DESKTOP_GAMEPAD = 5,
	USB_HIDUT_USAGE_GENERIC_DESKTOP_KEYBOARD = 6,
	USB_HIDUT_USAGE_GENERIC_DESKTOP_KEYPAD = 7,
	USB_HIDUT_USAGE_GENERIC_DESKTOP_X = 0x30,
	USB_HIDUT_USAGE_GENERIC_DESKTOP_Y = 0x31,
	USB_HIDUT_USAGE_GENERIC_DESKTOP_WHEEL = 0x38
	/* USB_HIDUT_USAGE_GENERIC_DESKTOP_ = , */

} usb_hidut_usage_generic_desktop_t;

typedef enum {
	USB_HIDUT_USAGE_CONSUMER_CONSUMER_CONTROL = 1
} usb_hidut_usage_consumer_t;


#endif
/**
 * @}
 */
