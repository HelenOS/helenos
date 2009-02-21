/*
 * Copyright (c) 2009 Jiri Svoboda
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
 * @brief	Serial TTY-like keyboard controller driver.
 */

#include <kbd.h>
#include <kbd/kbd.h>
#include <kbd/keycode.h>
#include <kbd_ctl.h>

static void parse_ds_start(int scancode);
static void parse_ds_e(int scancode);
static void parse_ds_e1(int scancode);
static void parse_ds_e2(int scancode);
static void parse_ds_e2a(int scancode);
static void parse_ds_e2b(int scancode);

static void parse_leaf(int scancode, int (*map)[2], size_t map_length);

enum dec_state {
	ds_start,
	ds_e,
	ds_e1,
	ds_e2,
	ds_e2a,
	ds_e2b
};

static int map_start[][2] = {

	[0x60] = { 0, KC_BACKTICK },

	[0x31] = { 0, KC_1 },
	[0x32] = { 0, KC_2 },
	[0x33] = { 0, KC_3 },
	[0x34] = { 0, KC_4 },
	[0x35] = { 0, KC_5 },
	[0x36] = { 0, KC_6 },
	[0x37] = { 0, KC_7 },
	[0x38] = { 0, KC_8 },
	[0x39] = { 0, KC_9 },
	[0x30] = { 0, KC_0 },

	[0x2d] = { 0, KC_MINUS },
	[0x3d] = { 0, KC_EQUALS },
	[0x08] = { 0, KC_BACKSPACE },

	[0x0f] = { 0, KC_TAB },

	[0x71] = { 0, KC_Q },
	[0x77] = { 0, KC_W },
	[0x65] = { 0, KC_E },
	[0x72] = { 0, KC_R },
	[0x74] = { 0, KC_T },
	[0x79] = { 0, KC_Y },
	[0x75] = { 0, KC_U },
	[0x69] = { 0, KC_I },
	[0x6f] = { 0, KC_O },
	[0x70] = { 0, KC_P },

	[0x5b] = { 0, KC_LBRACKET },
	[0x5d] = { 0, KC_RBRACKET },

	[0x61] = { 0, KC_A },
	[0x73] = { 0, KC_S },
	[0x64] = { 0, KC_D },
	[0x66] = { 0, KC_F },
	[0x67] = { 0, KC_G },
	[0x68] = { 0, KC_H },
	[0x6a] = { 0, KC_J },
	[0x6b] = { 0, KC_K },
	[0x6c] = { 0, KC_L },

	[0x3b] = { 0, KC_SEMICOLON },
	[0x27] = { 0, KC_QUOTE },
	[0x5c] = { 0, KC_BACKSLASH },

	[0x7a] = { 0, KC_Z },
	[0x78] = { 0, KC_X },
	[0x63] = { 0, KC_C },
	[0x76] = { 0, KC_V },
	[0x62] = { 0, KC_B },
	[0x6e] = { 0, KC_N },
	[0x6d] = { 0, KC_M },

	[0x2c] = { 0, KC_COMMA },
	[0x2e] = { 0, KC_PERIOD },
	[0x2f] = { 0, KC_SLASH },

	[0x20] = { 0, KC_SPACE },

	[0x1b] = { 0, KC_ESCAPE },

	[0x0a] = { 0, KC_ENTER },
	[0x0d] = { 0, KC_ENTER },

	/* with Shift pressed */

	[0x7e] = { KM_LSHIFT, KC_BACKTICK },

	[0x21] = { KM_LSHIFT, KC_1 },
	[0x40] = { KM_LSHIFT, KC_2 },
	[0x23] = { KM_LSHIFT, KC_3 },
	[0x24] = { KM_LSHIFT, KC_4 },
	[0x25] = { KM_LSHIFT, KC_5 },
	[0x5e] = { KM_LSHIFT, KC_6 },
	[0x26] = { KM_LSHIFT, KC_7 },
	[0x2a] = { KM_LSHIFT, KC_8 },
	[0x28] = { KM_LSHIFT, KC_9 },
	[0x29] = { KM_LSHIFT, KC_0 },

	[0x5f] = { KM_LSHIFT, KC_MINUS },
	[0x2b] = { KM_LSHIFT, KC_EQUALS },

	[0x51] = { KM_LSHIFT, KC_Q },
	[0x57] = { KM_LSHIFT, KC_W },
	[0x45] = { KM_LSHIFT, KC_E },
	[0x52] = { KM_LSHIFT, KC_R },
	[0x54] = { KM_LSHIFT, KC_T },
	[0x59] = { KM_LSHIFT, KC_Y },
	[0x55] = { KM_LSHIFT, KC_U },
	[0x49] = { KM_LSHIFT, KC_I },
	[0x4f] = { KM_LSHIFT, KC_O },
	[0x50] = { KM_LSHIFT, KC_P },

	[0x7b] = { KM_LSHIFT, KC_LBRACKET },
	[0x7d] = { KM_LSHIFT, KC_RBRACKET },

	[0x41] = { KM_LSHIFT, KC_A },
	[0x53] = { KM_LSHIFT, KC_S },
	[0x44] = { KM_LSHIFT, KC_D },
	[0x46] = { KM_LSHIFT, KC_F },
	[0x47] = { KM_LSHIFT, KC_G },
	[0x48] = { KM_LSHIFT, KC_H },
	[0x4a] = { KM_LSHIFT, KC_J },
	[0x4b] = { KM_LSHIFT, KC_K },
	[0x4c] = { KM_LSHIFT, KC_L },

	[0x3a] = { KM_LSHIFT, KC_SEMICOLON },
	[0x22] = { KM_LSHIFT, KC_QUOTE },
	[0x7c] = { KM_LSHIFT, KC_BACKSLASH },

	[0x5a] = { KM_LSHIFT, KC_Z },
	[0x58] = { KM_LSHIFT, KC_X },
	[0x43] = { KM_LSHIFT, KC_C },
	[0x56] = { KM_LSHIFT, KC_V },
	[0x42] = { KM_LSHIFT, KC_B },
	[0x4e] = { KM_LSHIFT, KC_N },
	[0x4d] = { KM_LSHIFT, KC_M },

	[0x3c] = { KM_LSHIFT, KC_COMMA },
	[0x3e] = { KM_LSHIFT, KC_PERIOD },
	[0x3f] = { KM_LSHIFT, KC_SLASH }
};

static int map_e1[][2] =
{
	[0x50] = { 0, KC_F1 },
	[0x51] = { 0, KC_F2 },
	[0x52] = { 0, KC_F3 },
	[0x53] = { 0, KC_F4 },
};

static int map_e2[][2] =
{
	[0x41] = { 0, KC_UP },
	[0x42] = { 0, KC_DOWN },
	[0x44] = { 0, KC_LEFT },
	[0x43] = { 0, KC_RIGHT },
};

static int map_e2a[][2] =
{
	[0x35] = { 0, KC_F5 },
	[0x37] = { 0, KC_F6 },
	[0x38] = { 0, KC_F7 },
	[0x39] = { 0, KC_F8 },
};

static int map_e2b[][2] =
{
	[0x30] = { 0, KC_F9 },
	[0x31] = { 0, KC_F10 },
	[0x32] = { 0, KC_F11 },
	[0x33] = { 0, KC_F12 },
};

static unsigned int mods_keys[][2] = {
	{ KM_LSHIFT, KC_LSHIFT },
	{ 0, 0 }
};

static enum dec_state ds;

void kbd_ctl_parse_scancode(int scancode)
{
	switch (ds) {
	case ds_start:	parse_ds_start(scancode); break;
	case ds_e: 	parse_ds_e(scancode); break;
	case ds_e1:	parse_ds_e1(scancode); break;
	case ds_e2:	parse_ds_e2(scancode); break;
	case ds_e2a:	parse_ds_e2a(scancode); break;
	case ds_e2b:	parse_ds_e2b(scancode); break;
	}
}

static void parse_ds_start(int scancode)
{
	if (scancode == 0x1b) {
		ds = ds_e;
		return;
	}

	parse_leaf(scancode, map_start, sizeof(map_start) / (2 * sizeof(int)));
}

static void parse_ds_e(int scancode)
{
	if (scancode < 0 || scancode >= 0x80) return;

	switch (scancode) {
	case 0x4f: ds = ds_e1; return;
	case 0x5b: ds = ds_e2; return;
	case 0x1b: ds = ds_start; break;
	default: ds = ds_start; return;
	}

	kbd_push_ev(KE_PRESS, KC_ESCAPE);
}

static void parse_ds_e1(int scancode)
{
	parse_leaf(scancode, map_e1, sizeof(map_e1) / (2 * sizeof(int)));
}

static void parse_ds_e2(int scancode)
{
	switch (scancode) {
	case 0x31: ds = ds_e2a; break;
	case 0x32: ds = ds_e2b; break;
	default: ds = ds_start; break;
	}

	parse_leaf(scancode, map_e2, sizeof(map_e2) / (2 * sizeof(int)));
}

static void parse_ds_e2a(int scancode)
{
	parse_leaf(scancode, map_e2a, sizeof(map_e2a) / (2 * sizeof(int)));
}

static void parse_ds_e2b(int scancode)
{
	parse_leaf(scancode, map_e2b, sizeof(map_e2b) / (2 * sizeof(int)));
}

static void parse_leaf(int scancode, int (*map)[2], size_t map_length)
{
	unsigned int key, mod;
	int i;

	ds = ds_start;

	if (scancode < 0 || scancode >= map_length)
		return;

	mod = map[scancode][0];
	key = map[scancode][1];

	/* Simulate modifier pressing. */
	i = 0;
	while (mods_keys[i][0] != 0) {
		if (mod & mods_keys[i][0]) {
			kbd_push_ev(KE_PRESS, mods_keys[i][1]);
		}
		++i;
	}

	if (key != 0) {
		kbd_push_ev(KE_PRESS, key);
		kbd_push_ev(KE_RELEASE, key);
	}

	/* Simulate modifier releasing. */
	i = 0;
	while (mods_keys[i][0] != 0) {
		if (mod & mods_keys[i][0]) {
			kbd_push_ev(KE_RELEASE, mods_keys[i][1]);
		}
		++i;
	}
}


/**
 * @}
 */ 
