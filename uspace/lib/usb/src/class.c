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
