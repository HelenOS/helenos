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

static char map_lcase[] = {
	[KC_R] = 'p',
	[KC_T] = 'y',
	[KC_Y] = 'f',
	[KC_U] = 'g',
	[KC_I] = 'c',
	[KC_O] = 'r',
	[KC_P] = 'l',

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

	[KC_X] = 'q',
	[KC_C] = 'j',
	[KC_V] = 'k',
	[KC_B] = 'x',
	[KC_N] = 'b',
	[KC_M] = 'm',

	[KC_COMMA] = 'w',
	[KC_PERIOD] = 'v',
	[KC_SLASH] = 'z',
};

static char map_ucase[] = {
	[KC_R] = 'P',
	[KC_T] = 'Y',
	[KC_Y] = 'F',
	[KC_U] = 'G',
	[KC_I] = 'C',
	[KC_O] = 'R',
	[KC_P] = 'L',

	[KC_A] = 'A',
	[KC_S] = 'O',
	[KC_D] = 'E',
	[KC_F] = 'U',
	[KC_G] = 'I',
	[KC_H] = 'D',
	[KC_J] = 'H',
	[KC_K] = 'T',
	[KC_L] = 'N',

	[KC_SEMICOLON] = 'S',

	[KC_X] = 'Q',
	[KC_C] = 'J',
	[KC_V] = 'K',
	[KC_B] = 'X',
	[KC_N] = 'B',
	[KC_M] = 'M',

	[KC_COMMA] = 'W',
	[KC_PERIOD] = 'V',
	[KC_SLASH] = 'Z',
};

static char map_not_shifted[] = {
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

	[KC_Q] = '\'',
	[KC_W] = ',',
	[KC_E] = '.',

	[KC_LBRACKET] = '/',
	[KC_RBRACKET] = '=',

	[KC_QUOTE] = '-',
	[KC_BACKSLASH] = '\\',

	[KC_Z] = ';',
};

static char map_shifted[] = {
	[KC_BACKTICK] = '~',

	[KC_1] = '!',
	[KC_2] = '@',
	[KC_3] = '#',
	[KC_4] = '$',
	[KC_5] = '%',
	[KC_6] = '^',
	[KC_7] = '&',
	[KC_8] = '*',
	[KC_9] = '(',
	[KC_0] = ')',

	[KC_MINUS] = '{',
	[KC_EQUALS] = '}',

	[KC_Q] = '"',
	[KC_W] = '<',
	[KC_E] = '>',

	[KC_LBRACKET] = '?',
	[KC_RBRACKET] = '+',

	[KC_QUOTE] = '_',
	[KC_BACKSLASH] = '|',

	[KC_Z] = ':',
};

static char map_neutral[] = {
	[KC_BACKSPACE] = '\b',
	[KC_TAB] = '\t',
	[KC_ENTER] = '\n'
};

static int translate(unsigned int key, char *map, size_t map_length)
{
	if (key >= map_length) return 0;
	return map[key];	
}

char layout_parse_ev(kbd_event_t *ev)
{
	char c;

	/* Produce no characters when Ctrl or Alt is pressed. */
	if ((ev->mods & (KM_CTRL | KM_ALT)) != 0)
		return 0;

	c = translate(ev->key, map_neutral, sizeof(map_neutral) / sizeof(char));
	if (c != 0) return c;

	if (((ev->mods & KM_SHIFT) != 0) ^ ((ev->mods & KM_CAPS_LOCK) != 0))
		c = translate(ev->key, map_ucase, sizeof(map_ucase) / sizeof(char));
	else
		c = translate(ev->key, map_lcase, sizeof(map_lcase) / sizeof(char));

	if (c != 0) return c;

	if ((ev->mods & KM_SHIFT) != 0)
		c = translate(ev->key, map_shifted, sizeof(map_shifted) / sizeof(char));
	else
		c = translate(ev->key, map_not_shifted, sizeof(map_not_shifted) / sizeof(char));

	if (c != 0 ) return c;

}


/**
 * @}
 */ 
