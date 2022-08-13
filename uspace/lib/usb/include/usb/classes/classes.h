/*
 * SPDX-FileCopyrightText: 2010 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
