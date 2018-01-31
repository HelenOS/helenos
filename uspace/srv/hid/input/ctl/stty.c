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

/** @addtogroup kbd_ctl
 * @ingroup input
 * @{
 */
/**
 * @file
 * @brief Serial TTY-like keyboard controller driver.
 *
 * Keyboard emulation on a serial terminal.
 */

#include <io/keycode.h>
#include "../stroke.h"
#include "../gsp.h"
#include "../kbd.h"
#include "../kbd_port.h"
#include "../kbd_ctl.h"

static void stty_ctl_parse(sysarg_t);
static errno_t stty_ctl_init(kbd_dev_t *);
static void stty_ctl_set_ind(kbd_dev_t *, unsigned int);

kbd_ctl_ops_t stty_ctl = {
	.parse = stty_ctl_parse,
	.init = stty_ctl_init,
	.set_ind = stty_ctl_set_ind
};

static kbd_dev_t *kbd_dev;

/** Scancode parser */
static gsp_t sp;

/** Current parser state */
static int ds;

#include <stdio.h>

/**
 * Sequnece definitions are primarily for Xterm. Additionally we define
 * sequences that are unique to Gnome terminal -- most are the same but
 * some differ.
 */
static int seq_defs[] = {
	/* Not shifted */

	0,	KC_BACKTICK,	0x60, GSP_END,

	0,	KC_1,		0x31, GSP_END,
	0,	KC_2,		0x32, GSP_END,
	0,	KC_3,		0x33, GSP_END,
	0,	KC_4,		0x34, GSP_END,
	0,	KC_5,		0x35, GSP_END,
	0,	KC_6,		0x36, GSP_END,
	0,	KC_7,		0x37, GSP_END,
	0,	KC_8,		0x38, GSP_END,
	0,	KC_9,		0x39, GSP_END,
	0,	KC_0,		0x30, GSP_END,

	0,	KC_MINUS,	0x2d, GSP_END,
	0,	KC_EQUALS,	0x3d, GSP_END,

	0,	KC_BACKSPACE,	0x08, GSP_END,

	0,	KC_TAB,		0x09, GSP_END,

	0,	KC_Q,		0x71, GSP_END,
	0,	KC_W,		0x77, GSP_END,
	0,	KC_E,		0x65, GSP_END,
	0,	KC_R,		0x72, GSP_END,
	0,	KC_T,		0x74, GSP_END,
	0,	KC_Y,		0x79, GSP_END,
	0,	KC_U,		0x75, GSP_END,
	0,	KC_I,		0x69, GSP_END,
	0,	KC_O,		0x6f, GSP_END,
	0,	KC_P,		0x70, GSP_END,

	0,	KC_LBRACKET,	0x5b, GSP_END,
	0,	KC_RBRACKET,	0x5d, GSP_END,

	0,	KC_A,		0x61, GSP_END,
	0,	KC_S,		0x73, GSP_END,
	0,	KC_D,		0x64, GSP_END,
	0,	KC_F,		0x66, GSP_END,
	0,	KC_G,		0x67, GSP_END,
	0,	KC_H,		0x68, GSP_END,
	0,	KC_J,		0x6a, GSP_END,
	0,	KC_K,		0x6b, GSP_END,
	0,	KC_L,		0x6c, GSP_END,

	0,	KC_SEMICOLON,	0x3b, GSP_END,
	0,	KC_QUOTE,	0x27, GSP_END,
	0,	KC_BACKSLASH,	0x5c, GSP_END,

	0,	KC_Z,		0x7a, GSP_END,
	0,	KC_X,		0x78, GSP_END,
	0,	KC_C,		0x63, GSP_END,
	0,	KC_V,		0x76, GSP_END,
	0,	KC_B,		0x62, GSP_END,
	0,	KC_N,		0x6e, GSP_END,
	0,	KC_M,		0x6d, GSP_END,

	0,	KC_COMMA,	0x2c, GSP_END,
	0,	KC_PERIOD,	0x2e, GSP_END,
	0,	KC_SLASH,	0x2f, GSP_END,

	/* Shifted */

	KM_SHIFT,	KC_BACKTICK,	0x7e, GSP_END,

	KM_SHIFT,	KC_1,		0x21, GSP_END,
	KM_SHIFT,	KC_2,		0x40, GSP_END,
	KM_SHIFT,	KC_3,		0x23, GSP_END,
	KM_SHIFT,	KC_4,		0x24, GSP_END,
	KM_SHIFT,	KC_5,		0x25, GSP_END,
	KM_SHIFT,	KC_6,		0x5e, GSP_END,
	KM_SHIFT,	KC_7,		0x26, GSP_END,
	KM_SHIFT,	KC_8,		0x2a, GSP_END,
	KM_SHIFT,	KC_9,		0x28, GSP_END,
	KM_SHIFT,	KC_0,		0x29, GSP_END,

	KM_SHIFT,	KC_MINUS,	0x5f, GSP_END,
	KM_SHIFT,	KC_EQUALS,	0x2b, GSP_END,

	KM_SHIFT,	KC_Q,		0x51, GSP_END,
	KM_SHIFT,	KC_W,		0x57, GSP_END,
	KM_SHIFT,	KC_E,		0x45, GSP_END,
	KM_SHIFT,	KC_R,		0x52, GSP_END,
	KM_SHIFT,	KC_T,		0x54, GSP_END,
	KM_SHIFT,	KC_Y,		0x59, GSP_END,
	KM_SHIFT,	KC_U,		0x55, GSP_END,
	KM_SHIFT,	KC_I,		0x49, GSP_END,
	KM_SHIFT,	KC_O,		0x4f, GSP_END,
	KM_SHIFT,	KC_P,		0x50, GSP_END,

	KM_SHIFT,	KC_LBRACKET,	0x7b, GSP_END,
	KM_SHIFT,	KC_RBRACKET,	0x7d, GSP_END,

	KM_SHIFT,	KC_A,		0x41, GSP_END,
	KM_SHIFT,	KC_S,		0x53, GSP_END,
	KM_SHIFT,	KC_D,		0x44, GSP_END,
	KM_SHIFT,	KC_F,		0x46, GSP_END,
	KM_SHIFT,	KC_G,		0x47, GSP_END,
	KM_SHIFT,	KC_H,		0x48, GSP_END,
	KM_SHIFT,	KC_J,		0x4a, GSP_END,
	KM_SHIFT,	KC_K,		0x4b, GSP_END,
	KM_SHIFT,	KC_L,		0x4c, GSP_END,

	KM_SHIFT,	KC_SEMICOLON,	0x3a, GSP_END,
	KM_SHIFT,	KC_QUOTE,	0x22, GSP_END,
	KM_SHIFT,	KC_BACKSLASH,	0x7c, GSP_END,

	KM_SHIFT,	KC_Z,		0x5a, GSP_END,
	KM_SHIFT,	KC_X,		0x58, GSP_END,
	KM_SHIFT,	KC_C,		0x43, GSP_END,
	KM_SHIFT,	KC_V,		0x56, GSP_END,
	KM_SHIFT,	KC_B,		0x42, GSP_END,
	KM_SHIFT,	KC_N,		0x4e, GSP_END,
	KM_SHIFT,	KC_M,		0x4d, GSP_END,

	KM_SHIFT,	KC_COMMA,	0x3c, GSP_END,
	KM_SHIFT,	KC_PERIOD,	0x3e, GSP_END,
	KM_SHIFT,	KC_SLASH,	0x3f, GSP_END,

	/* ... */

	0,	KC_SPACE,	0x20, GSP_END,
	0,	KC_ENTER,	0x0a, GSP_END,
	0,	KC_ENTER,	0x0d, GSP_END,

	0,	KC_ESCAPE,	0x1b, 0x1b, GSP_END,

	0,	KC_F1,		0x1b, 0x4f, 0x50, GSP_END,
	0,	KC_F2,		0x1b, 0x4f, 0x51, GSP_END,
	0,	KC_F3,		0x1b, 0x4f, 0x52, GSP_END,
	0,	KC_F4,		0x1b, 0x4f, 0x53, GSP_END,
	0,	KC_F5,		0x1b, 0x5b, 0x31, 0x35, 0x7e, GSP_END,
	0,	KC_F6,		0x1b, 0x5b, 0x31, 0x37, 0x7e, GSP_END,
	0,	KC_F7,		0x1b, 0x5b, 0x31, 0x38, 0x7e, GSP_END,
	0,	KC_F8,		0x1b, 0x5b, 0x31, 0x39, 0x7e, GSP_END,
	0,	KC_F9,		0x1b, 0x5b, 0x32, 0x30, 0x7e, GSP_END,
	0,	KC_F10,		0x1b, 0x5b, 0x32, 0x31, 0x7e, GSP_END,
	0,	KC_F11,		0x1b, 0x5b, 0x32, 0x33, 0x7e, GSP_END,
	0,	KC_F12,		0x1b, 0x5b, 0x32, 0x34, 0x7e, GSP_END,

	0,	KC_PRTSCR,	0x1b, 0x5b, 0x32, 0x35, 0x7e, GSP_END,
	0,	KC_PAUSE,	0x1b, 0x5b, 0x32, 0x38, 0x7e, GSP_END,

	0,	KC_INSERT,	0x1b, 0x5b, 0x32, 0x7e, GSP_END,
	0,	KC_HOME,	0x1b, 0x5b, 0x48, GSP_END,
	0,	KC_PAGE_UP,	0x1b, 0x5b, 0x35, 0x7e, GSP_END,
	0,	KC_DELETE,	0x1b, 0x5b, 0x33, 0x7e, GSP_END,
	0,	KC_END,		0x1b, 0x5b, 0x46, GSP_END,
	0,	KC_PAGE_DOWN,	0x1b, 0x5b, 0x36, 0x7e, GSP_END,

	0,	KC_UP,		0x1b, 0x5b, 0x41, GSP_END,
	0,	KC_LEFT,	0x1b, 0x5b, 0x44, GSP_END,
	0,	KC_DOWN,	0x1b, 0x5b, 0x42, GSP_END,
	0,	KC_RIGHT,	0x1b, 0x5b, 0x43, GSP_END,

	/*
	 * Sequences specific to Gnome terminal
	 */
	0,	KC_BACKSPACE,	0x7f, GSP_END, /* ASCII DEL */
	0,	KC_HOME,	0x1b, 0x4f, 0x48, GSP_END,
	0,	KC_END,		0x1b, 0x4f, 0x46, GSP_END,

	0,	0
};

static errno_t stty_ctl_init(kbd_dev_t *kdev)
{
	kbd_dev = kdev;
	ds = 0;

	gsp_init(&sp);
	if (gsp_insert_defs(&sp, seq_defs) < 0) {
		return EINVAL;
	}
	return EOK;
}

static void stty_ctl_parse(sysarg_t scancode)
{
	unsigned mods, key;

	ds = gsp_step(&sp, ds, scancode, &mods, &key);
	if (key != 0) {
		stroke_sim(kbd_dev, mods, key);
	}
}

static void stty_ctl_set_ind(kbd_dev_t *kdev, unsigned mods)
{
	(void) mods;
}

/**
 * @}
 */ 
