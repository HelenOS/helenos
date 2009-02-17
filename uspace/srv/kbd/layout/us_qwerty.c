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
 * @brief	US QWERTY leyout.
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

	[KC_MINUS] = '-',
	[KC_EQUALS] = '=',
	[KC_BACKSPACE] = '\b',

	[KC_TAB] = '\t',

	[KC_Q] = 'q',
	[KC_W] = 'w',
	[KC_E] = 'e',
	[KC_R] = 'r',
	[KC_T] = 't',
	[KC_Y] = 'y',
	[KC_U] = 'u',
	[KC_I] = 'i',
	[KC_O] = 'o',
	[KC_P] = 'p',

	[KC_LBRACKET] = '[',
	[KC_RBRACKET] = ']',

	[KC_A] = 'a',
	[KC_S] = 's',
	[KC_D] = 'd',
	[KC_F] = 'f',
	[KC_G] = 'g',
	[KC_H] = 'h',
	[KC_J] = 'j',
	[KC_K] = 'k',
	[KC_L] = 'l',

	[KC_SEMICOLON] = ';',
	[KC_QUOTE] = '\'',
	[KC_BACKSLASH] = '\\',
	[KC_ENTER] = '\n',

	[KC_Z] = 'z',
	[KC_X] = 'x',
	[KC_C] = 'c',
	[KC_V] = 'v',
	[KC_B] = 'b',
	[KC_N] = 'n',
	[KC_M] = 'm',

	[KC_COMMA] = ',',
	[KC_PERIOD] = '.',
	[KC_SLASH] = '/',

	[KC_SPACE] = ' '
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
