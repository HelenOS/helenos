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

/** @addtogroup genarch
 * @{
 */
/**
 * @file
 * @brief Scan codes for PC keyboards.
 */

#include <genarch/kbrd/scanc.h>
#include <typedefs.h>
#include <str.h>

/** Primary meaning of scancodes. */
wchar_t sc_primary_map[SCANCODES] = {
	U_NULL,         /* 0x00 - undefined */
	U_ESCAPE,       /* 0x01 - Esc */
	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
	'\b',           /* 0x0e - Backspace */
	'\t',           /* 0x0f - Tab */
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
	'\n',           /* 0x1e - Enter */
	U_SPECIAL,      /* 0x1d - Left Ctrl */
	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
	U_SPECIAL,      /* 0x2a - Left Shift */
	'\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
	U_SPECIAL,      /* 0x36 - Right Shift */
	U_SPECIAL,      /* 0x37 - Print Screen */
	U_SPECIAL,      /* 0x38 - Left Alt */
	' ',
	U_SPECIAL,      /* 0x3a - CapsLock */
	U_SPECIAL,      /* 0x3b - F1 */
	U_SPECIAL,      /* 0x3c - F2 */
	U_SPECIAL,      /* 0x3d - F3 */
	U_SPECIAL,      /* 0x3e - F4 */
	U_SPECIAL,      /* 0x3f - F5 */
	U_SPECIAL,      /* 0x40 - F6 */
	U_SPECIAL,      /* 0x41 - F7 */
	U_SPECIAL,      /* 0x42 - F8 */
	U_SPECIAL,      /* 0x43 - F9 */
	U_SPECIAL,      /* 0x44 - F10 */
	U_SPECIAL,      /* 0x45 - NumLock */
	U_SPECIAL,      /* 0x46 - ScrollLock */
	U_HOME_ARROW,   /* 0x47 - Home */
	U_UP_ARROW,     /* 0x48 - Up Arrow */
	U_PAGE_UP,      /* 0x49 - Page Up */
	'-',
	U_LEFT_ARROW,   /* 0x4b - Left Arrow */
	'5',            /* 0x4c - Numpad Center */
	U_RIGHT_ARROW,  /* 0x4d - Right Arrow */
	'+',
	U_END_ARROW,    /* 0x4f - End */
	U_DOWN_ARROW,   /* 0x50 - Down Arrow */
	U_PAGE_DOWN,    /* 0x51 - Page Down */
	'0',            /* 0x52 - Numpad Insert */
	U_DELETE,       /* 0x53 - Delete */
	U_SPECIAL,      /* 0x54 - Alt-SysRq */
	U_SPECIAL,      /* 0x55 - F11/F12/PF1/FN */
	U_SPECIAL,      /* 0x56 - unlabelled key next to LAlt */
	U_SPECIAL,      /* 0x57 - F11 */
	U_SPECIAL,      /* 0x58 - F12 */
	U_SPECIAL,      /* 0x59 */
	U_SPECIAL,      /* 0x5a */
	U_SPECIAL,      /* 0x5b */
	U_SPECIAL,      /* 0x5c */
	U_SPECIAL,      /* 0x5d */
	U_SPECIAL,      /* 0x5e */
	U_SPECIAL,      /* 0x5f */
	U_SPECIAL,      /* 0x60 */
	U_SPECIAL,      /* 0x61 */
	U_SPECIAL,      /* 0x62 */
	U_SPECIAL,      /* 0x63 */
	U_SPECIAL,      /* 0x64 */
	U_SPECIAL,      /* 0x65 */
	U_SPECIAL,      /* 0x66 */
	U_SPECIAL,      /* 0x67 */
	U_SPECIAL,      /* 0x68 */
	U_SPECIAL,      /* 0x69 */
	U_SPECIAL,      /* 0x6a */
	U_SPECIAL,      /* 0x6b */
	U_SPECIAL,      /* 0x6c */
	U_SPECIAL,      /* 0x6d */
	U_SPECIAL,      /* 0x6e */
	U_SPECIAL,      /* 0x6f */
	U_SPECIAL,      /* 0x70 */
	U_SPECIAL,      /* 0x71 */
	U_SPECIAL,      /* 0x72 */
	U_SPECIAL,      /* 0x73 */
	U_SPECIAL,      /* 0x74 */
	U_SPECIAL,      /* 0x75 */
	U_SPECIAL,      /* 0x76 */
	U_SPECIAL,      /* 0x77 */
	U_SPECIAL,      /* 0x78 */
	U_SPECIAL,      /* 0x79 */
	U_SPECIAL,      /* 0x7a */
	U_SPECIAL,      /* 0x7b */
	U_SPECIAL,      /* 0x7c */
	U_SPECIAL,      /* 0x7d */
	U_SPECIAL,      /* 0x7e */
	U_SPECIAL       /* 0x7f */
};

/** Secondary meaning of scancodes. */
wchar_t sc_secondary_map[SCANCODES] = {
	U_NULL,         /* 0x00 - undefined */
	U_ESCAPE,       /* 0x01 - Esc */
	'!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
	'\b',           /* 0x0e - Backspace */
	'\t',           /* 0x0f - Tab */
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',
	'\n',           /* 0x1e - Enter */
	U_SPECIAL,      /* 0x1d - Left Ctrl */
	'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
	U_SPECIAL,      /* 0x2a - Left Shift */
	'|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
	U_SPECIAL,      /* 0x36 - Right Shift */
	U_SPECIAL,      /* 0x37 - Print Screen */
	U_SPECIAL,      /* 0x38 - Left Alt */
	' ',
	U_SPECIAL,      /* 0x3a - CapsLock */
	U_SPECIAL,      /* 0x3b - F1 */
	U_SPECIAL,      /* 0x3c - F2 */
	U_SPECIAL,      /* 0x3d - F3 */
	U_SPECIAL,      /* 0x3e - F4 */
	U_SPECIAL,      /* 0x3f - F5 */
	U_SPECIAL,      /* 0x40 - F6 */
	U_SPECIAL,      /* 0x41 - F7 */
	U_SPECIAL,      /* 0x42 - F8 */
	U_SPECIAL,      /* 0x43 - F9 */
	U_SPECIAL,      /* 0x44 - F10 */
	U_SPECIAL,      /* 0x45 - NumLock */
	U_SPECIAL,      /* 0x46 - ScrollLock */

	U_HOME_ARROW,   /* 0x47 - Home */
	U_UP_ARROW,     /* 0x48 - Up Arrow */
	U_PAGE_UP,      /* 0x49 - Page Up */
	'-',
	U_LEFT_ARROW,   /* 0x4b - Left Arrow */
	'5',            /* 0x4c - Numpad Center */
	U_RIGHT_ARROW,  /* 0x4d - Right Arrow */
	'+',
	U_END_ARROW,    /* 0x4f - End */
	U_DOWN_ARROW,   /* 0x50 - Down Arrow */
	U_PAGE_DOWN,    /* 0x51 - Page Down */
	'0',            /* 0x52 - Numpad Insert */
	U_DELETE,       /* 0x53 - Delete */
	U_SPECIAL,      /* 0x54 - Alt-SysRq */
	U_SPECIAL,      /* 0x55 - F11/F12/PF1/FN */
	U_SPECIAL,      /* 0x56 - unlabelled key next to LAlt */
	U_SPECIAL,      /* 0x57 - F11 */
	U_SPECIAL,      /* 0x58 - F12 */
	U_SPECIAL,      /* 0x59 */
	U_SPECIAL,      /* 0x5a */
	U_SPECIAL,      /* 0x5b */
	U_SPECIAL,      /* 0x5c */
	U_SPECIAL,      /* 0x5d */
	U_SPECIAL,      /* 0x5e */
	U_SPECIAL,      /* 0x5f */
	U_SPECIAL,      /* 0x60 */
	U_SPECIAL,      /* 0x61 */
	U_SPECIAL,      /* 0x62 */
	U_SPECIAL,      /* 0x63 */
	U_SPECIAL,      /* 0x64 */
	U_SPECIAL,      /* 0x65 */
	U_SPECIAL,      /* 0x66 */
	U_SPECIAL,      /* 0x67 */
	U_SPECIAL,      /* 0x68 */
	U_SPECIAL,      /* 0x69 */
	U_SPECIAL,      /* 0x6a */
	U_SPECIAL,      /* 0x6b */
	U_SPECIAL,      /* 0x6c */
	U_SPECIAL,      /* 0x6d */
	U_SPECIAL,      /* 0x6e */
	U_SPECIAL,      /* 0x6f */
	U_SPECIAL,      /* 0x70 */
	U_SPECIAL,      /* 0x71 */
	U_SPECIAL,      /* 0x72 */
	U_SPECIAL,      /* 0x73 */
	U_SPECIAL,      /* 0x74 */
	U_SPECIAL,      /* 0x75 */
	U_SPECIAL,      /* 0x76 */
	U_SPECIAL,      /* 0x77 */
	U_SPECIAL,      /* 0x78 */
	U_SPECIAL,      /* 0x79 */
	U_SPECIAL,      /* 0x7a */
	U_SPECIAL,      /* 0x7b */
	U_SPECIAL,      /* 0x7c */
	U_SPECIAL,      /* 0x7d */
	U_SPECIAL,      /* 0x7e */
	U_SPECIAL       /* 0x7f */
};

/** @}
 */
