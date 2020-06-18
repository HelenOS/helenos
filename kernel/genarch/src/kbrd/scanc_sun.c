/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief Scan codes for Sun keyboards.
 */

#include <genarch/kbrd/scanc.h>
#include <typedefs.h>
#include <str.h>

/** Primary meaning of scancodes. */
char32_t sc_primary_map[SCANCODES] = {
	[0x00] = U_SPECIAL,
	[0x01] = U_SPECIAL,
	[0x02] = U_SPECIAL,
	[0x03] = U_SPECIAL,
	[0x04] = U_SPECIAL,
	[0x05] = U_SPECIAL,      /* F1 */
	[0x06] = U_SPECIAL,      /* F2 */
	[0x07] = U_SPECIAL,      /* F10 */
	[0x08] = U_SPECIAL,      /* F3 */
	[0x09] = U_SPECIAL,      /* F11 */
	[0x0a] = U_SPECIAL,      /* F4 */
	[0x0b] = U_SPECIAL,      /* F12 */
	[0x0c] = U_SPECIAL,      /* F5 */
	[0x0d] = U_SPECIAL,      /* Right Alt */
	[0x0e] = U_SPECIAL,      /* F6 */
	[0x0f] = U_SPECIAL,
	[0x10] = U_SPECIAL,      /* F7 */
	[0x11] = U_SPECIAL,      /* F8 */
	[0x12] = U_SPECIAL,      /* F9 */
	[0x13] = U_SPECIAL,      /* Left Alt */
	[0x14] = U_UP_ARROW,     /* Up Arrow */
	[0x15] = U_SPECIAL,      /* Pause */
	[0x16] = U_SPECIAL,
	[0x17] = U_SPECIAL,      /* Scroll Lock */
	[0x18] = U_LEFT_ARROW,   /* Left Arrow */
	[0x19] = U_SPECIAL,
	[0x1a] = U_SPECIAL,
	[0x1b] = U_DOWN_ARROW,   /* Down Arrow */
	[0x1c] = U_RIGHT_ARROW,  /* Right Arrow */
	[0x1d] = U_ESCAPE,       /* Esc */
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
	[0x2b] = '\b',           /* Backspace */
	[0x2c] = U_SPECIAL,      /* Insert */
	[0x2d] = U_SPECIAL,
	[0x2e] = '/',            /* Numpad / */
	[0x2f] = '*',            /* Numpad * */
	[0x30] = U_SPECIAL,
	[0x31] = U_SPECIAL,
	[0x32] = '.',            /* Numpad . */
	[0x33] = U_SPECIAL,
	[0x34] = U_HOME_ARROW,   /* Home */
	[0x35] = '\t',           /* Tab */
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
	[0x42] = U_DELETE,       /* Delete */
	[0x43] = U_SPECIAL,
	[0x44] = '7',            /* Numpad 7 */
	[0x45] = '8',            /* Numpad 8 */
	[0x46] = '9',            /* Numpad 9 */
	[0x47] = '-',            /* Numpad - */
	[0x48] = U_SPECIAL,
	[0x49] = U_SPECIAL,
	[0x4a] = U_END_ARROW,    /* End */
	[0x4b] = U_SPECIAL,
	[0x4c] = U_SPECIAL,      /* Control */
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
	[0x59] = '\n',           /* Enter */
	[0x5a] = '\n',           /* Numpad Enter */
	[0x5b] = '4',            /* Numpad 4 */
	[0x5c] = '5',            /* Numpad 5 */
	[0x5d] = '6',            /* Numpad 6 */
	[0x5e] = '0',            /* Numpad 0 */
	[0x5f] = U_SPECIAL,
	[0x60] = U_PAGE_UP,      /* Page Up */
	[0x61] = U_SPECIAL,
	[0x62] = U_SPECIAL,      /* NumLock */
	[0x63] = U_SPECIAL,      /* Left Shift */
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
	[0x6e] = U_SPECIAL,      /* Right Shift */
	[0x6f] = U_SPECIAL,
	[0x70] = '1',            /* Numpad 1 */
	[0x71] = '2',            /* Numpad 2 */
	[0x72] = '3',            /* Numpad 3 */
	[0x73] = U_SPECIAL,
	[0x74] = U_SPECIAL,
	[0x75] = U_SPECIAL,
	[0x76] = U_SPECIAL,
	[0x77] = U_SPECIAL,      /* CapsLock */
	[0x78] = U_SPECIAL,
	[0x79] = ' ',
	[0x7a] = U_SPECIAL,
	[0x7b] = U_PAGE_DOWN,    /* Page Down */
	[0x7c] = U_SPECIAL,
	[0x7d] = '+',            /* Numpad + */
	[0x7e] = U_SPECIAL,
	[0x7f] = U_SPECIAL
};

/** Secondary meaning of scancodes. */
char32_t sc_secondary_map[SCANCODES] = {
	[0x00] = U_SPECIAL,
	[0x01] = U_SPECIAL,
	[0x02] = U_SPECIAL,
	[0x03] = U_SPECIAL,
	[0x04] = U_SPECIAL,
	[0x05] = U_SPECIAL,      /* F1 */
	[0x06] = U_SPECIAL,      /* F2 */
	[0x07] = U_SPECIAL,      /* F10 */
	[0x08] = U_SPECIAL,      /* F3 */
	[0x09] = U_SPECIAL,      /* F11 */
	[0x0a] = U_SPECIAL,      /* F4 */
	[0x0b] = U_SPECIAL,      /* F12 */
	[0x0c] = U_SPECIAL,      /* F5 */
	[0x0d] = U_SPECIAL,      /* Right Alt */
	[0x0e] = U_SPECIAL,      /* F6 */
	[0x0f] = U_SPECIAL,
	[0x10] = U_SPECIAL,      /* F7 */
	[0x11] = U_SPECIAL,      /* F8 */
	[0x12] = U_SPECIAL,      /* F9 */
	[0x13] = U_SPECIAL,      /* Left Alt */
	[0x14] = U_UP_ARROW,     /* Up Arrow */
	[0x15] = U_SPECIAL,      /* Pause */
	[0x16] = U_SPECIAL,
	[0x17] = U_SPECIAL,      /* Scroll Lock */
	[0x18] = U_LEFT_ARROW,   /* Left Arrow */
	[0x19] = U_SPECIAL,
	[0x1a] = U_SPECIAL,
	[0x1b] = U_DOWN_ARROW,   /* Down Arrow */
	[0x1c] = U_RIGHT_ARROW,  /* Right Arrow */
	[0x1d] = U_ESCAPE,       /* Esc */
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
	[0x2b] = '\b',           /* Backspace */
	[0x2c] = U_SPECIAL,      /* Insert */
	[0x2d] = U_SPECIAL,
	[0x2e] = '/',            /* Numpad / */
	[0x2f] = '*',            /* Numpad * */
	[0x30] = U_SPECIAL,
	[0x31] = U_SPECIAL,
	[0x32] = '.',            /* Numpad . */
	[0x33] = U_SPECIAL,
	[0x34] = U_HOME_ARROW,   /* Home */
	[0x35] = '\t',           /* Tab */
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
	[0x42] = U_DELETE,       /* Delete */
	[0x43] = U_SPECIAL,
	[0x44] = '7',            /* Numpad 7 */
	[0x45] = '8',            /* Numpad 8 */
	[0x46] = '9',            /* Numpad 9 */
	[0x47] = '-',            /* Numpad - */
	[0x48] = U_SPECIAL,
	[0x49] = U_SPECIAL,
	[0x4a] = U_END_ARROW,    /* End */
	[0x4b] = U_SPECIAL,
	[0x4c] = U_SPECIAL,      /* Control */
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
	[0x59] = '\n',           /* Enter */
	[0x5a] = '\n',           /* Numpad Enter */
	[0x5b] = '4',            /* Numpad 4 */
	[0x5c] = '5',            /* Numpad 5 */
	[0x5d] = '6',            /* Numpad 6 */
	[0x5e] = '0',            /* Numpad 0 */
	[0x5f] = U_SPECIAL,
	[0x60] = U_PAGE_UP,      /* Page Up */
	[0x61] = U_SPECIAL,
	[0x62] = U_SPECIAL,      /* NumLock */
	[0x63] = U_SPECIAL,      /* Left Shift */
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
	[0x6e] = U_SPECIAL,      /* Right Shift */
	[0x6f] = U_SPECIAL,
	[0x70] = '1',            /* Numpad 1 */
	[0x71] = '2',            /* Numpad 2 */
	[0x72] = '3',            /* Numpad 3 */
	[0x73] = U_SPECIAL,
	[0x74] = U_SPECIAL,
	[0x75] = U_SPECIAL,
	[0x76] = U_SPECIAL,
	[0x77] = U_SPECIAL,      /* CapsLock */
	[0x78] = U_SPECIAL,
	[0x79] = ' ',
	[0x7a] = U_SPECIAL,
	[0x7b] = U_PAGE_DOWN,    /* Page Down */
	[0x7c] = U_SPECIAL,
	[0x7d] = '+',            /* Numpad + */
	[0x7e] = U_SPECIAL,
	[0x7f] = U_SPECIAL
};

/** @}
 */
