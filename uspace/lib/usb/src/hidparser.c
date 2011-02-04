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
 * @brief HID parser implementation.
 */
#include <usb/classes/hidparser.h>
#include <errno.h>
#include <stdio.h>

/** Parse HID report descriptor.
 *
 * @param parser Opaque HID report parser structure.
 * @param data Data describing the report.
 * @return Error code.
 */
int usb_hid_parse_report_descriptor(usb_hid_report_parser_t *parser, 
    const uint8_t *data, size_t size)
{
	return ENOTSUP;
}

/** Parse and act upon a HID report.
 *
 * @see usb_hid_parse_report_descriptor
 *
 * @param parser Opaque HID report parser structure.
 * @param data Data for the report.
 * @param callbacks Callbacks for report actions.
 * @param arg Custom argument (passed through to the callbacks).
 * @return Error code.
 */
int usb_hid_parse_report(const usb_hid_report_parser_t *parser,  
    const uint8_t *data, size_t size,
    const usb_hid_report_in_callbacks_t *callbacks, void *arg)
{
	int i;
	
	/* main parsing loop */
	while(0){
	}
	
	
	uint8_t keys[6];
	
	for (i = 0; i < 6; ++i) {
		keys[i] = data[i];
	}
	
	callbacks->keyboard(keys, 6, 0, arg);

	return EOK;
}

/** Free the HID report parser structure 
 *
 * @param parser Opaque HID report parser structure
 * @return Error code
 */
int usb_hid_free_report_parser(usb_hid_report_parser_t *parser)
{

	return EOK;
}


/**
 * Parse input report.
 *
 * @param data Data for report
 * @param size Size of report
 * @param callbacks Callbacks for report actions
 * @param arg Custom arguments
 *
 * @return Error code
 */
int usb_hid_boot_keyboard_input_report(const uint8_t *data, size_t size,
	const usb_hid_report_in_callbacks_t *callbacks, void *arg)
{
	int i;
	usb_hid_report_item_t item;

	/* fill item due to the boot protocol report descriptor */
	// modifier keys are in the first byte
	uint8_t modifiers = data[0];

	item.offset = 2; /* second byte is reserved */
	item.size = 8;
	item.count = 6;
	item.usage_min = 0;
	item.usage_max = 255;
	item.logical_min = 0;
	item.logical_max = 255;

	if (size != 8) {
		return ERANGE;
	}

	uint8_t keys[6];
	for (i = 0; i < item.count; i++) {
		keys[i] = data[i + item.offset];
	}

	callbacks->keyboard(keys, 6, modifiers, arg);
	return EOK;
}

/**
 * Makes output report for keyboard boot protocol
 *
 * @param leds
 * @param output Output report data buffer
 * @param size Size of the output buffer
 * @return Error code
 */
int usb_hid_boot_keyboard_output_report(uint8_t leds, uint8_t *data, size_t size)
{
	if(size != 1){
		return -1;
	}

	/* used only first five bits, others are only padding*/
	*data = leds;
	return EOK;
}

/**
 * @}
 */
