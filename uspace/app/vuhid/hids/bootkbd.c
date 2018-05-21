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
 *
 */
#include "../virthid.h"
#include <errno.h>
#include <usb/debug.h>
#include <usb/hid/hid.h>
#include <usb/hid/usages/core.h>

#include "../report.h"

uint8_t report_descriptor[] = {
	STD_USAGE_PAGE(USB_HIDUT_PAGE_GENERIC_DESKTOP),
	USAGE1(USB_HIDUT_USAGE_GENERIC_DESKTOP_KEYBOARD),
	START_COLLECTION(COLLECTION_APPLICATION),

	STD_USAGE_PAGE(USB_HIDUT_PAGE_KEYBOARD),
	USAGE_MINIMUM1(224),
	USAGE_MAXIMUM1(231),
	LOGICAL_MINIMUM1(0),
	LOGICAL_MAXIMUM1(1),
	REPORT_SIZE1(1),
	REPORT_COUNT1(8),
	/* Modifiers */
	INPUT(IOF_DATA | IOF_VARIABLE | IOF_ABSOLUTE),
	REPORT_COUNT1(1),
	REPORT_SIZE1(8),
	/* Reserved */
	INPUT(IOF_CONSTANT),
	REPORT_COUNT1(5),
	REPORT_SIZE1(1),
	STD_USAGE_PAGE(USB_HIDUT_PAGE_LED),
	USAGE_MINIMUM1(1),
	USAGE_MAXIMUM1(5),
	/* LED states */
	OUTPUT(IOF_DATA | IOF_VARIABLE | IOF_ABSOLUTE),
	REPORT_COUNT1(1),
	REPORT_SIZE1(3),
	/* LED states padding */
	OUTPUT(IOF_CONSTANT),
	REPORT_COUNT1(6),
	REPORT_SIZE1(8),
	LOGICAL_MINIMUM1(0),
	LOGICAL_MAXIMUM1(101),
	STD_USAGE_PAGE(USB_HIDUT_PAGE_KEYBOARD),
	USAGE_MINIMUM1(0),
	USAGE_MAXIMUM1(101),
	/* Key array */
	INPUT(IOF_DATA | IOF_ARRAY),

	END_COLLECTION()
};

#define INPUT_SIZE 8

static uint8_t in_data[] = {
	0,      0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0,      0, 0x39, 0x00, 0x00, 0x00, 0x00, 0x00, // Caps Lock
	0,      0, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00, // Num Lock
	0,      0, 0x39, 0x00, 0x00, 0x00, 0x00, 0x00, // Caps Lock
	1 << 2, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	1 << 2, 0, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00,
	1 << 2, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0,      0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static vuhid_interface_life_t boot_life = {
	.data_in = in_data,
	.data_in_count = sizeof(in_data) / INPUT_SIZE,
	.data_in_pos_change_delay = 500,
	.msg_born = "Boot keyboard comes to life...",
	.msg_die = "Boot keyboard died."
};

static errno_t on_data_out(vuhid_interface_t *iface,
    const void *buffer, size_t buffer_size)
{
	if (buffer_size == 0) {
		return EEMPTY;
	}
	uint8_t leds = ((uint8_t *) buffer)[0];
#define _GET_LED(index, signature) \
	(((leds) & (1 << index)) ? (signature) : '-')
	usb_log_info("%s: LEDs = %c%c%c%c%c",
	    iface->name,
	    _GET_LED(0, '0'), _GET_LED(1, 'A'), _GET_LED(2, 's'),
	    _GET_LED(3, 'c'), _GET_LED(4, 'k'));
#undef _GET_LED
	return EOK;
}

vuhid_interface_t vuhid_interface_bootkbd = {
	.id = "boot",
	.name = "boot keyboard",
	.usb_subclass = USB_HID_SUBCLASS_BOOT,
	.usb_protocol = USB_HID_PROTOCOL_KEYBOARD,

	.report_descriptor = report_descriptor,
	.report_descriptor_size = sizeof(report_descriptor),

	.in_data_size = INPUT_SIZE,
	.on_data_in = interface_live_on_data_in,

	.out_data_size = 1,
	.on_data_out = on_data_out,

	.live = interface_life_live,

	.interface_data = &boot_life,
	.vuhid_data = NULL
};

/**
 * @}
 */
