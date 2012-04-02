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

/** @addtogroup libusb
 * @{
 */
/** @file
 * USB device classes (generic constants and functions).
 */
#ifndef LIBUSB_CLASSES_H_
#define LIBUSB_CLASSES_H_

/** USB device class. */
typedef enum {
	USB_CLASS_USE_INTERFACE = 0x00,
	USB_CLASS_AUDIO = 0x01,
	USB_CLASS_COMMUNICATIONS_CDC_CONTROL = 0x02,
	USB_CLASS_HID = 0x03,
	USB_CLASS_PHYSICAL = 0x05,
	USB_CLASS_IMAGE = 0x06,
	USB_CLASS_PRINTER = 0x07,
	USB_CLASS_MASS_STORAGE = 0x08,
	USB_CLASS_HUB = 0x09,
	USB_CLASS_CDC_DATA = 0x0A,
	USB_CLASS_SMART_CARD = 0x0B,
	USB_CLASS_CONTENT_SECURITY = 0x0D,
	USB_CLASS_VIDEO = 0x0E,
	USB_CLASS_PERSONAL_HEALTHCARE = 0x0F,
	USB_CLASS_DIAGNOSTIC = 0xDC,
	USB_CLASS_WIRELESS_CONTROLLER = 0xE0,
	USB_CLASS_MISCELLANEOUS = 0xEF,
	USB_CLASS_APPLICATION_SPECIFIC = 0xFE,
	USB_CLASS_VENDOR_SPECIFIC = 0xFF,
	/* USB_CLASS_ = 0x, */
} usb_class_t;

const char *usb_str_class(usb_class_t);

#endif
/**
 * @}
 */
