/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup kbd_ctl
 * @ingroup input
 * @{
 */
/**
 * @file
 * @brief Sun keyboard controller driver.
 */

#include <io/console.h>
#include <io/keycode.h>
#include "../kbd.h"
#include "../kbd_port.h"
#include "../kbd_ctl.h"

static void sun_ctl_parse(sysarg_t);
static errno_t sun_ctl_init(kbd_dev_t *);
static void sun_ctl_set_ind(kbd_dev_t *, unsigned int);

kbd_ctl_ops_t sun_ctl = {
	.parse = sun_ctl_parse,
	.init = sun_ctl_init,
	.set_ind = sun_ctl_set_ind
};

static kbd_dev_t *kbd_dev;

#define KBD_KEY_RELEASE		0x80
#define KBD_ALL_KEYS_UP		0x7f

static int scanmap_simple[];

static errno_t sun_ctl_init(kbd_dev_t *kdev)
{
	kbd_dev = kdev;
	return 0;
}

static void sun_ctl_parse(sysarg_t scancode)
{
	kbd_event_type_t type;
	unsigned int key;

	if (scancode >= 0x100)
		return;

	if (scancode == KBD_ALL_KEYS_UP)
		return;

	if (scancode & KBD_KEY_RELEASE) {
		scancode &= ~KBD_KEY_RELEASE;
		type = KEY_RELEASE;
	} else {
		type = KEY_PRESS;
	}

	key = scanmap_simple[scancode];
	if (key != 0)
		kbd_push_event(kbd_dev, type, key);
}

static void sun_ctl_set_ind(kbd_dev_t *kdev, unsigned mods)
{
	(void) mods;
}

/** Primary meaning of scancodes. */
static int scanmap_simple[] = {
	[0x00] = 0,
	[0x01] = 0,
	[0x02] = 0,
	[0x03] = 0,
	[0x04] = 0,
	[0x05] = KC_F1,
	[0x06] = KC_F2,
	[0x07] = KC_F10,
	[0x08] = KC_F3,
	[0x09] = KC_F11,
	[0x0a] = KC_F4,
	[0x0b] = KC_F12,
	[0x0c] = KC_F5,
	[0x0d] = KC_RALT,
	[0x0e] = KC_F6,
	[0x0f] = 0,
	[0x10] = KC_F7,
	[0x11] = KC_F8,
	[0x12] = KC_F9,
	[0x13] = KC_LALT,
	[0x14] = KC_UP,
	[0x15] = KC_PAUSE,
	[0x16] = KC_PRTSCR,
	[0x17] = KC_SCROLL_LOCK,
	[0x18] = KC_LEFT,
	[0x19] = 0,
	[0x1a] = 0,
	[0x1b] = KC_DOWN,
	[0x1c] = KC_RIGHT,
	[0x1d] = KC_ESCAPE,
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
	[0x28] = KC_MINUS,
	[0x29] = KC_EQUALS,
	[0x2a] = KC_BACKTICK,
	[0x2b] = KC_BACKSPACE,
	[0x2c] = KC_INSERT,
	[0x2d] = 0,
	[0x2e] = KC_NSLASH,
	[0x2f] = KC_NTIMES,
	[0x30] = 0,
	[0x31] = 0,
	[0x32] = KC_NPERIOD,
	[0x33] = 0,
	[0x34] = KC_HOME,
	[0x35] = KC_TAB,
	[0x36] = KC_Q,
	[0x37] = KC_W,
	[0x38] = KC_E,
	[0x39] = KC_R,
	[0x3a] = KC_T,
	[0x3b] = KC_Y,
	[0x3c] = KC_U,
	[0x3d] = KC_I,
	[0x3e] = KC_O,
	[0x3f] = KC_P,
	[0x40] = KC_LBRACKET,
	[0x41] = KC_RBRACKET,
	[0x42] = KC_DELETE,
	[0x43] = 0,
	[0x44] = KC_N7,
	[0x45] = KC_N8,
	[0x46] = KC_N9,
	[0x47] = KC_NMINUS,
	[0x48] = 0,
	[0x49] = 0,
	[0x4a] = KC_END,
	[0x4b] = 0,
	[0x4c] = KC_LCTRL,
	[0x4d] = KC_A,
	[0x4e] = KC_S,
	[0x4f] = KC_D,
	[0x50] = KC_F,
	[0x51] = KC_G,
	[0x52] = KC_H,
	[0x53] = KC_J,
	[0x54] = KC_K,
	[0x55] = KC_L,
	[0x56] = KC_SEMICOLON,
	[0x57] = KC_QUOTE,
	[0x58] = KC_BACKSLASH,
	[0x59] = KC_ENTER,
	[0x5a] = KC_NENTER,
	[0x5b] = KC_N4,
	[0x5c] = KC_N5,
	[0x5d] = KC_N6,
	[0x5e] = KC_N0,
	[0x5f] = 0,
	[0x60] = KC_PAGE_UP,
	[0x61] = 0,
	[0x62] = KC_NUM_LOCK,
	[0x63] = KC_LSHIFT,
	[0x64] = KC_Z,
	[0x65] = KC_X,
	[0x66] = KC_C,
	[0x67] = KC_V,
	[0x68] = KC_B,
	[0x69] = KC_N,
	[0x6a] = KC_M,
	[0x6b] = KC_COMMA,
	[0x6c] = KC_PERIOD,
	[0x6d] = KC_SLASH,
	[0x6e] = KC_RSHIFT,
	[0x6f] = 0,
	[0x70] = KC_N1,
	[0x71] = KC_N2,
	[0x72] = KC_N3,
	[0x73] = 0,
	[0x74] = 0,
	[0x75] = 0,
	[0x76] = 0,
	[0x77] = KC_CAPS_LOCK,
	[0x78] = 0,
	[0x79] = KC_SPACE,
	[0x7a] = 0,
	[0x7b] = KC_PAGE_DOWN,
	[0x7c] = 0,
	[0x7d] = KC_NPLUS,
	[0x7e] = 0,
	[0x7f] = 0
};

/** @}
 */
