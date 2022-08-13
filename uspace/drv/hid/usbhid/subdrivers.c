/*
 * SPDX-FileCopyrightText: 2011 Lubos Slovak
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
