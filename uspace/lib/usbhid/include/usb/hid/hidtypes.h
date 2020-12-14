/*
 * Copyright (c) 2011 Matej Klonfar
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
 * Basic data structures for USB HID Report descriptor and report parser.
 */
#ifndef LIBUSB_HIDTYPES_H_
#define LIBUSB_HIDTYPES_H_

#include <stdint.h>
#include <adt/list.h>

/**
 * Maximum amount of specified usages for one report item
 */
#define USB_HID_MAX_USAGES	0xffff

/**
 * Converts integer from unsigned two's complement format format to signed
 * one.
 *
 * @param x Number to convert
 * @param size Length of the unsigned number in bites
 * @return signed int
 */
#define USB_HID_UINT32_TO_INT32(x, size)	\
	((((x) & (1 << ((size) - 1))) != 0) ?   \
	 -(~((x) - 1) & ((1 << size) - 1)) : (x))

/**
 * Convert integer from signed format to unsigned. If number is negative the
 * two's complement format is used.
 *
 * @param x Number to convert
 * @param size Length of result number in bites
 * @return unsigned int
 */
#define USB_HID_INT32_TO_UINT32(x, size)	\
	(((x) < 0 ) ? ((1 << (size)) + (x)) : (x))

/**
 * Enum of report types
 */
typedef enum {
	/** Input report. Data are sent from device to system */
	USB_HID_REPORT_TYPE_INPUT = 1,

	/** Output report. Data are sent from system to device */
	USB_HID_REPORT_TYPE_OUTPUT = 2,

	/** Feature report. Describes device configuration information that
	 * can be sent to the device
	 */
	USB_HID_REPORT_TYPE_FEATURE = 3
} usb_hid_report_type_t;

/**
 * Description of all reports described in one report descriptor.
 */
typedef struct {
	/** Count of available reports. */
	int report_count;

	/** List of description of reports. */
	list_t reports; /* of usb_hid_report_description_t */

	/** List of all used usage/collection paths. */
	list_t collection_paths;

	/** Length of list of usage paths. */
	int collection_paths_count;

	/** Flag whether report ids are used. */
	int use_report_ids;

	/** Report id of last parsed report. */
	uint8_t last_report_id;

} usb_hid_report_t;

/**
 * Description of one concrete report
 */
typedef struct {
	/** Report id. Zero when no report id is used. */
	uint8_t report_id;

	/** Type of report */
	usb_hid_report_type_t type;

	/** Bit length of the report */
	size_t bit_length;

	/** Number of items in report */
	size_t item_length;

	/** List of report items in report */
	list_t report_items;

	/** Link to usb_hid_report_t.reports list. */
	link_t reports_link;
} usb_hid_report_description_t;

/**
 * Description of one field/item in report
 */
typedef struct {
	/** Bit offset of the field */
	int offset;

	/** Bit size of the field */
	size_t size;

	/** Usage page. Zero when usage page can be changed. */
	uint16_t usage_page;

	/** Usage. Zero when usage can be changed. */
	uint16_t usage;

	/** Item's attributes */
	uint8_t item_flags;

	/** Usage/Collection path of the field. */
	usb_hid_report_path_t *collection_path;

	/**
	 * The lowest valid logical value (value with the device operates)
	 */
	int32_t logical_minimum;

	/**
	 * The greatest valid logical value
	 */
	int32_t logical_maximum;

	/**
	 * The lowest valid physical value (value with the system operates)
	 */
	int32_t physical_minimum;

	/** The greatest valid physical value */
	int32_t physical_maximum;

	/** The lowest valid usage index */
	int32_t usage_minimum;

	/** The greatest valid usage index */
	int32_t usage_maximum;

	/** Unit of the value */
	uint32_t unit;

	/** Unit exponent */
	uint32_t unit_exponent;

	/** Array of possible usages */
	uint32_t *usages;

	/** Size of the array of usages */
	size_t usages_count;

	/** Parsed value */
	int32_t value;

	/** Link to usb_hid_report_description_t.report_items list */
	link_t ritems_link;
} usb_hid_report_field_t;

/**
 * State table for report descriptor parsing
 */
typedef struct {
	/** report id */
	int32_t id;

	/** Extended usage page */
	uint16_t extended_usage_page;

	/** Array of usages specified for this item */
	uint32_t usages[USB_HID_MAX_USAGES];

	/** Length of usages array */
	int usages_count;

	/** Usage page */
	uint32_t usage_page;

	/** Minimum valid usage index */
	int32_t usage_minimum;

	/** Maximum valid usage index */
	int32_t usage_maximum;

	/** Minimum valid logical value */
	int32_t logical_minimum;

	/** Maximum valid logical value */
	int32_t logical_maximum;

	/** Length of the items in bits */
	int32_t size;

	/** Count of items */
	int32_t count;

	/**  Bit offset of the item in report */
	size_t offset;

	/** Unit exponent */
	int32_t unit_exponent;
	/** Unit of the value */
	int32_t unit;

	/** String index */
	uint32_t string_index;

	/** Minimum valid string index */
	uint32_t string_minimum;

	/** Maximum valid string index */
	uint32_t string_maximum;

	/** The designator index */
	uint32_t designator_index;

	/** Minimum valid designator value */
	uint32_t designator_minimum;

	/** Maximum valid designator value */
	uint32_t designator_maximum;

	/** Minimal valid physical value */
	int32_t physical_minimum;

	/** Maximal valid physical value */
	int32_t physical_maximum;

	/** Items attributes */
	uint8_t item_flags;

	/** Report type */
	usb_hid_report_type_t type;

	/** current collection path */
	usb_hid_report_path_t *usage_path;

	/** Unused */
	link_t link;

	int in_delimiter;
} usb_hid_report_item_t;

/**
 * Enum of the keyboard modifiers
 */
typedef enum {
	USB_HID_MOD_LCTRL = 0x01,
	USB_HID_MOD_LSHIFT = 0x02,
	USB_HID_MOD_LALT = 0x04,
	USB_HID_MOD_LGUI = 0x08,
	USB_HID_MOD_RCTRL = 0x10,
	USB_HID_MOD_RSHIFT = 0x20,
	USB_HID_MOD_RALT = 0x40,
	USB_HID_MOD_RGUI = 0x80,
	USB_HID_MOD_COUNT = 8
} usb_hid_modifiers_t;

static const usb_hid_modifiers_t
    usb_hid_modifiers_consts[USB_HID_MOD_COUNT] = {
	USB_HID_MOD_LCTRL,
	USB_HID_MOD_LSHIFT,
	USB_HID_MOD_LALT,
	USB_HID_MOD_LGUI,
	USB_HID_MOD_RCTRL,
	USB_HID_MOD_RSHIFT,
	USB_HID_MOD_RALT,
	USB_HID_MOD_RGUI
};

#endif
/**
 * @}
 */
