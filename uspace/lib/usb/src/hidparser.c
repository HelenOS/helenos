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

/** @addtogroup libusb usb
 * @{
 */
/** @file
 * @brief HID parser implementation.
 */
#include <usb/classes/hidparser.h>
#include <errno.h>

/** Parse HID report descriptor.
 *
 * @param parser Opaque HID report parser structure.
 * @param data Data describing the report.
 * @param size Size of the descriptor in bytes.
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
 * @param size Size of the data in bytes.
 * @param callbacks Callbacks for report actions.
 * @param arg Custom argument (passed through to the callbacks).
 * @return Error code.
 */
int usb_hid_parse_report(const usb_hid_report_parser_t *parser,  
    const uint8_t *data, size_t size,
    const usb_hid_report_in_callbacks_t *callbacks, void *arg)
{
	int i;
	
	// TODO: parse report
	
	uint16_t keys[6];
	
	for (i = 0; i < 6; ++i) {
		keys[i] = data[i];
	}
	
	callbacks->keyboard(keys, 6, arg);
}


/**
 * @}
 */
