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

/** @addtogroup kbd
 * @brief	US Dvorak Simplified Keyboard layout.
 * @{
 */ 

#include <kbd.h>
#include <kbd/kbd.h>
#include <kbd/keycode.h>
#include <layout.h>

static int map_normal[] = {
	[KC_BACKTICK] = '`',

	[KC_1] = '1',
	[KC_2] = '2',
	[KC_3] = '3',
	[KC_4] = '4',
	[KC_5] = '5',
	[KC_6] = '6',
	[KC_7] = '7',
	[KC_8] = '8',
	[KC_9] = '9',
	[KC_0] = '0',

	[KC_MINUS] = '[',
	[KC_EQUALS] = ']',
	[KC_BACKSPACE] = '\b',

	[KC_TAB] = '\t',

	[KC_Q] = '\'',
	[KC_W] = ',',
	[KC_E] = '.',
	[KC_R] = 'p',
	[KC_T] = 'y',
	[KC_Y] = 'f',
	[KC_U] = 'g',
	[KC_I] = 'c',
	[KC_O] = 'r',
	[KC_P] = 'l',

	[KC_LBRACKET] = '/',
	[KC_RBRACKET] = '=',

	[KC_A] = 'a',
	[KC_S] = 'o',
	[KC_D] = 'e',
	[KC_F] = 'u',
	[KC_G] = 'i',
	[KC_H] = 'd',
	[KC_J] = 'h',
	[KC_K] = 't',
	[KC_L] = 'n',

	[KC_SEMICOLON] = 's',
	[KC_QUOTE] = '-',
	[KC_BACKSLASH] = '\\',

	[KC_Z] = ';',
	[KC_X] = 'q',
	[KC_C] = 'j',
	[KC_V] = 'k',
	[KC_B] = 'x',
	[KC_N] = 'b',
	[KC_M] = 'm',

	[KC_COMMA] = 'w',
	[KC_PERIOD] = 'v',
	[KC_SLASH] = 'z',

	[KC_ENTER] = '\n'
};

char layout_parse_ev(kbd_event_t *ev)
{
	if (ev->key >= sizeof(map_normal) / sizeof(int))
		return 0;

	return map_normal[ev->key];
}

/**
 * @}
 */ 
