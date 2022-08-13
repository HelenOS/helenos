/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 * SPDX-FileCopyrightText: 2012 Petr Koupy
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup output
 * @{
 */
/** @file
 */

#ifndef FONT_8X16_H_
#define FONT_8X16_H_

#include <stdint.h>
#include <str.h>

#define FONT_GLYPHS     2899
#define FONT_WIDTH      8
#define FONT_SCANLINES  16
#define FONT_ASCENDER   12

extern uint16_t fb_font_glyph(const char32_t, bool *);
extern uint8_t fb_font[FONT_GLYPHS][FONT_SCANLINES];

#endif

/** @}
 */
