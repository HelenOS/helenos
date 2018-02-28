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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef LIBC_IO_KEYCODE_H_
#define LIBC_IO_KEYCODE_H_

/** Keycode definitions.
 *
 * A keycode identifies a key by its position on the keyboard, rather
 * than by its label. For human readability, key positions are noted
 * with the key label on a keyboard with US layout. This label has
 * nothing to do with the character, that the key produces
 * -- this is determined by the keymap.
 *
 * The keyboard model reflects a standard PC keyboard layout.
 * Non-standard keyboards need to be mapped to this model in some
 * logical way. Scancodes are mapped to keycodes with a scanmap.
 *
 * For easier mapping to the model and to emphasize the nature of keycodes,
 * they really are organized here by position, rather than by label.
 */
typedef enum {

	/* Main block row 1 */

	KC_BACKTICK = 1,

	KC_1,
	KC_2,
	KC_3,
	KC_4,
	KC_5,
	KC_6,
	KC_7,
	KC_8,
	KC_9,
	KC_0,

	KC_MINUS,
	KC_EQUALS,
	KC_BACKSPACE,

	/* Main block row 2 */

	KC_TAB,

	KC_Q,
	KC_W,
	KC_E,
	KC_R,
	KC_T,
	KC_Y,
	KC_U,
	KC_I,
	KC_O,
	KC_P,

	KC_LBRACKET,
	KC_RBRACKET,

	/* Main block row 3 */

	KC_CAPS_LOCK,

	KC_A,
	KC_S,
	KC_D,
	KC_F,
	KC_G,
	KC_H,
	KC_J,
	KC_K,
	KC_L,

	KC_SEMICOLON,
	KC_QUOTE,
	KC_BACKSLASH,
	KC_HASH,

	KC_ENTER,

	/* Main block row 4 */

	KC_LSHIFT,

	KC_Z,
	KC_X,
	KC_C,
	KC_V,
	KC_B,
	KC_N,
	KC_M,

	KC_COMMA,
	KC_PERIOD,
	KC_SLASH,

	KC_RSHIFT,

	/* Main block row 5 */

	KC_LCTRL,
	KC_LALT,
	KC_SPACE,
	KC_RALT,
	KC_RCTRL,

	/* Function keys block */

	KC_ESCAPE,

	KC_F1,
	KC_F2,
	KC_F3,
	KC_F4,
	KC_F5,
	KC_F6,
	KC_F7,
	KC_F8,
	KC_F9,
	KC_F10,
	KC_F11,
	KC_F12,

	KC_PRTSCR,
	KC_SYSREQ,
	KC_SCROLL_LOCK,
	KC_PAUSE,
	KC_BREAK,

	/* Cursor keys block */

	KC_INSERT,
	KC_HOME,
	KC_PAGE_UP,

	KC_DELETE,
	KC_END,
	KC_PAGE_DOWN,

	KC_UP,
	KC_LEFT,
	KC_DOWN,
	KC_RIGHT,

	/* Numeric block */

	KC_NUM_LOCK,
	KC_NSLASH,
	KC_NTIMES,
	KC_NMINUS,

	KC_NPLUS,
	KC_NENTER,

	KC_N7,
	KC_N8,
	KC_N9,

	KC_N4,
	KC_N5,
	KC_N6,

	KC_N1,
	KC_N2,
	KC_N3,

	KC_N0,
	KC_NPERIOD

} keycode_t;

typedef enum {
	KM_LSHIFT      = 0x001,
	KM_RSHIFT      = 0x002,
	KM_LCTRL       = 0x004,
	KM_RCTRL       = 0x008,
	KM_LALT        = 0x010,
	KM_RALT        = 0x020,
	KM_CAPS_LOCK   = 0x040,
	KM_NUM_LOCK    = 0x080,
	KM_SCROLL_LOCK = 0x100,

	KM_SHIFT       = KM_LSHIFT | KM_RSHIFT,
	KM_CTRL        = KM_LCTRL | KM_RCTRL,
	KM_ALT         = KM_LALT | KM_RALT
} keymod_t;

#endif

/** @}
 */
