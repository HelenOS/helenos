/*
 * Copyright (c) 2021 Jiri Svoboda
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

/** @addtogroup libcodepage
 * @{
 */
/**
 * @file Code page 437
 *
 * The ROM fonts of graphics adapters decending from IBM's MDA/CGA
 * use Code Page 437 and provide popular graphic characters
 * (such as box drawing) used in text-based user interfaces.
 */

#include <codepage/cp437.h>
#include <errno.h>
#include <stdint.h>
#include <str.h>

/** All 256 graphic characters of Code Page 437 represented in Unicode */
static char32_t cp437[256] = {
	/* 0x */
	L'\x0000', L'\x263a', L'\x263b', L'\x2665',
	L'\x2666', L'\x2663', L'\x2660', L'\x2022',
	L'\x25d8', L'\x25cb', L'\x25d9', L'\x2642',
	L'\x2640', L'\x266a', L'\x266b', L'\x263c',

	/* 1x */
	L'\x25ba', L'\x25c4', L'\x2195', L'\x203c',
	L'\x00b6', L'\x00a7', L'\x25ac', L'\x21a8',
	L'\x2191', L'\x2193', L'\x2192', L'\x2190',
	L'\x221f', L'\x2194', L'\x25b2', L'\x25bc',

	/* 2x */
	L'\x0020', L'\x0021', L'\x0022', L'\x0023',
	L'\x0024', L'\x0025', L'\x0026', L'\x0027',
	L'\x0028', L'\x0029', L'\x002a', L'\x002b',
	L'\x002c', L'\x002d', L'\x002e', L'\x002f',

	/* 3x */
	L'\x0030', L'\x0031', L'\x0032', L'\x0033',
	L'\x0034', L'\x0035', L'\x0036', L'\x0037',
	L'\x0038', L'\x0039', L'\x003a', L'\x003b',
	L'\x003c', L'\x003d', L'\x003e', L'\x003f',

	/* 4x */
	L'\x0040', L'\x0041', L'\x0042', L'\x0043',
	L'\x0044', L'\x0045', L'\x0046', L'\x0047',
	L'\x0048', L'\x0049', L'\x004a', L'\x004b',
	L'\x004c', L'\x004d', L'\x004e', L'\x004f',

	/* 5x */
	L'\x0050', L'\x0051', L'\x0052', L'\x0053',
	L'\x0054', L'\x0055', L'\x0056', L'\x0057',
	L'\x0058', L'\x0059', L'\x005a', L'\x005b',
	L'\x005c', L'\x005d', L'\x005e', L'\x005f',

	/* 6x */
	L'\x0060', L'\x0061', L'\x0062', L'\x0063',
	L'\x0064', L'\x0065', L'\x0066', L'\x0067',
	L'\x0068', L'\x0069', L'\x006a', L'\x006b',
	L'\x006c', L'\x006d', L'\x006e', L'\x006f',

	/* 7x */
	L'\x0070', L'\x0071', L'\x0072', L'\x0073',
	L'\x0074', L'\x0075', L'\x0076', L'\x0077',
	L'\x0078', L'\x0079', L'\x007a', L'\x007b',
	L'\x007c', L'\x007d', L'\x007e', L'\x2302',

	/* 8x */
	L'\x00c7', L'\x00fc', L'\x00e9', L'\x00e2',
	L'\x00e4', L'\x00e0', L'\x00e5', L'\x00e7',
	L'\x00ea', L'\x00eb', L'\x00e8', L'\x00ef',
	L'\x00ee', L'\x00ec', L'\x00c4', L'\x00c5',

	/* 9x */
	L'\x00c9', L'\x00e6', L'\x00c6', L'\x00f4',
	L'\x00f6', L'\x00f2', L'\x00fb', L'\x00f9',
	L'\x00ff', L'\x00d6', L'\x00dc', L'\x00a2',
	L'\x00a3', L'\x00a5', L'\x20a7', L'\x0192',

	/* Ax */
	L'\x00e1', L'\x00ed', L'\x00f3', L'\x00fa',
	L'\x00f1', L'\x00d1', L'\x00aa', L'\x00ba',
	L'\x00bf', L'\x2310', L'\x00ac', L'\x00bd',
	L'\x00bc', L'\x00a1', L'\x00ab', L'\x00bb',

	/* Bx */
	L'\x2591', L'\x2592', L'\x2593', L'\x2502',
	L'\x2524', L'\x2561', L'\x2562', L'\x2556',
	L'\x2555', L'\x2563', L'\x2551', L'\x2557',
	L'\x255d', L'\x255c', L'\x255b', L'\x2510',

	/* Cx */
	L'\x2514', L'\x2534', L'\x252c', L'\x251c',
	L'\x2500', L'\x253c', L'\x255e', L'\x255f',
	L'\x255a', L'\x2554', L'\x2569', L'\x2566',
	L'\x2560', L'\x2550', L'\x256c', L'\x2567',

	/* Dx */
	L'\x2568', L'\x2564', L'\x2565', L'\x2559',
	L'\x2558', L'\x2552', L'\x2553', L'\x256b',
	L'\x256a', L'\x2518', L'\x250c', L'\x2588',
	L'\x2584', L'\x258c', L'\x2590', L'\x2580',

	/* Ex */
	L'\x03b1', L'\x00df', L'\x0393', L'\x03c0',
	L'\x03a3', L'\x03c3', L'\x00b5', L'\x03c4',
	L'\x03a6', L'\x0398', L'\x03a9', L'\x03b4',
	L'\x221e', L'\x03c6', L'\x03b5', L'\x2229',

	/* Fx */
	L'\x2261', L'\x00b1', L'\x2265', L'\x2264',
	L'\x2320', L'\x2321', L'\x00f7', L'\x2248',
	L'\x00b0', L'\x2219', L'\x00b7', L'\x221a',
	L'\x207f', L'\x00b2', L'\x25a0', L'\x00a0'
};

/** Map of Unicode characters 0x0000 - 0x0400 to code page 437 */
static uint8_t u0xxx_to_cp437[0x400] = {
	/* 0x */
	[0x0000] = 0x00,

	/* 1x */
	[0x00b6] = 0x14, [0x00a7] = 0x15,

	/* 2x */
	[0x0020] = 0x20, [0x0021] = 0x21, [0x0022] = 0x22, [0x0023] = 0x23,
	[0x0024] = 0x24, [0x0025] = 0x25, [0x0026] = 0x26, [0x0027] = 0x27,
	[0x0028] = 0x28, [0x0029] = 0x29, [0x002a] = 0x2a, [0x002b] = 0x2b,
	[0x002c] = 0x2c, [0x002d] = 0x2d, [0x002e] = 0x2e, [0x002f] = 0x2f,

	/* 3x */
	[0x0030] = 0x30, [0x0031] = 0x31, [0x0032] = 0x32, [0x0033] = 0x33,
	[0x0034] = 0x34, [0x0035] = 0x35, [0x0036] = 0x36, [0x0037] = 0x37,
	[0x0038] = 0x38, [0x0039] = 0x39, [0x003a] = 0x3a, [0x003b] = 0x3b,
	[0x003c] = 0x3c, [0x003d] = 0x3d, [0x003e] = 0x3e, [0x003f] = 0x3f,

	/* 4x */
	[0x0040] = 0x40, [0x0041] = 0x41, [0x0042] = 0x42, [0x0043] = 0x43,
	[0x0044] = 0x44, [0x0045] = 0x45, [0x0046] = 0x46, [0x0047] = 0x47,
	[0x0048] = 0x48, [0x0049] = 0x49, [0x004a] = 0x4a, [0x004b] = 0x4b,
	[0x004c] = 0x4c, [0x004d] = 0x4d, [0x004e] = 0x4e, [0x004f] = 0x4f,

	/* 5x */
	[0x0050] = 0x50, [0x0051] = 0x51, [0x0052] = 0x52, [0x0053] = 0x53,
	[0x0054] = 0x54, [0x0055] = 0x55, [0x0056] = 0x56, [0x0057] = 0x57,
	[0x0058] = 0x58, [0x0059] = 0x59, [0x005a] = 0x5a, [0x005b] = 0x5b,
	[0x005c] = 0x5c, [0x005d] = 0x5d, [0x005e] = 0x5e, [0x005f] = 0x5f,

	/* 6x */
	[0x0060] = 0x60, [0x0061] = 0x61, [0x0062] = 0x62, [0x0063] = 0x63,
	[0x0064] = 0x64, [0x0065] = 0x65, [0x0066] = 0x66, [0x0067] = 0x67,
	[0x0068] = 0x68, [0x0069] = 0x69, [0x006a] = 0x6a, [0x006b] = 0x6b,
	[0x006c] = 0x6c, [0x006d] = 0x6d, [0x006e] = 0x6e, [0x006f] = 0x6f,

	/* 7x */
	[0x0070] = 0x70, [0x0071] = 0x71, [0x0072] = 0x72, [0x0073] = 0x73,
	[0x0074] = 0x74, [0x0075] = 0x75, [0x0076] = 0x76, [0x0077] = 0x77,
	[0x0078] = 0x78, [0x0079] = 0x79, [0x007a] = 0x7a, [0x007b] = 0x7b,
	[0x007c] = 0x7c, [0x007d] = 0x7d, [0x007e] = 0x7e,

	/* 8x */
	[0x00c7] = 0x80, [0x00fc] = 0x81, [0x00e9] = 0x82, [0x00e2] = 0x83,
	[0x00e4] = 0x84, [0x00e0] = 0x85, [0x00e5] = 0x86, [0x00e7] = 0x87,
	[0x00ea] = 0x88, [0x00eb] = 0x89, [0x00e8] = 0x8a, [0x00ef] = 0x8b,
	[0x00ee] = 0x8c, [0x00ec] = 0x8d, [0x00c4] = 0x8e, [0x00c5] = 0x8f,

	/* 9x */
	[0x00c9] = 0x90, [0x00e6] = 0x91, [0x00c6] = 0x92, [0x00f4] = 0x93,
	[0x00f6] = 0x94, [0x00f2] = 0x95, [0x00fb] = 0x96, [0x00f9] = 0x97,
	[0x00ff] = 0x98, [0x00d6] = 0x99, [0x00dc] = 0x9a, [0x00a2] = 0x9b,
	[0x00a3] = 0x9c, [0x00a5] = 0x9d,                  [0x0192] = 0x9f,

	/* Ax */
	[0x00e1] = 0xa0, [0x00ed] = 0xa1, [0x00f3] = 0xa2, [0x00fa] = 0xa3,
	[0x00f1] = 0xa4, [0x00d1] = 0xa5, [0x00aa] = 0xa6, [0x00ba] = 0xa7,
	[0x00bf] = 0xa8,                  [0x00ac] = 0xaa, [0x00bd] = 0xab,
	[0x00bc] = 0xac, [0x00a1] = 0xad, [0x00ab] = 0xae, [0x00bb] = 0xaf,

	/* Ex */
	[0x03b1] = 0xe0, [0x00df] = 0xe1, [0x0393] = 0xe2, [0x03c0] = 0xe3,
	[0x03a3] = 0xe4, [0x03c3] = 0xe5, [0x00b5] = 0xe6, [0x03c4] = 0xe7,
	[0x03a6] = 0xe8, [0x0398] = 0xe9, [0x03a9] = 0xea, [0x03b4] = 0xeb,
	/* skipped */    [0x03c6] = 0xed, [0x03b5] = 0xee,

	/* Fx */
	/* skipped */    [0x00b1] = 0xf1,
	/* skipped */                     [0x00f7] = 0xf6,
	[0x00b0] = 0xf8, [0x00b7] = 0xfa,
	/* skipped */    [0x00b2] = 0xfd,                  [0x00a0] = 0xff,
};

/** Map of Unicode characters 0x2000 - 0x2700 to code page 437 */
static uint8_t u2xxx_to_cp437[0x700] = {
	/* 0x */
	/* skipped */   [0x63a] = 0x01, [0x63b] = 0x02, [0x665] = 0x03,
	[0x666] = 0x04, [0x663] = 0x05, [0x660] = 0x06, [0x022] = 0x07,
	[0x5d8] = 0x08, [0x5cb] = 0x09, [0x5d9] = 0x0a, [0x642] = 0x0b,
	[0x640] = 0x0c, [0x66a] = 0x0d, [0x66b] = 0x0e, [0x63c] = 0x0f,

	/* 1x */
	[0x5ba] = 0x10, [0x5c4] = 0x11, [0x195] = 0x12, [0x03c] = 0x13,
	/* skipped */                   [0x5ac] = 0x16, [0x1a8] = 0x17,
	[0x191] = 0x18, [0x193] = 0x19, [0x192] = 0x1a, [0x190] = 0x1b,
	[0x21f] = 0x1c, [0x194] = 0x1d, [0x5b2] = 0x1e, [0x5bc] = 0x1f,

	/* 7x */
	[0x302] = 0x7f,

	/* 9x */
	[0x0a7] = 0x9e,

	/* Ax */
	[0x310] = 0xa9,

	/* Bx */
	[0x591] = 0xb0, [0x592] = 0xb1, [0x593] = 0xb2, [0x502] = 0xb3,
	[0x524] = 0xb4, [0x561] = 0xb5, [0x562] = 0xb6, [0x556] = 0xb7,
	[0x555] = 0xb8, [0x563] = 0xb9, [0x551] = 0xba, [0x557] = 0xbb,
	[0x55d] = 0xbc, [0x55c] = 0xbd, [0x55b] = 0xbe, [0x510] = 0xbf,

	/* Cx */
	[0x514] = 0xc0, [0x534] = 0xc1, [0x52c] = 0xc2, [0x51c] = 0xc3,
	[0x500] = 0xc4, [0x53c] = 0xc5, [0x55e] = 0xc6, [0x55f] = 0xc7,
	[0x55a] = 0xc8, [0x554] = 0xc9, [0x569] = 0xca, [0x566] = 0xcb,
	[0x560] = 0xcc, [0x550] = 0xcd, [0x56c] = 0xce, [0x567] = 0xcf,

	/* Dx */
	[0x568] = 0xd0, [0x564] = 0xd1, [0x565] = 0xd2, [0x559] = 0xd3,
	[0x558] = 0xd4, [0x552] = 0xd5, [0x553] = 0xd6, [0x56b] = 0xd7,
	[0x56a] = 0xd8, [0x518] = 0xd9, [0x50c] = 0xda, [0x588] = 0xdb,
	[0x584] = 0xdc, [0x58c] = 0xdd, [0x590] = 0xde, [0x580] = 0xdf,

	/* Ex */
	[0x21e] = 0xec, [0x229] = 0xef,

	/* Fx */
	[0x261] = 0xf0,                 [0x265] = 0xf2, [0x264] = 0xf3,
	[0x320] = 0xf4, [0x321] = 0xf5,                 [0x248] = 0xf7,
	/* skipped */   [0x219] = 0xf9,                 [0x21a] = 0xfb,
	[0x07f] = 0xfc,                 [0x5a0] = 0xfe
};

/** Decode character from code page 437 8-bit code.
 *
 * Note that this function considers all code page 437 members as graphic
 * characters (including those in range 0-31), not control characters.
 *
 * @param code 8-bit code
 * @return Character
 */
char32_t cp437_decode(uint8_t code)
{
	return cp437[code];
}

/** Encode character to code page 437 8-bit code.
 *
 * Note that this function considers all code page 437 members as graphic
 * characters (including those in range 0-31), not control characters.
 *
 * @param c Character
 * @param code Place to store 8-bit code
 * @return EOK on success, EDOM if character cannot be encoded to CP437.
 */
errno_t cp437_encode(char32_t c, uint8_t *code)
{
	uint8_t b;

	/* Unicode character 0 is the only that can map to cp437 code 0 */
	if (c == 0) {
		*code = 0;
		return EOK;
	}

	b = 0;

	/*
	 * The map is split into two parts, 0x0000 - 0x0400 and
	 * 0x2000 - 0x2700. This seems like a reasonable compromise
	 * between complexity and memory efficiency.
	 */

	if (c < 0x400)
		b = u0xxx_to_cp437[c];
	else if (c >= 0x2000 && c < 0x2700)
		b = u2xxx_to_cp437[c - 0x2000];

	/*
	 * If we got zero, it was an uninitialized entry (since no Unicode
	 * character except 0 can map to b == 0 and that was already
	 * taken care of).
	 */
	if (b == 0)
		return EDOM;

	*code = b;
	return EOK;
}

/** @}
 */
