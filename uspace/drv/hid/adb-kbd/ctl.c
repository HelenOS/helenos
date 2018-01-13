/*
 * Copyright (c) 2011 Jiri Svoboda
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

/**
 * @file
 * @brief Apple ADB keyboard controller
 */

#include <errno.h>
#include <io/console.h>
#include <io/keycode.h>

#include "ctl.h"

#define KBD_KEY_RELEASE  0x80

static unsigned int scanmap[] = {
	[0x00] = KC_A,
	[0x01] = KC_S,
	[0x02] = KC_D,
	[0x03] = KC_F,
	[0x04] = KC_H,
	[0x05] = KC_G,
	[0x06] = KC_Z,
	[0x07] = KC_X,
	[0x08] = KC_C,
	[0x09] = KC_V,
	[0x0a] = KC_BACKSLASH,
	[0x0b] = KC_B,
	[0x0c] = KC_Q,
	[0x0d] = KC_W,
	[0x0e] = KC_E,
	[0x0f] = KC_R,
	[0x10] = KC_Y,
	[0x11] = KC_T,
	[0x12] = KC_1,
	[0x13] = KC_2,
	[0x14] = KC_3,
	[0x15] = KC_4,
	[0x16] = KC_6,
	[0x17] = KC_5,
	[0x18] = KC_EQUALS,
	[0x19] = KC_9,
	[0x1a] = KC_7,
	[0x1b] = KC_MINUS,
	[0x1c] = KC_8,
	[0x1d] = KC_0,
	[0x1e] = KC_RBRACKET,
	[0x1f] = KC_O,
	[0x20] = KC_U,
	[0x21] = KC_LBRACKET,
	[0x22] = KC_I,
	[0x23] = KC_P,
	[0x24] = KC_ENTER,
	[0x25] = KC_L,
	[0x26] = KC_J,
	[0x27] = KC_QUOTE,
	[0x28] = KC_K,
	[0x29] = KC_SEMICOLON,
	[0x2a] = KC_BACKSLASH,
	[0x2b] = KC_COMMA,
	[0x2c] = KC_SLASH,
	[0x2d] = KC_N,
	[0x2e] = KC_M,
	[0x2f] = KC_PERIOD,
	[0x30] = KC_TAB,
	[0x31] = KC_SPACE,
	[0x32] = KC_BACKTICK,
	[0x33] = KC_BACKSPACE,
	[0x34] = 0,
	[0x35] = KC_ESCAPE,
	[0x36] = KC_LCTRL,
	[0x37] = 0,
	[0x38] = KC_LSHIFT,
	[0x39] = KC_CAPS_LOCK,
	[0x3a] = KC_LALT,
	[0x3b] = KC_LEFT,
	[0x3c] = KC_RIGHT,
	[0x3d] = KC_DOWN,
	[0x3e] = KC_UP,
	[0x3f] = 0,
	[0x40] = 0,
	[0x41] = KC_NPERIOD,
	[0x42] = 0,
	[0x43] = KC_NTIMES,
	[0x44] = 0,
	[0x45] = KC_NPLUS,
	[0x46] = 0,
	[0x47] = KC_NUM_LOCK,
	[0x48] = 0,
	[0x49] = 0,
	[0x4a] = 0,
	[0x4b] = KC_NSLASH,
	[0x4c] = KC_NENTER,
	[0x4d] = 0,
	[0x4e] = KC_NMINUS,
	[0x4f] = 0,
	[0x50] = 0,
	[0x51] = 0,
	[0x52] = KC_N0,
	[0x53] = KC_N1,
	[0x54] = KC_N2,
	[0x55] = KC_N3,
	[0x56] = KC_N4,
	[0x57] = KC_N5,
	[0x58] = KC_N6,
	[0x59] = KC_N7,
	[0x5a] = 0,
	[0x5b] = KC_N8,
	[0x5c] = KC_N9,
	[0x5d] = 0,
	[0x5e] = 0,
	[0x5f] = 0,
	[0x60] = KC_F5,
	[0x61] = KC_F6,
	[0x62] = KC_F7,
	[0x63] = KC_F3,
	[0x64] = KC_F8,
	[0x65] = KC_F9,
	[0x66] = 0,
	[0x67] = KC_F11,
	[0x68] = 0,
	[0x69] = KC_SYSREQ,
	[0x6a] = 0,
	[0x6b] = KC_SCROLL_LOCK,
	[0x6c] = 0,
	[0x6d] = KC_F10,
	[0x6e] = 0,
	[0x6f] = KC_F12,
	[0x70] = 0,
	[0x71] = KC_PAUSE,
	[0x72] = KC_INSERT,
	[0x73] = KC_HOME,
	[0x74] = KC_PAGE_UP,
	[0x75] = KC_DELETE,
	[0x76] = KC_F4,
	[0x77] = KC_END,
	[0x78] = KC_F2,
	[0x79] = KC_PAGE_DOWN,
	[0x7a] = KC_F1,
	[0x7b] = KC_RSHIFT,
	[0x7c] = KC_RALT,
	[0x7d] = KC_RCTRL,
	[0x7e] = 0,
	[0x7f] = 0
};

/** Translate ADB keyboard scancode into keyboard event.
 *
 * @param scancode Scancode
 * @param rtype Place to store type of keyboard event (press or release)
 * @param rkey Place to store key code
 *
 * @return EOK on success, ENOENT if no translation exists
 */
errno_t adb_kbd_key_translate(sysarg_t scancode, kbd_event_type_t *rtype,
    unsigned int *rkey)
{
	kbd_event_type_t etype;
	unsigned int key;

	if (scancode & KBD_KEY_RELEASE) {
		scancode &= ~KBD_KEY_RELEASE;
		etype = KEY_RELEASE;
	} else {
		etype = KEY_PRESS;
	}

	if (scancode >= sizeof(scanmap) / sizeof(unsigned int))
		return ENOENT;

	key = scanmap[scancode];
	if (key == 0)
		return ENOENT;

	*rtype = etype;
	*rkey = key;
	return EOK;
}

/** @}
 */
