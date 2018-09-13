/*
 * Copyright (c) 2011 Jiri Michalec
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

#include "defs.h"

const char *model_names[RTL8139_VER_COUNT] = {
	"RTL8139",
	"RTL8139A",
	"RTL8139A_G",
	"RTL8139B",
	"RTL8130",
	"RTL8139C",
	"RTL8100",
	"RTL8139C+",
	"RTL8139D",
	"RTL8101"
};

#define HWVER(b1, b2, b3, b4, b5, b6, b7) ((b1 << 6) | (b2 << 5) | (b3 << 4) \
    | (b4 << 3) | (b5 << 2) | (b6 << 1) | (b7))

const struct rtl8139_hwver_map rtl8139_versions[RTL8139_VER_COUNT + 1] = {
	{ HWVER(1, 1, 0, 0, 0, 0, 0), RTL8139 },
	{ HWVER(1, 1, 1, 0, 0, 0, 0), RTL8139A },
	{ HWVER(1, 1, 1, 0, 0, 1, 0), RTL8139A_G },
	{ HWVER(1, 1, 1, 1, 0, 0, 0), RTL8139B },
	{ HWVER(1, 1, 1, 1, 1, 0, 0), RTL8130 },
	{ HWVER(1, 1, 1, 0, 1, 0, 0), RTL8139C },
	{ HWVER(1, 1, 1, 1, 0, 1, 0), RTL8100 },
	{ HWVER(1, 1, 1, 0, 1, 0, 1), RTL8139D },
	{ HWVER(1, 1, 1, 0, 1, 1, 0), RTL8139Cp },
	{ HWVER(1, 1, 1, 0, 1, 1, 1), RTL8101 },
	/* End value */
	{ 0, RTL8139_VER_COUNT }
};
