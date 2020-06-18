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

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief Scan codes for Macintosh ADB keyboards.
 */

#include <genarch/kbrd/scanc.h>
#include <typedefs.h>
#include <str.h>

/** Primary meaning of scancodes. */
char32_t sc_primary_map[SCANCODES] = {
	[0x00] = 'a',
	[0x01] = 's',
	[0x02] = 'd',
	[0x03] = 'f',
	[0x04] = 'h',
	[0x05] = 'g',
	[0x06] = 'z',
	[0x07] = 'x',
	[0x08] = 'c',
	[0x09] = 'v',
	[0x0a] = U_SPECIAL,
	[0x0b] = 'b',
	[0x0c] = 'q',
	[0x0d] = 'w',
	[0x0e] = 'e',
	[0x0f] = 'r',
	[0x10] = 'y',
	[0x11] = 't',
	[0x12] = '1',
	[0x13] = '2',
	[0x14] = '3',
	[0x15] = '4',
	[0x16] = '6',
	[0x17] = '5',
	[0x18] = '=',
	[0x19] = '9',
	[0x1a] = '7',
	[0x1b] = '-',
	[0x1c] = '8',
	[0x1d] = '0',
	[0x1e] = ']',
	[0x1f] = 'o',
	[0x20] = 'u',
	[0x21] = '[',
	[0x22] = 'i',
	[0x23] = 'p',
	[0x24] = '\n',		/* Enter */
	[0x25] = 'l',
	[0x26] = 'j',
	[0x27] = '\'',
	[0x28] = 'k',
	[0x29] = ';',
	[0x2a] = '\\',
	[0x2b] = ',',
	[0x2c] = '/',
	[0x2d] = 'n',
	[0x2e] = 'm',
	[0x2f] = '.',
	[0x30] = '\t',		/* Tab */
	[0x31] = ' ',		/* Space */
	[0x32] = '`',
	[0x33] = '\b',		/* Backspace */
	[0x34] = U_SPECIAL,
	[0x35] = U_ESCAPE,
	[0x36] = U_SPECIAL,
	[0x37] = U_SPECIAL,
	[0x38] = U_SPECIAL,
	[0x39] = U_SPECIAL,
	[0x3a] = U_SPECIAL,
	[0x3b] = U_LEFT_ARROW,
	[0x3c] = U_RIGHT_ARROW,
	[0x3d] = U_DOWN_ARROW,
	[0x3e] = U_UP_ARROW,
	[0x3f] = U_SPECIAL,
	[0x40] = U_SPECIAL,
	[0x41] = '.',		/* Num Separator */
	[0x42] = U_SPECIAL,
	[0x43] = '*',		/* Num Times */
	[0x44] = U_SPECIAL,
	[0x45] = '+',		/* Num Plus */
	[0x46] = U_SPECIAL,
	[0x47] = U_SPECIAL,
	[0x48] = U_SPECIAL,
	[0x49] = U_SPECIAL,
	[0x4a] = U_SPECIAL,
	[0x4b] = '/',		/* Num Divide */
	[0x4c] = U_SPECIAL,
	[0x4d] = U_SPECIAL,
	[0x4e] = '-',		/* Num Minus */
	[0x4f] = U_SPECIAL,
	[0x50] = U_SPECIAL,
	[0x51] = U_SPECIAL,
	[0x52] = '0',		/* Num Zero */
	[0x53] = '1',		/* Num One */
	[0x54] = '2',		/* Num Two */
	[0x55] = '3',		/* Num Three */
	[0x56] = '4',		/* Num Four */
	[0x57] = '5',		/* Num Five */
	[0x58] = '6',		/* Num Six */
	[0x59] = '7',		/* Num Seven */
	[0x5a] = U_SPECIAL,
	[0x5b] = '8',		/* Num Eight */
	[0x5c] = '9',		/* Num Nine */
	[0x5d] = U_SPECIAL,
	[0x5e] = U_SPECIAL,
	[0x5f] = U_SPECIAL,
	[0x60] = U_SPECIAL,
	[0x61] = U_SPECIAL,
	[0x62] = U_SPECIAL,
	[0x63] = U_SPECIAL,
	[0x64] = U_SPECIAL,
	[0x65] = U_SPECIAL,
	[0x66] = U_SPECIAL,
	[0x67] = U_SPECIAL,
	[0x68] = U_SPECIAL,
	[0x69] = U_SPECIAL,
	[0x6a] = U_SPECIAL,
	[0x6b] = U_SPECIAL,
	[0x6c] = U_SPECIAL,
	[0x6d] = U_SPECIAL,
	[0x6e] = U_SPECIAL,
	[0x6f] = U_SPECIAL,
	[0x70] = U_SPECIAL,
	[0x71] = U_SPECIAL,
	[0x72] = U_SPECIAL,
	[0x73] = U_HOME_ARROW,
	[0x74] = U_PAGE_UP,
	[0x75] = U_DELETE,
	[0x76] = U_SPECIAL,
	[0x77] = U_SPECIAL,
	[0x78] = U_SPECIAL,
	[0x79] = U_PAGE_DOWN,
	[0x7a] = U_SPECIAL,
	[0x7b] = U_SPECIAL,
	[0x7c] = U_SPECIAL,
	[0x7d] = U_SPECIAL,
	[0x7e] = U_SPECIAL,
	[0x7f] = U_SPECIAL
};

/** Secondary meaning of scancodes. */
char32_t sc_secondary_map[SCANCODES] = {
	[0x00] = 'A',
	[0x01] = 'S',
	[0x02] = 'D',
	[0x03] = 'F',
	[0x04] = 'H',
	[0x05] = 'G',
	[0x06] = 'Z',
	[0x07] = 'X',
	[0x08] = 'C',
	[0x09] = 'V',
	[0x0a] = U_SPECIAL,
	[0x0b] = 'B',
	[0x0c] = 'Q',
	[0x0d] = 'W',
	[0x0e] = 'E',
	[0x0f] = 'R',
	[0x10] = 'Y',
	[0x11] = 'T',
	[0x12] = '!',
	[0x13] = '@',
	[0x14] = '#',
	[0x15] = '$',
	[0x16] = '^',
	[0x17] = '%',
	[0x18] = '+',
	[0x19] = '(',
	[0x1a] = '&',
	[0x1b] = '_',
	[0x1c] = '*',
	[0x1d] = ')',
	[0x1e] = '}',
	[0x1f] = 'O',
	[0x20] = 'U',
	[0x21] = '{',
	[0x22] = 'I',
	[0x23] = 'P',
	[0x24] = '\n',		/* Enter */
	[0x25] = 'L',
	[0x26] = 'J',
	[0x27] = '"',
	[0x28] = 'K',
	[0x29] = ':',
	[0x2a] = '|',
	[0x2b] = '<',
	[0x2c] = '?',
	[0x2d] = 'N',
	[0x2e] = 'M',
	[0x2f] = '>',
	[0x30] = '\t',		/* Tab */
	[0x31] = ' ',		/* Space */
	[0x32] = '~',
	[0x33] = '\b',		/* Backspace */
	[0x34] = U_SPECIAL,
	[0x35] = U_SPECIAL,
	[0x36] = U_SPECIAL,
	[0x37] = U_SPECIAL,
	[0x38] = U_SPECIAL,
	[0x39] = U_SPECIAL,
	[0x3a] = U_SPECIAL,
	[0x3b] = U_SPECIAL,
	[0x3c] = U_SPECIAL,
	[0x3d] = U_SPECIAL,
	[0x3e] = U_SPECIAL,
	[0x3f] = U_SPECIAL,
	[0x40] = U_SPECIAL,
	[0x41] = '.',		/* Num Separator */
	[0x42] = U_SPECIAL,
	[0x43] = '*',		/* Num Times */
	[0x44] = U_SPECIAL,
	[0x45] = '+',		/* Num Plus */
	[0x46] = U_SPECIAL,
	[0x47] = U_SPECIAL,
	[0x48] = U_SPECIAL,
	[0x49] = U_SPECIAL,
	[0x4a] = U_SPECIAL,
	[0x4b] = '/',		/* Num Divide */
	[0x4c] = U_SPECIAL,
	[0x4d] = U_SPECIAL,
	[0x4e] = '-',		/* Num Minus */
	[0x4f] = U_SPECIAL,
	[0x50] = U_SPECIAL,
	[0x51] = U_SPECIAL,
	[0x52] = '0',		/* Num Zero */
	[0x53] = '1',		/* Num One */
	[0x54] = '2',		/* Num Two */
	[0x55] = '3',		/* Num Three */
	[0x56] = '4',		/* Num Four */
	[0x57] = '5',		/* Num Five */
	[0x58] = '6',		/* Num Six */
	[0x59] = '7',		/* Num Seven */
	[0x5a] = U_SPECIAL,
	[0x5b] = '8',		/* Num Eight */
	[0x5c] = '9',		/* Num Nine */
	[0x5d] = U_SPECIAL,
	[0x5e] = U_SPECIAL,
	[0x5f] = U_SPECIAL,
	[0x60] = U_SPECIAL,
	[0x61] = U_SPECIAL,
	[0x62] = U_SPECIAL,
	[0x63] = U_SPECIAL,
	[0x64] = U_SPECIAL,
	[0x65] = U_SPECIAL,
	[0x66] = U_SPECIAL,
	[0x67] = U_SPECIAL,
	[0x68] = U_SPECIAL,
	[0x69] = U_SPECIAL,
	[0x6a] = U_SPECIAL,
	[0x6b] = U_SPECIAL,
	[0x6c] = U_SPECIAL,
	[0x6d] = U_SPECIAL,
	[0x6e] = U_SPECIAL,
	[0x6f] = U_SPECIAL,
	[0x70] = U_SPECIAL,
	[0x71] = U_SPECIAL,
	[0x72] = U_SPECIAL,
	[0x73] = U_SPECIAL,
	[0x74] = U_SPECIAL,
	[0x75] = U_SPECIAL,
	[0x76] = U_SPECIAL,
	[0x77] = U_SPECIAL,
	[0x78] = U_SPECIAL,
	[0x79] = U_SPECIAL,
	[0x7a] = U_SPECIAL,
	[0x7b] = U_SPECIAL,
	[0x7c] = U_SPECIAL,
	[0x7d] = U_SPECIAL,
	[0x7e] = U_SPECIAL,
	[0x7f] = U_SPECIAL
};

/** @}
 */
