/*
 * Copyright (c) 2011 Vojtech Horky
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

/** @addtogroup usbvirthid
 * @{
 */
/** @file
 * Logitech wireless mouse-keyboard combo simulation (see issue 349).
 */
#include "../virthid.h"
#include <errno.h>
#include <usb/debug.h>
#include <usb/hid/hid.h>
#include <usb/hid/usages/core.h>

static uint8_t iface1_report_descriptor[] = {
	0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x85, 0x02, 0x09, 0x01,
	0xA1, 0x00, 0x05, 0x09, 0x19, 0x01, 0x29, 0x10, 0x15, 0x00,
	0x25, 0x01, 0x95, 0x10, 0x75, 0x01, 0x81, 0x02, 0x05, 0x01,
	0x16, 0x01, 0xF8, 0x26, 0xFF, 0x07, 0x75, 0x0C, 0x95, 0x02,
	0x09, 0x30, 0x09, 0x31, 0x81, 0x06, 0x15, 0x81, 0x25, 0x7F,
	0x75, 0x08, 0x95, 0x01, 0x09, 0x38, 0x81, 0x06, 0x05, 0x0C,
	0x0A, 0x38, 0x02, 0x95, 0x01, 0x81, 0x06, 0xC0, 0xC0, 0x05,
	0x0C, 0x09, 0x01, 0xA1, 0x01, 0x85, 0x03, 0x75, 0x10, 0x95,
	0x02, 0x15, 0x01, 0x26, 0x8C, 0x02, 0x19, 0x01, 0x2A, 0x8C,
	0x02, 0x81, 0x00, 0xC0, 0x05, 0x01, 0x09, 0x80, 0xA1, 0x01,
	0x85, 0x04, 0x75, 0x02, 0x95, 0x01, 0x15, 0x01, 0x25, 0x03,
	0x09, 0x82, 0x09, 0x81, 0x09, 0x83, 0x81, 0x60, 0x75, 0x06,
	0x81, 0x03, 0xC0, 0x06, 0xBC, 0xFF, 0x09, 0x88, 0xA1, 0x01,
	0x85, 0x08, 0x19, 0x01, 0x29, 0xFF, 0x15, 0x01, 0x26, 0xFF,
	0x00, 0x75, 0x08, 0x95, 0x01, 0x81, 0x00, 0xC0
};
#define iface1_input_size 8
static uint8_t iface1_in_data[] = {
	0, 0, 0, 0, 0, 0, 0, 0
};

static vuhid_interface_life_t iface1_life = {
	.data_in = iface1_in_data,
	.data_in_count = sizeof(iface1_in_data) / iface1_input_size,
	.data_in_pos_change_delay = 50,
	.msg_born = "Mouse of Logitech Unifying Receiver comes to life...",
	.msg_die = "Mouse of Logitech Unifying Receiver disconnected."
};

vuhid_interface_t vuhid_interface_logitech_wireless_1 = {
	.id = "lw1",
	.name = "Logitech Unifying Receiver, interface 1 (mouse)",
	.usb_subclass = USB_HID_SUBCLASS_BOOT,
	.usb_protocol = USB_HID_PROTOCOL_MOUSE,

	.report_descriptor = iface1_report_descriptor,
	.report_descriptor_size = sizeof(iface1_report_descriptor),

	.in_data_size = iface1_input_size,
	.on_data_in = interface_live_on_data_in,

	.out_data_size = 0,
	.on_data_out = NULL,

	.live = interface_life_live,

	.interface_data = &iface1_life,
	.vuhid_data = NULL
};

/**
 * @}
 */
