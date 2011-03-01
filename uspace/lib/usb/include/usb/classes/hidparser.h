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
 * @brief USB HID parser.
 */
#ifndef LIBUSB_HIDPARSER_H_
#define LIBUSB_HIDPARSER_H_

#include <stdint.h>
#include <adt/list.h>

#include <hid_report_items.h>

/**
 * Items prefix
 */
#define USB_HID_ITEM_SIZE(data) 	((uint8_t)(data & 0x3))
#define USB_HID_ITEM_TAG(data) 		((uint8_t)((data & 0xF0) >> 4))
#define USB_HID_ITEM_TAG_CLASS(data)	((uint8_t)((data & 0xC) >> 2))
#define USB_HID_ITEM_IS_LONG(data)	(data == 0xFE)


/**
 * Input/Output/Feature Item flags
 */
#define USB_HID_ITEM_FLAG_CONSTANT(flags) 	(flags & 0x1)
#define USB_HID_ITEM_FLAG_VARIABLE(flags) 	(flags & 0x2)
#define USB_HID_ITEM_FLAG_RELATIVE(flags) 	(flags & 0x4)
#define USB_HID_ITEM_FLAG_WRAP(flags)		(flags & 0x8)
#define USB_HID_ITEM_FLAG_LINEAR(flags)		(flags & 0x10)
#define USB_HID_ITEM_FLAG_PREFERRED(flags)	(flags & 0x20)
#define USB_HID_ITEM_FLAG_POSITION(flags)	(flags & 0x40)
#define USB_HID_ITEM_FLAG_VOLATILE(flags)	(flags & 0x80)
#define USB_HID_ITEM_FLAG_BUFFERED(flags)	(flags & 0x100)


/**
 * Collection Item Types
 */
#define USB_HID_COLLECTION_TYPE_PHYSICAL		0x00
#define USB_HID_COLLECTION_TYPE_APPLICATION		0x01
#define USB_HID_COLLECTION_TYPE_LOGICAL			0x02
#define USB_HID_COLLECTION_TYPE_REPORT			0x03
#define USB_HID_COLLECTION_TYPE_NAMED_ARRAY		0x04
#define USB_HID_COLLECTION_TYPE_USAGE_SWITCH	0x05

/*
 * modifiers definitions
 */
#define USB_HID_BOOT_KEYBOARD_NUM_LOCK		0x01
#define USB_HID_BOOT_KEYBOARD_CAPS_LOCK		0x02
#define USB_HID_BOOT_KEYBOARD_SCROLL_LOCK	0x04
#define USB_HID_BOOT_KEYBOARD_COMPOSE		0x08
#define USB_HID_BOOT_KEYBOARD_KANA			0x10


/**
 * Description of report items
 */
typedef struct {
	int32_t id;
	int32_t usage_page;
	int32_t	usage;	
	int32_t usage_minimum;
	int32_t usage_maximum;
	int32_t logical_minimum;
	int32_t logical_maximum;
	int32_t size;
	int32_t count;
	int32_t offset;

	int32_t unit_exponent;
	int32_t unit;

	/*
	 * some not yet used fields
	 */
	int32_t string_index;
	int32_t string_minimum;
	int32_t string_maximum;
	int32_t designator_index;
	int32_t designator_minimum;
	int32_t designator_maximum;
	int32_t physical_minimum;
	int32_t physical_maximum;

	uint8_t item_flags;

	link_t link;
} usb_hid_report_item_t;


/** HID report parser structure. */
typedef struct {	
	link_t input;
	link_t output;
	link_t feature;
} usb_hid_report_parser_t;	


/** HID parser callbacks for IN items. */
typedef struct {
	/** Callback for keyboard.
	 *
	 * @param key_codes Array of pressed key (including modifiers).
	 * @param count Length of @p key_codes.
	 * @param arg Custom argument.
	 */
	void (*keyboard)(const uint8_t *key_codes, size_t count, const uint8_t modifiers, void *arg);
} usb_hid_report_in_callbacks_t;

int usb_hid_boot_keyboard_input_report(const uint8_t *data, size_t size,
	const usb_hid_report_in_callbacks_t *callbacks, void *arg);

int usb_hid_boot_keyboard_output_report(uint8_t leds, uint8_t *data, size_t size);


int usb_hid_parser_init(usb_hid_report_parser_t *parser);
int usb_hid_parse_report_descriptor(usb_hid_report_parser_t *parser, 
    const uint8_t *data, size_t size);

int usb_hid_parse_report(const usb_hid_report_parser_t *parser,  
    const uint8_t *data, size_t size,
    const usb_hid_report_in_callbacks_t *callbacks, void *arg);


void usb_hid_free_report_parser(usb_hid_report_parser_t *parser);

void usb_hid_descriptor_print(usb_hid_report_parser_t *parser);
#endif
/**
 * @}
 */
