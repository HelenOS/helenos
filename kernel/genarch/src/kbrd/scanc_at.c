/*
 * Copyright (c) 2009 Vineeth Pillai
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
 * INCIDENTAL, U_SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
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
 * @brief	Scan codes for PC/AT keyboards (set 2).
 */
#include <genarch/kbrd/scanc.h>
#include <typedefs.h>
#include <str.h>


/** Primary meaning of scancodes. */
wchar_t sc_primary_map[] = {
	U_NULL, /* 0x00 */
	U_SPECIAL, /* 0x01 - F9 */
	U_SPECIAL, /* 0x02 - F7 */
	U_SPECIAL, /* 0x03 - F5 */
	U_SPECIAL, /* 0x04 - F3 */
	U_SPECIAL, /* 0x05 - F1 */
	U_SPECIAL, /* 0x06 - F2 */
	U_SPECIAL, /* 0x07 - F12 */
	U_SPECIAL, /* 0x08 -  */
	U_SPECIAL, /* 0x09 - F10 */
	U_SPECIAL, /* 0x0A - F8 */
	U_SPECIAL, /* 0x0B - F10 */
	U_SPECIAL, /* 0x0C - F4 */
	'\t', /* 0x0D - Tab */
	'`',
	U_SPECIAL, /* 0x0F */
	U_SPECIAL, /* 0x10 */
	U_SPECIAL, /* 0x11 - LAlt */
	U_SPECIAL, /* 0x12 - LShift */
	U_SPECIAL, /* ox13 */
	U_SPECIAL, /* 0x14 Ctrl */
	'q', '1',
	U_SPECIAL, /* 0x17 */
	U_SPECIAL, /* 0x18 */
	U_SPECIAL, /* 0x19 */
	'z', 's', 'a', 'w', '2',
	U_SPECIAL, /* 0x1F */
	U_SPECIAL, /* 0x20 */
	'c', 'x', 'd', 'e', '4', '3',
	U_SPECIAL, /* 0x27 */
	U_SPECIAL, /* 0x28 */
	' ', 'v', 'f', 't', 'r', '5',
	U_SPECIAL, /* 0x2F */
	U_SPECIAL, /* 0x30 */
	'n', 'b', 'h', 'g', 'y', '6',
	U_SPECIAL, /* 0x37 */
	U_SPECIAL, /* 0x38 */
	U_SPECIAL, /* 0x39 */
	'm', 'j', 'u', '7', '8',
	U_SPECIAL, /* 0x3F */
	U_SPECIAL, /* 0x40 */
	',', 'k', 'i', 'o', '0', '9',
	U_SPECIAL, /* 0x47 */
	U_SPECIAL, /* 0x48 */
	'.', '/', 'l', ';', 'p', '-',
	U_SPECIAL, /* 0x4F */
	U_SPECIAL, /* 0x50 */
	U_SPECIAL, /* 0x51 */
	'\'',
	U_SPECIAL, /* 0x53 */
	'[', '=',
	U_SPECIAL, /* 0x56 */
	U_SPECIAL, /* 0x57 */
	U_SPECIAL, /* 0x58 - Caps Lock */
	U_SPECIAL, /* 0x59 - RShift */
	'\n', ']',
	U_SPECIAL, /* 0x5C */
	'\\',
	U_SPECIAL, /* 0x5E */
	U_SPECIAL, /* 0x5F */
	U_SPECIAL, /* 0x60 */
	U_SPECIAL, /* 0x61 */
	U_SPECIAL, /* 0x62 */
	U_SPECIAL, /* 0x63 */
	U_SPECIAL, /* 0x64 */
	U_SPECIAL, /* 0x65 */
	'\b', /* 0x66  - backspace*/
	U_SPECIAL, /* 0x67 */
	U_SPECIAL, /* 0x68 */
	U_END_ARROW, /* 0x69 */
	U_SPECIAL, /* 0x6a */
	U_LEFT_ARROW, /* 0x6b - Left Arrow */
	U_SPECIAL, /* 0x6c */
	U_SPECIAL, /* 0x6d */
	U_SPECIAL, /* 0x6e */
	U_SPECIAL, /* 0x6f */
	U_SPECIAL, /* 0x70 */
	U_DELETE, /* 0x71 - Del*/
	U_DOWN_ARROW, /* 0x72 Down Arrow */
	U_SPECIAL, /* 0x73 */
	U_RIGHT_ARROW, /* 0x74  - Right Arrow */
	U_UP_ARROW, /* 0x75  Up Arrow */
	U_ESCAPE, /* 0x76 Esc */
	U_SPECIAL, /* 0x77 - NumLock*/
	U_SPECIAL, /* 0x78  F11*/
	U_SPECIAL, /* 0x79 */
	U_PAGE_DOWN, /* 0x7a */
	U_SPECIAL, /* 0x7b */
	U_SPECIAL, /* 0x7c */
	U_PAGE_UP, /* 0x7d */
	U_SPECIAL, /* 0x7e */
	U_SPECIAL /* 0x7f */
};

/** Secondary meaning of scancodes. */
wchar_t sc_secondary_map[] = {
	U_NULL, /* 0x00 */
	U_SPECIAL, /* 0x01 - F9 */
	U_SPECIAL, /* 0x02 - F7 */
	U_SPECIAL, /* 0x03 - F5 */
	U_SPECIAL, /* 0x04 - F3 */
	U_SPECIAL, /* 0x05 - F1 */
	U_SPECIAL, /* 0x06 - F2 */
	U_SPECIAL, /* 0x07 - F12 */
	U_SPECIAL, /* 0x08 -  */
	U_SPECIAL, /* 0x09 - F10 */
	U_SPECIAL, /* 0x0A - F8 */
	U_SPECIAL, /* 0x0B - F10 */
	U_SPECIAL, /* 0x0C - F4 */
	'\t', /* 0x0D - Tab */
	'~',
	U_SPECIAL, /* 0x0F */
	U_SPECIAL, /* 0x10 */
	U_SPECIAL, /* 0x11 - LAlt */
	U_SPECIAL, /* 0x12 - LShift */
	U_SPECIAL, /* ox13 */
	U_SPECIAL, /* 0x14 Ctrl */
	'Q', '!',
	U_SPECIAL, /* 0x17 */
	U_SPECIAL, /* 0x18 */
	U_SPECIAL, /* 0x19 */
	'Z', 'S', 'A', 'W', '@',
	U_SPECIAL, /* 0x1F */
	U_SPECIAL, /* 0x20 */
	'C', 'X', 'D', 'E', '$', '#',
	U_SPECIAL, /* 0x27 */
	U_SPECIAL, /* 0x28 */
	' ', 'V', 'F', 'T', 'R', '%',
	U_SPECIAL, /* 0x2F */
	U_SPECIAL, /* 0x30 */
	'N', 'B', 'H', 'G', 'Y', '^',
	U_SPECIAL, /* 0x37 */
	U_SPECIAL, /* 0x38 */
	U_SPECIAL, /* 0x39 */
	'M', 'J', 'U', '&', '*',
	U_SPECIAL, /* 0x3F */
	U_SPECIAL, /* 0x40 */
	'<', 'K', 'I', 'O', ')', '(',
	U_SPECIAL, /* 0x47 */
	U_SPECIAL, /* 0x48 */
	'>', '?', 'L', ':', 'P', '_',
	U_SPECIAL, /* 0x4F */
	U_SPECIAL, /* 0x50 */
	U_SPECIAL, /* 0x51 */
	'"',
	U_SPECIAL, /* 0x53 */
	'{', '+',
	U_SPECIAL, /* 0x56 */
	U_SPECIAL, /* 0x57 */
	U_SPECIAL, /* 0x58 - Caps Lock */
	U_SPECIAL, /* 0x59 - RShift */
	'\n', '}',
	U_SPECIAL, /* 0x5C */
	'|',
	U_SPECIAL, /* 0x5E */
	U_SPECIAL, /* 0x5F */
	U_SPECIAL, /* 0x60 */
	U_SPECIAL, /* 0x61 */
	U_SPECIAL, /* 0x62 */
	U_SPECIAL, /* 0x63 */
	U_SPECIAL, /* 0x64 */
	U_SPECIAL, /* 0x65 */
	'\b', /* 0x66  - backspace*/
	U_SPECIAL, /* 0x67 */
	U_SPECIAL, /* 0x68 */
	U_END_ARROW, /* 0x69 */
	U_SPECIAL, /* 0x6a */
	U_LEFT_ARROW, /* 0x6b - Left Arrow */
	U_SPECIAL, /* 0x6c */
	U_SPECIAL, /* 0x6d */
	U_SPECIAL, /* 0x6e */
	U_SPECIAL, /* 0x6f */
	U_SPECIAL, /* 0x70 */
	U_DELETE, /* 0x71 - Del*/
	U_DOWN_ARROW, /* 0x72 Down Arrow */
	U_SPECIAL, /* 0x73 */
	U_RIGHT_ARROW, /* 0x74  - Right Arrow */
	U_UP_ARROW, /* 0x75  Up Arrow */
	U_ESCAPE, /* 0x76 Esc */
	U_SPECIAL, /* 0x77 - NumLock*/
	U_SPECIAL, /* 0x78  F11*/
	U_SPECIAL, /* 0x79 */
	U_PAGE_DOWN, /* 0x7a */
	U_SPECIAL, /* 0x7b */
	U_SPECIAL, /* 0x7c */
	U_PAGE_UP, /* 0x7d */
	U_SPECIAL, /* 0x7e */
	U_SPECIAL /* 0x7f */
};

/** @}
 */
