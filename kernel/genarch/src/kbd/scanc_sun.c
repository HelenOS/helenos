/*
 * Copyright (C) 2006 Jakub Jermar
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

/** @addtogroup genarch	
 * @{
 */
/**
 * @file
 * @brief	Scan codes for Sun keyboards.
 */

#include <genarch/kbd/scanc.h>

/** Primary meaning of scancodes. */
char sc_primary_map[] = {
	[0x00] = SPECIAL,
	[0x01] = SPECIAL,
	[0x02] = SPECIAL,
	[0x03] = SPECIAL,
	[0x04] = SPECIAL,
	[0x05] = SPECIAL,	/* F1 */
	[0x06] = SPECIAL,	/* F2 */
	[0x07] = SPECIAL,	/* F10 */
	[0x08] = SPECIAL,	/* F3 */
	[0x09] = SPECIAL,	/* F11 */
	[0x0a] = SPECIAL,	/* F4 */
	[0x0b] = SPECIAL,	/* F12 */
	[0x0c] = SPECIAL,	/* F5 */
	[0x0d] = SPECIAL,	/* RAlt */
	[0x0e] = SPECIAL,	/* F6 */
	[0x0f] = SPECIAL,
	[0x10] = SPECIAL,	/* F7 */
	[0x11] = SPECIAL,	/* F8 */
	[0x12] = SPECIAL,	/* F9 */
	[0x13] = SPECIAL,	/* LAlt */
	[0x14] = SPECIAL,	/* Up Arrow */
	[0x15] = SPECIAL,	/* Pause */
	[0x16] = SPECIAL,
	[0x17] = SPECIAL,	/* Scroll Lock */
	[0x18] = SPECIAL,	/* Left Arrow */
	[0x19] = SPECIAL,
	[0x1a] = SPECIAL,
	[0x1b] = SPECIAL,	/* Down Arrow */
	[0x1c] = SPECIAL,	/* Right Arrow */
	[0x1d] = SPECIAL,	/* Esc */
	[0x1e] = '1',
	[0x1f] = '2',
	[0x20] = '3',
	[0x21] = '4',
	[0x22] = '5',
	[0x23] = '6',
	[0x24] = '7',
	[0x25] = '8',
	[0x26] = '9',
	[0x27] = '0',
	[0x28] = '-',
	[0x29] = '=',
	[0x2a] = '`',
	[0x2b] = '\b',		/* Backspace */
	[0x2c] = SPECIAL,	/* Insert */
	[0x2d] = SPECIAL,
	[0x2e] = '/',		/* numeric keypad */
	[0x2f] = '*',		/* numeric keypad */
	[0x30] = SPECIAL,
	[0x31] = SPECIAL,
	[0x32] = '.',		/* numeric keypad */
	[0x33] = SPECIAL,
	[0x34] = SPECIAL,	/* Home */
	[0x35] = '\t',		/* Tab */
	[0x36] = 'q',
	[0x37] = 'w',
	[0x38] = 'e',
	[0x39] = 'r',
	[0x3a] = 't',
	[0x3b] = 'y',
	[0x3c] = 'u',
	[0x3d] = 'i',
	[0x3e] = 'o',
	[0x3f] = 'p',
	[0x40] = '[',
	[0x41] = ']',
	[0x42] = SPECIAL,	/* Del */
	[0x43] = SPECIAL,
	[0x44] = '7',		/* numeric keypad */
	[0x45] = '8',		/* numeric keypad */
	[0x46] = '9',		/* numeric keypad */
	[0x47] = '-',		/* numeric keypad */
	[0x48] = SPECIAL,
	[0x49] = SPECIAL,
	[0x4a] = SPECIAL,	/* End */
	[0x4b] = SPECIAL,
	[0x4c] = SPECIAL,	/* Control */
	[0x4d] = 'a',
	[0x4e] = 's',
	[0x4f] = 'd',
	[0x50] = 'f',
	[0x51] = 'g',
	[0x52] = 'h',
	[0x53] = 'j',
	[0x54] = 'k',
	[0x55] = 'l',
	[0x56] = ';',
	[0x57] = '\'',
	[0x58] = '\\',
	[0x59] = '\n',		/* Enter */
	[0x5a] = '\n',		/* Enter on numeric keypad */
	[0x5b] = '4',		/* numeric keypad */
	[0x5c] = '5',		/* numeric keypad */
	[0x5d] = '6',		/* numeric keypad */
	[0x5e] = '0',		/* numeric keypad */
	[0x5f] = SPECIAL,
	[0x60] = SPECIAL,	/* Page Up */
	[0x61] = SPECIAL,
	[0x62] = SPECIAL,	/* Num Lock */
	[0x63] = SPECIAL,	/* LShift */
	[0x64] = 'z',
	[0x65] = 'x',
	[0x66] = 'c',
	[0x67] = 'v',
	[0x68] = 'b',
	[0x69] = 'n',
	[0x6a] = 'm',
	[0x6b] = ',',
	[0x6c] = '.',
	[0x6d] = '/',
	[0x6e] = SPECIAL,	/* RShift */
	[0x6f] = SPECIAL,
	[0x70] = '1',		/* numeric keypad */
	[0x71] = '2',		/* numeric keypad */
	[0x72] = '3',		/* numeric keypad */
	[0x73] = SPECIAL,
	[0x74] = SPECIAL,
	[0x75] = SPECIAL,
	[0x76] = SPECIAL,
	[0x77] = SPECIAL,	/* Caps Lock */
	[0x78] = SPECIAL,
	[0x79] = ' ',
	[0x7a] = SPECIAL,
	[0x7b] = SPECIAL,	/* Page Down */
	[0x7c] = SPECIAL,
	[0x7d] = '+',		/* numeric key pad */
	[0x7e] = SPECIAL,
	[0x7f] = SPECIAL
};

/** Secondary meaning of scancodes. */
char sc_secondary_map[] = {
	[0x00] = SPECIAL,
	[0x01] = SPECIAL,
	[0x02] = SPECIAL,
	[0x03] = SPECIAL,
	[0x04] = SPECIAL,
	[0x05] = SPECIAL,	/* F1 */
	[0x06] = SPECIAL,	/* F2 */
	[0x07] = SPECIAL,	/* F10 */
	[0x08] = SPECIAL,	/* F3 */
	[0x09] = SPECIAL,	/* F11 */
	[0x0a] = SPECIAL,	/* F4 */
	[0x0b] = SPECIAL,	/* F12 */
	[0x0c] = SPECIAL,	/* F5 */
	[0x0d] = SPECIAL,	/* RAlt */
	[0x0e] = SPECIAL,	/* F6 */
	[0x0f] = SPECIAL,
	[0x10] = SPECIAL,	/* F7 */
	[0x11] = SPECIAL,	/* F8 */
	[0x12] = SPECIAL,	/* F9 */
	[0x13] = SPECIAL,	/* LAlt */
	[0x14] = SPECIAL,	/* Up Arrow */
	[0x15] = SPECIAL,	/* Pause */
	[0x16] = SPECIAL,
	[0x17] = SPECIAL,	/* Scroll Lock */
	[0x18] = SPECIAL,	/* Left Arrow */
	[0x19] = SPECIAL,
	[0x1a] = SPECIAL,
	[0x1b] = SPECIAL,	/* Down Arrow */
	[0x1c] = SPECIAL,	/* Right Arrow */
	[0x1d] = SPECIAL,	/* Esc */
	[0x1e] = '!',
	[0x1f] = '@',
	[0x20] = '#',
	[0x21] = '$',
	[0x22] = '%',
	[0x23] = '^',
	[0x24] = '&',
	[0x25] = '*',
	[0x26] = '(',
	[0x27] = ')',
	[0x28] = '_',
	[0x29] = '+',
	[0x2a] = '~',
	[0x2b] = SPECIAL,	/* Backspace */
	[0x2c] = SPECIAL,	/* Insert */
	[0x2d] = SPECIAL,
	[0x2e] = '/',		/* numeric keypad */
	[0x2f] = '*',		/* numeric keypad */
	[0x30] = SPECIAL,
	[0x31] = SPECIAL,
	[0x32] = '.',		/* numeric keypad */
	[0x33] = SPECIAL,
	[0x34] = SPECIAL,	/* Home */
	[0x35] = SPECIAL,	/* Tab */
	[0x36] = 'Q',
	[0x37] = 'W',
	[0x38] = 'E',
	[0x39] = 'R',
	[0x3a] = 'T',
	[0x3b] = 'Y',
	[0x3c] = 'U',
	[0x3d] = 'I',
	[0x3e] = 'O',
	[0x3f] = 'P',
	[0x40] = '{',
	[0x41] = '}',
	[0x42] = SPECIAL,	/* Del */
	[0x43] = SPECIAL,
	[0x44] = '7',		/* numeric keypad */
	[0x45] = '8',		/* numeric keypad */
	[0x46] = '9',		/* numeric keypad */
	[0x47] = '-',		/* numeric keypad */
	[0x48] = SPECIAL,
	[0x49] = SPECIAL,
	[0x4a] = SPECIAL,	/* End */
	[0x4b] = SPECIAL,
	[0x4c] = SPECIAL,	/* Control */
	[0x4d] = 'A',
	[0x4e] = 'S',
	[0x4f] = 'D',
	[0x50] = 'F',
	[0x51] = 'G',
	[0x52] = 'H',
	[0x53] = 'J',
	[0x54] = 'K',
	[0x55] = 'L',
	[0x56] = ':',
	[0x57] = '"',
	[0x58] = '|',
	[0x59] = SPECIAL,	/* Enter */
	[0x5a] = SPECIAL,	/* Enter on numeric keypad */
	[0x5b] = '4',		/* numeric keypad */
	[0x5c] = '5',		/* numeric keypad */
	[0x5d] = '6',		/* numeric keypad */
	[0x5e] = '0',		/* numeric keypad */
	[0x5f] = SPECIAL,
	[0x60] = SPECIAL,	/* Page Up */
	[0x61] = SPECIAL,
	[0x62] = SPECIAL,	/* Num Lock */
	[0x63] = SPECIAL,	/* LShift */
	[0x64] = 'Z',
	[0x65] = 'X',
	[0x66] = 'C',
	[0x67] = 'V',
	[0x68] = 'B',
	[0x69] = 'N',
	[0x6a] = 'M',
	[0x6b] = '<',
	[0x6c] = '>',
	[0x6d] = '?',
	[0x6e] = SPECIAL,	/* RShift */
	[0x6f] = SPECIAL,
	[0x70] = '1',		/* numeric keypad */
	[0x71] = '2',		/* numeric keypad */
	[0x72] = '3',		/* numeric keypad */
	[0x73] = SPECIAL,
	[0x74] = SPECIAL,
	[0x75] = SPECIAL,
	[0x76] = SPECIAL,
	[0x77] = SPECIAL,	/* Caps Lock */
	[0x78] = SPECIAL,
	[0x79] = ' ',
	[0x7a] = SPECIAL,
	[0x7b] = SPECIAL,	/* Page Down */
	[0x7c] = SPECIAL,
	[0x7d] = '+',		/* numeric key pad */
	[0x7e] = SPECIAL,
	[0x7f] = SPECIAL
};

/** @}
 */
