/*
 * Copyright (c) 2011 Lubos Slovak, Vojtech Horky
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
/**
 * @file
 * USB Logitech UltraX Keyboard sample driver.
 */


#include "lgtch-ultrax.h"
#include "../usbhid.h"

#include <usb/classes/hidparser.h>
#include <usb/debug.h>
#include <errno.h>
#include <str_error.h>

#define NAME "lgtch-ultrax"

/*----------------------------------------------------------------------------*/

static void usb_lgtch_process_keycodes(const uint8_t *key_codes, size_t count,
    uint8_t report_id, void *arg);

static const usb_hid_report_in_callbacks_t usb_lgtch_parser_callbacks = {
	.keyboard = usb_lgtch_process_keycodes
};

/*----------------------------------------------------------------------------*/

static void usb_lgtch_process_keycodes(const uint8_t *key_codes, size_t count,
    uint8_t report_id, void *arg)
{
	// TODO: checks
	
	usb_log_debug(NAME " Got keys from parser (report id: %u): %s\n", 
	    report_id, usb_debug_str_buffer(key_codes, count, 0));
}

/*----------------------------------------------------------------------------*/

bool usb_lgtch_polling_callback(struct usb_hid_dev *hid_dev, 
    uint8_t *buffer, size_t buffer_size)
{
	// TODO: checks
	
	usb_log_debug(NAME " usb_lgtch_polling_callback(%p, %p, %zu)\n",
	    hid_dev, buffer, buffer_size);

	usb_log_debug(NAME " Calling usb_hid_parse_report() with "
	    "buffer %s\n", usb_debug_str_buffer(buffer, buffer_size, 0));
	
	usb_hid_report_path_t *path = usb_hid_report_path();
	usb_hid_report_path_append_item(path, 0xc, 0);

	uint8_t report_id;
	
	int rc = usb_hid_parse_report(hid_dev->report, buffer, buffer_size, 
	    &report_id);
	usb_hid_report_path_set_report_id(path, report_id);

	usb_hid_report_field_t *field = usb_hid_report_get_sibling(
	    hid_dev->report, NULL, path, USB_HID_PATH_COMPARE_END , 
	    USB_HID_REPORT_TYPE_INPUT);
	
	while (field != NULL) {
		usb_log_debug("KEY VALUE(%X) USAGE(%X)\n", field->value, 
		    field->usage);
	}
	

	usb_hid_report_path_free(path);
	
	if (rc != EOK) {
		usb_log_warning("Error in usb_hid_boot_keyboard_input_report():"
		    "%s\n", str_error(rc));
	}
	
	return true;
}

/**
 * @}
 */
