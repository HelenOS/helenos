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

static void parse_leaf(int scancode, int *map, size_t map_length);

enum dec_state {
	ds_start,
	ds_e,
	ds_e1,
	ds_e2,
	ds_e2a,
	ds_e2b
};

static int map_start[] = {

	[0x60] = KC_BACKTICK,

	[0x31] = KC_1,
	[0x32] = KC_2,
	[0x33] = KC_3,
	[0x34] = KC_4,
	[0x35] = KC_5,
	[0x36] = KC_6,
	[0x37] = KC_7,
	[0x38] = KC_8,
	[0x39] = KC_9,
	[0x30] = KC_0,

	[0x2d] = KC_MINUS,
	[0x3d] = KC_EQUALS,
	[0x08] = KC_BACKSPACE,

	[0x0f] = KC_TAB,

	[0x71] = KC_Q,
	[0x77] = KC_W,
	[0x65] = KC_E,
	[0x72] = KC_R,
	[0x74] = KC_T,
	[0x79] = KC_Y,
	[0x75] = KC_U,
	[0x69] = KC_I,
	[0x6f] = KC_O,
	[0x70] = KC_P,

	[0x5b] = KC_LBRACKET,
	[0x5d] = KC_RBRACKET,

//	[0x3a] = KC_CAPS_LOCK,

	[0x61] = KC_A,
	[0x73] = KC_S,
	[0x64] = KC_D,
	[0x66] = KC_F,
	[0x67] = KC_G,
	[0x68] = KC_H,
	[0x6a] = KC_J,
	[0x6b] = KC_K,
	[0x6c] = KC_L,

	[0x3b] = KC_SEMICOLON,
	[0x27] = KC_QUOTE,
	[0x5c] = KC_BACKSLASH,

//	[0x2a] = KC_LSHIFT,

	[0x7a] = KC_Z,
	[0x78] = KC_X,
	[0x63] = KC_C,
	[0x76] = KC_V,
	[0x62] = KC_B,
	[0x6e] = KC_N,
	[0x6d] = KC_M,

	[0x2c] = KC_COMMA,
	[0x2e] = KC_PERIOD,
	[0x2f] = KC_SLASH,

//	[0x36] = KC_RSHIFT,

//	[0x1d] = KC_LCTRL,
//	[0x38] = KC_LALT,
	[0x20] = KC_SPACE,

	[0x1b] = KC_ESCAPE,

	[0x0a] = KC_ENTER,
	[0x0d] = KC_ENTER

/*
	[0x1] = KC_PRNSCR,
	[0x1] = KC_SCROLL_LOCK,
	[0x1] = KC_PAUSE,
*/
};

static int map_e1[] =
{
	[0x50] = KC_F1,
	[0x51] = KC_F2,
	[0x52] = KC_F3,
	[0x53] = KC_F4,
};

static int map_e2[] =
{
	[0x41] = KC_UP,
	[0x42] = KC_DOWN,
	[0x44] = KC_LEFT,
	[0x43] = KC_RIGHT,
};

static int map_e2a[] =
{
	[0x35] = KC_F5,
	[0x37] = KC_F6,
	[0x38] = KC_F7,
	[0x39] = KC_F8,
};

static int map_e2b[] =
{
	[0x30] = KC_F9,
	[0x31] = KC_F10,
	[0x32] = KC_F11,
	[0x33] = KC_F12,
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
	if (scancode < 0 || scancode >= sizeof(map_start) / sizeof(int))
		return;

	if (scancode == 0x1b) {
		ds = ds_e;
		return;
	}

	parse_leaf(scancode, map_start, sizeof(map_start) / sizeof(int));
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
	parse_leaf(scancode, map_e1, sizeof(map_e1) / sizeof(int));
}

static void parse_ds_e2(int scancode)
{
	switch (scancode) {
	case 0x31: ds = ds_e2a; break;
	case 0x32: ds = ds_e2b; break;
	default: ds = ds_start; break;
	}

	parse_leaf(scancode, map_e2, sizeof(map_e2) / sizeof(int));
}

static void parse_ds_e2a(int scancode)
{
	parse_leaf(scancode, map_e2a, sizeof(map_e2a) / sizeof(int));
}

static void parse_ds_e2b(int scancode)
{
	parse_leaf(scancode, map_e2b, sizeof(map_e2b) / sizeof(int));
}

static void parse_leaf(int scancode, int *map, size_t map_length)
{
	unsigned int key;

	ds = ds_start;

	if (scancode < 0 || scancode >= map_length)
		return;

	key = map[scancode];
	if (key != 0)
		kbd_push_ev(KE_PRESS, key);
}


/**
 * @}
 */ 
