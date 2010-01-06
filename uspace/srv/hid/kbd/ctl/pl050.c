/*
 * Copyright (c) 2009 Vineeth Pillai
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

/** @addtogroup kbd_ctl
 * @ingroup kbd
 * @{
 */
/**
 * @file
 * @brief PL050 keyboard controller driver.
 */

#include <kbd.h>
#include <io/console.h>
#include <io/keycode.h>
#include <kbd_ctl.h>
#include <gsp.h>
#include <stdio.h>

#define PL050_CAPS_SCAN_CODE 0x58
#define PL050_NUM_SCAN_CODE 0x77
#define PL050_SCROLL_SCAN_CODE 0x7E

static bool is_lock_key(int);
enum dec_state {
	ds_s,
	ds_e
};

static enum dec_state ds;

static int scanmap_simple[] = {

	[0x0e] = KC_BACKTICK,

	[0x16] = KC_1,
	[0x1e] = KC_2,
	[0x26] = KC_3,
	[0x25] = KC_4,
	[0x2e] = KC_5,
	[0x36] = KC_6,
	[0x3d] = KC_7,
	[0x3e] = KC_8,
	[0x46] = KC_9,
	[0x45] = KC_0,

	[0x4e] = KC_MINUS,
	[0x55] = KC_EQUALS,
	[0x66] = KC_BACKSPACE,

	[0x0d] = KC_TAB,

	[0x15] = KC_Q,
	[0x1d] = KC_W,
	[0x24] = KC_E,
	[0x2d] = KC_R,
	[0x2c] = KC_T,
	[0x35] = KC_Y,
	[0x3c] = KC_U,
	[0x43] = KC_I,
	[0x44] = KC_O,
	[0x4d] = KC_P,

	[0x54] = KC_LBRACKET,
	[0x5b] = KC_RBRACKET,

	[0x58] = KC_CAPS_LOCK,

	[0x1c] = KC_A,
	[0x1b] = KC_S,
	[0x23] = KC_D,
	[0x2b] = KC_F,
	[0x34] = KC_G,
	[0x33] = KC_H,
	[0x3b] = KC_J,
	[0x42] = KC_K,
	[0x4b] = KC_L,

	[0x4c] = KC_SEMICOLON,
	[0x52] = KC_QUOTE,
	[0x5d] = KC_BACKSLASH,

	[0x12] = KC_LSHIFT,

	[0x1a] = KC_Z,
	[0x22] = KC_X,
	[0x21] = KC_C,
	[0x2a] = KC_V,
	[0x32] = KC_B,
	[0x31] = KC_N,
	[0x3a] = KC_M,

	[0x41] = KC_COMMA,
	[0x49] = KC_PERIOD,
	[0x4a] = KC_SLASH,

	[0x59] = KC_RSHIFT,

	[0x14] = KC_LCTRL,
	[0x11] = KC_LALT,
	[0x29] = KC_SPACE,

	[0x76] = KC_ESCAPE,

	[0x05] = KC_F1,
	[0x06] = KC_F2,
	[0x04] = KC_F3,
	[0x0c] = KC_F4,
	[0x03] = KC_F5,
	[0x0b] = KC_F6,
	[0x02] = KC_F7,

	[0x0a] = KC_F8,
	[0x01] = KC_F9,
	[0x09] = KC_F10,

	[0x78] = KC_F11,
	[0x07] = KC_F12,

	[0x60] = KC_SCROLL_LOCK,

	[0x5a] = KC_ENTER,

	[0x77] = KC_NUM_LOCK,
	[0x7c] = KC_NTIMES,
	[0x7b] = KC_NMINUS,
	[0x79] = KC_NPLUS,
	[0x6c] = KC_N7,
	[0x75] = KC_N8,
	[0x7d] = KC_N9,
	[0x6b] = KC_N4,
	[0x73] = KC_N5,
	[0x74] = KC_N6,
	[0x69] = KC_N1,
	[0x72] = KC_N2,
	[0x7a] = KC_N3,
	[0x70] = KC_N0,
	[0x71] = KC_NPERIOD
};

static int scanmap_e0[] = {
	[0x65] = KC_RALT,
	[0x59] = KC_RSHIFT,

	[0x64] = KC_PRTSCR,

	[0x70] = KC_INSERT,
	[0x6c] = KC_HOME,
	[0x7d] = KC_PAGE_UP,

	[0x71] = KC_DELETE,
	[0x69] = KC_END,
	[0x7a] = KC_PAGE_DOWN,

	[0x75] = KC_UP,
	[0x6b] = KC_LEFT,
	[0x72] = KC_DOWN,
	[0x74] = KC_RIGHT,

	[0x4a] = KC_NSLASH,
	[0x5a] = KC_NENTER
};

int kbd_ctl_init(void)
{
	ds = ds_s;
	return 0;
}

void kbd_ctl_parse_scancode(int scancode)
{
	static int key_release_flag = 0;
	static int is_locked = 0;
	console_ev_type_t type;
	unsigned int key;
	int *map;
	size_t map_length;

	if (scancode == 0xe0) {
		ds = ds_e;
		return;
	}

	switch (ds) {
	case ds_s:
		map = scanmap_simple;
		map_length = sizeof(scanmap_simple) / sizeof(int);
		break;
	case ds_e:
		map = scanmap_e0;
		map_length = sizeof(scanmap_e0) / sizeof(int);
		break;
	default:
		map = NULL;
		map_length = 0;
	}

	ds = ds_s;
	if (scancode == 0xf0) {
		key_release_flag = 1;
		return;
	} else {
		if (key_release_flag) {
			type = KEY_RELEASE;
			key_release_flag = 0;
			if (is_lock_key(scancode)) {
				if (!is_locked) {
					is_locked = 1;
				} else {
					is_locked = 0;
					return;
				}
			}
		} else {
			if (is_lock_key(scancode) && is_locked)
				return;
			type = KEY_PRESS;
		}
	}

	if (scancode < 0)
		return;

	key = map[scancode];
	if (key != 0)
		kbd_push_ev(type, key);
}

static bool is_lock_key(int sc)
{
	return ((sc == PL050_CAPS_SCAN_CODE) || (sc == PL050_NUM_SCAN_CODE) ||
	    (sc == PL050_SCROLL_SCAN_CODE));
}

void kbd_ctl_set_ind(unsigned mods)
{
	(void) mods;
}

/**
 * @}
 */ 
