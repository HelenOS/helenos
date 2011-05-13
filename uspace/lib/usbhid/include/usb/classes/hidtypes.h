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
 * USB HID report descriptor and report data parser
 */
#ifndef LIBUSB_HIDTYPES_H_
#define LIBUSB_HIDTYPES_H_

#include <stdint.h>
#include <adt/list.h>

#define USB_HID_MAX_USAGES	20

#define USB_HID_UINT32_TO_INT32(x, size)	((((x) & (1 << ((size) - 1))) != 0) ? -(~(x - 1) & ((1 << size) - 1)) : (x)) //(-(~((x) - 1)))
#define USB_HID_INT32_TO_UINT32(x, size)	(((x) < 0 ) ? ((1 << (size)) + (x)) : (x))


typedef enum {
	USB_HID_REPORT_TYPE_INPUT = 1,
	USB_HID_REPORT_TYPE_OUTPUT = 2,
	USB_HID_REPORT_TYPE_FEATURE = 3
} usb_hid_report_type_t;


typedef struct {
	/** */
	int report_count;
	link_t reports;		/** list of usb_hid_report_description_t */

	link_t collection_paths;
	int collection_paths_count;

	int use_report_ids;
	uint8_t last_report_id;
	
} usb_hid_report_t;

typedef struct {
	uint8_t report_id;
	usb_hid_report_type_t type;

	size_t bit_length;
	size_t item_length;
	
	link_t report_items;	/** list of report items (fields) */

	link_t link;
} usb_hid_report_description_t;

typedef struct {

	int offset;
	size_t size;

	uint16_t usage_page;
	uint16_t usage;

	uint8_t item_flags;
	usb_hid_report_path_t *collection_path;

	int32_t logical_minimum;
	int32_t logical_maximum;
	int32_t physical_minimum;
	int32_t physical_maximum;
	uint32_t usage_minimum;
	uint32_t usage_maximum;
	uint32_t unit;
	uint32_t unit_exponent;
	

	int32_t value;

	link_t link;
} usb_hid_report_field_t;



/**
 * state table
 */
typedef struct {
	/** report id */	
	int32_t id;
	
	/** */
	uint16_t extended_usage_page;
	uint32_t usages[USB_HID_MAX_USAGES];
	int usages_count;

	/** */
	uint32_t usage_page;

	/** */	
	uint32_t usage_minimum;
	/** */	
	uint32_t usage_maximum;
	/** */	
	int32_t logical_minimum;
	/** */	
	int32_t logical_maximum;
	/** */	
	int32_t size;
	/** */	
	int32_t count;
	/** */	
	size_t offset;
	/** */	
	int32_t unit_exponent;
	/** */	
	int32_t unit;

	/** */
	uint32_t string_index;
	/** */	
	uint32_t string_minimum;
	/** */	
	uint32_t string_maximum;
	/** */	
	uint32_t designator_index;
	/** */	
	uint32_t designator_minimum;
	/** */	
	uint32_t designator_maximum;
	/** */	
	int32_t physical_minimum;
	/** */	
	int32_t physical_maximum;

	/** */	
	uint8_t item_flags;

	usb_hid_report_type_t type;

	/** current collection path*/	
	usb_hid_report_path_t *usage_path;
	/** */	
	link_t link;

	int in_delimiter;
} usb_hid_report_item_t;

/** HID parser callbacks for IN items. */
typedef struct {
	/** Callback for keyboard.
	 *
	 * @param key_codes Array of pressed key (including modifiers).
	 * @param count Length of @p key_codes.
	 * @param arg Custom argument.
	 */
	void (*keyboard)(const uint8_t *key_codes, size_t count, const uint8_t report_id, void *arg);
} usb_hid_report_in_callbacks_t;


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
