/*
 * SPDX-FileCopyrightText: 2011 Lubos Slovak
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
