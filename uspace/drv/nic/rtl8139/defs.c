/*
 * SPDX-FileCopyrightText: 2011 Jiri Michalec
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
