/*
 * SPDX-FileCopyrightText: 2011 Lubos Slovak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbhid
 * @{
 */
/** @file
 * UUSB multimedia key to keycode mapping.
 */

#include <io/keycode.h>
#include <stdint.h>
#include <stdio.h>
#include <usb/debug.h>
#include "keymap.h"

/**
 * Mapping between USB HID multimedia usages (from HID Usage Tables) and
 * corresponding HelenOS key codes.
 *
 * Currently only Usages used by Logitech UltraX keyboard are present. All other
 * should result in 0.
 */
static int usb_hid_keymap_consumer[0x29c] = {
	[0xb5] = 0,       /* Scan Next Track */
	[0xb6] = 0,       /* Scan Previous Track */
	[0xb7] = 0,       /* Stop */
	[0xb8] = 0,       /* Eject */
	[0xcd] = 0,       /* Play/Pause */
	[0xe2] = 0,       /* Mute */
	[0xe9] = 0,       /* Volume Increment */
	[0xea] = 0,       /* Volume Decrement */
	[0x183] = 0,      /* AL Consumer Control Configuration */
	[0x18a] = 0,      /* AL Email Reader */
	[0x192] = 0,      /* AL Calculator */
	[0x221] = 0,      /* AC Search */
	[0x223] = 0,      /* AC Home */
	[0x224] = 0,      /* AC Back */
	[0x225] = 0,      /* AC Forward */
	[0x226] = 0,      /* AC Stop */
	[0x227] = 0,      /* AC Refresh */
	[0x22a] = 0       /* AC Bookmarks */
};

/**
 * Translates USB HID Usages from the Consumer Page into HelenOS keycodes.
 *
 * @param usage USB HID Consumer Page Usage number.
 *
 * @retval HelenOS key code corresponding to the given USB Consumer Page Usage.
 */
unsigned int usb_multimedia_map_usage(int usage)
{
	unsigned int key;
	int *map = usb_hid_keymap_consumer;
	size_t map_length = sizeof(usb_hid_keymap_consumer) / sizeof(int);

	if ((usage < 0) || ((size_t)usage >= map_length))
		return -1;

	/* TODO What if the usage is not in the table? */
	key = map[usage];

	return key;
}

/**
 * @}
 */
