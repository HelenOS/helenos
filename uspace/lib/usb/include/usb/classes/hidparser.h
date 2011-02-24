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

/**
 * Description of report items
 */
typedef struct {

	uint8_t usage_min;
	uint8_t usage_max;
	uint8_t logical_min;
	uint8_t logical_max;
	uint8_t size;
	uint8_t count;
	uint8_t offset;

} usb_hid_report_item_t;


/** HID report parser structure. */
typedef struct {
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

//#define USB_HID_BOOT_KEYBOARD_NUM_LOCK		0x01
//#define USB_HID_BOOT_KEYBOARD_CAPS_LOCK		0x02
//#define USB_HID_BOOT_KEYBOARD_SCROLL_LOCK	0x04
//#define USB_HID_BOOT_KEYBOARD_COMPOSE		0x08
//#define USB_HID_BOOT_KEYBOARD_KANA			0x10

/*
 * modifiers definitions
 */

int usb_hid_boot_keyboard_input_report(const uint8_t *data, size_t size,
	const usb_hid_report_in_callbacks_t *callbacks, void *arg);

int usb_hid_boot_keyboard_output_report(uint8_t leds, uint8_t *data, size_t size);

int usb_hid_parse_report_descriptor(usb_hid_report_parser_t *parser, 
    const uint8_t *data, size_t size);

int usb_hid_parse_report(const usb_hid_report_parser_t *parser,  
    const uint8_t *data, size_t size,
    const usb_hid_report_in_callbacks_t *callbacks, void *arg);


int usb_hid_free_report_parser(usb_hid_report_parser_t *parser);

#endif
/**
 * @}
 */
