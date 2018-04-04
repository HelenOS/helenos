/*
 * Copyright (c) 2011 Lubos Slovak
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
/** @file
 * USB HID subdriver mappings.
 */

#include "subdrivers.h"
#include <usb/hid/usages/core.h>
#include <usb/hid/hidpath.h>
#include "kbd/kbddev.h"
#include "mouse/mousedev.h"
#include "multimedia/multimedia.h"
#include "blink1/blink1.h"
#include "generic/hiddev.h"

static const usb_hid_subdriver_usage_t path_kbd[] = {
	{
		USB_HIDUT_PAGE_GENERIC_DESKTOP,
		USB_HIDUT_USAGE_GENERIC_DESKTOP_KEYBOARD
	},
	{ 0, 0 }
};

static const usb_hid_subdriver_usage_t path_mouse[] = {
	{
		USB_HIDUT_PAGE_GENERIC_DESKTOP,
		USB_HIDUT_USAGE_GENERIC_DESKTOP_MOUSE
	},
	{ 0, 0 }
};

static const usb_hid_subdriver_usage_t path_multim_key[] = {
	{
		USB_HIDUT_PAGE_CONSUMER,
		USB_HIDUT_USAGE_CONSUMER_CONSUMER_CONTROL
	},
	{ 0, 0 }
};

const usb_hid_subdriver_mapping_t usb_hid_subdrivers[] = {
	{
		path_kbd,
		0,
		USB_HID_PATH_COMPARE_BEGIN,
		-1,
		-1,
		{
			.init = usb_kbd_init,
			.deinit = usb_kbd_deinit,
			.poll = usb_kbd_polling_callback,
			.poll_end = NULL
		},
	},
	{
		path_multim_key,
		1,
		USB_HID_PATH_COMPARE_BEGIN,
		-1,
		-1,
		{
			.init = usb_multimedia_init,
			.deinit = usb_multimedia_deinit,
			.poll = usb_multimedia_polling_callback,
			.poll_end = NULL
		}
	},
	{
		path_mouse,
		0,
		USB_HID_PATH_COMPARE_BEGIN,
		-1,
		-1,
		{
			.init = usb_mouse_init,
			.deinit = usb_mouse_deinit,
			.poll = usb_mouse_polling_callback,
			.poll_end = NULL
		}
	},
	{
		NULL,
		0,
		USB_HID_PATH_COMPARE_BEGIN,
		0x27b8,
		0x01ed,
		{
			.init = usb_blink1_init,
			.deinit = usb_blink1_deinit,
			.poll = NULL,
			.poll_end = NULL
		}
	}
};

const size_t USB_HID_MAX_SUBDRIVERS =
    sizeof(usb_hid_subdrivers) / sizeof(usb_hid_subdrivers[0]);

/**
 * @}
 */
