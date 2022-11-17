/*
 * Copyright (c) 2022 Jiri Svoboda
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

/** @addtogroup libgfxfont
 * @{
 */
/**
 * @file Text types
 */

#ifndef _TYPES_GFX_TEXT_H
#define _TYPES_GFX_TEXT_H

#include <stdbool.h>
#include <types/gfx/coord.h>
#include <types/gfx/color.h>

/** Text horizontal alignment */
typedef enum {
	/** Align text left (start at anchor point) */
	gfx_halign_left,
	/** Align text on the center (center around anchor point) */
	gfx_halign_center,
	/** Align text right (end just before anchor point) */
	gfx_halign_right,
	/** Justify text on both left and right edge */
	gfx_halign_justify
} gfx_halign_t;

/** Text vertical alignment */
typedef enum {
	/** Align top (starts at anchor point) */
	gfx_valign_top,
	/** Align center (centered around anchor point) */
	gfx_valign_center,
	/** Align bottom (end just before anchor point) */
	gfx_valign_bottom,
	/** Align to baseline */
	gfx_valign_baseline
} gfx_valign_t;

/** Text formatting */
typedef struct {
	/** Text font */
	struct gfx_font *font;
	/** Text color */
	gfx_color_t *color;
	/** Horizontal alignment */
	gfx_halign_t halign;
	/** Width available for the text */
	gfx_coord_t width;
	/** Vertical alignment */
	gfx_valign_t valign;
	/** Abbreviate the text with ellipsis if it does not fit @c width */
	bool abbreviate;
	/** Underline */
	bool underline;
} gfx_text_fmt_t;

#endif

/** @}
 */
