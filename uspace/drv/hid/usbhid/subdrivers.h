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
 * USB HID subdriver mappings.
 */

#ifndef USB_HID_SUBDRIVERS_H_
#define USB_HID_SUBDRIVERS_H_

#include "usbhid.h"
#include "kbd/kbddev.h"

typedef struct usb_hid_subdriver_usage {
	int usage_page;
	int usage;
} usb_hid_subdriver_usage_t;

/** Structure representing the mapping between device requirements and the
 *  subdriver supposed to handle this device.
 *
 * By filling in this structure and adding it to the usb_hid_subdrivers array,
 * a new subdriver mapping will be created and used by the HID driver when it
 * searches for appropriate subdrivers for a device.
 *
 */
typedef struct usb_hid_subdriver_mapping {
	/** Usage path that the device's input reports must contain.
	 *
	 * It is an array of pairs <usage_page, usage>, terminated by a <0, 0>
	 * pair. If you do not wish to specify the device in this way, set this
	 * to NULL.
	 */
	const usb_hid_subdriver_usage_t *usage_path;

	/** Report ID for which the path should apply. */
	int report_id;

	/** Compare type for the usage path. */
	int compare;

	/** Vendor ID (set to -1 if not specified). */
	int vendor_id;

	/** Product ID (set to -1 if not specified). */
	int product_id;

	/** Subdriver for controlling this device. */
	const usb_hid_subdriver_t subdriver;
} usb_hid_subdriver_mapping_t;

extern const usb_hid_subdriver_mapping_t usb_hid_subdrivers[];
extern const size_t USB_HID_MAX_SUBDRIVERS;

#endif /* USB_HID_SUBDRIVERS_H_ */

/**
 * @}
 */
