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
 * USB scancode parser.
 */

#include <io/keycode.h>
#include <stdint.h>
#include <stdio.h>
#include <usb/debug.h>
#include "conv.h"

/**
 * Mapping between USB HID key codes (from HID Usage Tables) and corresponding
 * HelenOS key codes.
 */
static int scanmap_simple[255] = {
	[0x04] = KC_A,
	[0x05] = KC_B,
	[0x06] = KC_C,
	[0x07] = KC_D,
	[0x08] = KC_E,
	[0x09] = KC_F,
	[0x0a] = KC_G,
	[0x0b] = KC_H,
	[0x0c] = KC_I,
	[0x0d] = KC_J,
	[0x0e] = KC_K,
	[0x0f] = KC_L,
	[0x10] = KC_M,
	[0x11] = KC_N,
	[0x12] = KC_O,
	[0x13] = KC_P,
	[0x14] = KC_Q,
	[0x15] = KC_R,
	[0x16] = KC_S,
	[0x17] = KC_T,
	[0x18] = KC_U,
	[0x19] = KC_V,
	[0x1a] = KC_W,
	[0x1b] = KC_X,
	[0x1c] = KC_Y,
	[0x1d] = KC_Z,

	[0x1e] = KC_1,
	[0x1f] = KC_2,
	[0x20] = KC_3,
	[0x21] = KC_4,
	[0x22] = KC_5,
	[0x23] = KC_6,
	[0x24] = KC_7,
	[0x25] = KC_8,
	[0x26] = KC_9,
	[0x27] = KC_0,

	[0x28] = KC_ENTER,
	[0x29] = KC_ESCAPE,
	[0x2a] = KC_BACKSPACE,
	[0x2b] = KC_TAB,
	[0x2c] = KC_SPACE,

	[0x2d] = KC_MINUS,
	[0x2e] = KC_EQUALS,
	[0x2f] = KC_LBRACKET,
	[0x30] = KC_RBRACKET,
	[0x31] = KC_BACKSLASH,
	[0x32] = KC_HASH,
	[0x33] = KC_SEMICOLON,
	[0x34] = KC_QUOTE,
	[0x35] = KC_BACKTICK,
	[0x36] = KC_COMMA,
	[0x37] = KC_PERIOD,
	[0x38] = KC_SLASH,

	[0x39] = KC_CAPS_LOCK,

	[0x3a] = KC_F1,
	[0x3b] = KC_F2,
	[0x3c] = KC_F3,
	[0x3d] = KC_F4,
	[0x3e] = KC_F5,
	[0x3f] = KC_F6,
	[0x40] = KC_F7,
	[0x41] = KC_F8,
	[0x42] = KC_F9,
	[0x43] = KC_F10,
	[0x44] = KC_F11,
	[0x45] = KC_F12,

	[0x46] = KC_PRTSCR,
	[0x47] = KC_SCROLL_LOCK,
	[0x48] = KC_PAUSE,
	[0x49] = KC_INSERT,
	[0x4a] = KC_HOME,
	[0x4b] = KC_PAGE_UP,
	[0x4c] = KC_DELETE,
	[0x4d] = KC_END,
	[0x4e] = KC_PAGE_DOWN,
	[0x4f] = KC_RIGHT,
	[0x50] = KC_LEFT,
	[0x51] = KC_DOWN,
	[0x52] = KC_UP,

	[0x53] = KC_NUM_LOCK,
	[0x54] = KC_NSLASH,
	[0x55] = KC_NTIMES,
	[0x56] = KC_NMINUS,
	[0x57] = KC_NPLUS,
	[0x58] = KC_NENTER,
	[0x59] = KC_N1,
	[0x5a] = KC_N2,
	[0x5b] = KC_N3,
	[0x5c] = KC_N4,
	[0x5d] = KC_N5,
	[0x5e] = KC_N6,
	[0x5f] = KC_N7,
	[0x60] = KC_N8,
	[0x61] = KC_N9,
	[0x62] = KC_N0,
	[0x63] = KC_NPERIOD,

	[0x64] = KC_BACKSLASH,

	[0x9a] = KC_SYSREQ,

	[0xe0] = KC_LCTRL,
	[0xe1] = KC_LSHIFT,
	[0xe2] = KC_LALT,
	[0xe4] = KC_RCTRL,
	[0xe5] = KC_RSHIFT,
	[0xe6] = KC_RALT,
};

/**
 * Translate USB HID key codes (from HID Usage Tables) to generic key codes
 * recognized by HelenOS.
 *
 * @param scancode USB HID key code (from HID Usage Tables).
 *
 * @retval HelenOS key code corresponding to the given USB HID key code.
 */
unsigned int usbhid_parse_scancode(int scancode)
{
	unsigned int key;
	int *map = scanmap_simple;
	size_t map_length = sizeof(scanmap_simple) / sizeof(int);

	if ((scancode < 0) || ((size_t) scancode >= map_length))
		return -1;

	key = map[scancode];

	return key;
}

/**
 * @}
 */
