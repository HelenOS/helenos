/*
 * SPDX-FileCopyrightText: 2010 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libusb
 * @{
 */
/** @file
 * @brief Class related functions.
 */
#include <usb/classes/classes.h>

/** Tell string representation of USB class.
 *
 * @param cls Class code.
 * @return String representation.
 */
const char *usb_str_class(usb_class_t cls)
{
	switch (cls) {
	case USB_CLASS_USE_INTERFACE:
		return "use-interface";
	case USB_CLASS_AUDIO:
		return "audio";
	case USB_CLASS_COMMUNICATIONS_CDC_CONTROL:
		return "communications";
	case USB_CLASS_HID:
		return "HID";
	case USB_CLASS_PHYSICAL:
		return "physical";
	case USB_CLASS_IMAGE:
		return "image";
	case USB_CLASS_PRINTER:
		return "printer";
	case USB_CLASS_MASS_STORAGE:
		return "mass-storage";
	case USB_CLASS_HUB:
		return "hub";
	case USB_CLASS_CDC_DATA:
		return "CDC";
	case USB_CLASS_SMART_CARD:
		return "smart-card";
	case USB_CLASS_CONTENT_SECURITY:
		return "security";
	case USB_CLASS_VIDEO:
		return "video";
	case USB_CLASS_PERSONAL_HEALTHCARE:
		return "healthcare";
	case USB_CLASS_DIAGNOSTIC:
		return "diagnostic";
	case USB_CLASS_WIRELESS_CONTROLLER:
		return "wireless";
	case USB_CLASS_MISCELLANEOUS:
		return "misc";
	case USB_CLASS_APPLICATION_SPECIFIC:
		return "application";
	case USB_CLASS_VENDOR_SPECIFIC:
		return "vendor-specific";
	default:
		return "unknown";
	}
}

/**
 * @}
 */
