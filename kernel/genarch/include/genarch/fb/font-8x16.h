/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/** @file
 */

#ifndef KERN_FONT_8X16_H_
#define KERN_FONT_8X16_H_

#define FONT_GLYPHS     2899
#define FONT_WIDTH      8
#define FONT_SCANLINES  16

#include <typedefs.h>

extern uint16_t fb_font_glyph(const char32_t ch);
extern uint8_t fb_font[FONT_GLYPHS][FONT_SCANLINES];

#endif

/** @}
 */
