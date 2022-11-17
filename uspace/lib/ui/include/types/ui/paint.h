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

/** @addtogroup libui
 * @{
 */
/**
 * @file Painting routines
 */

#ifndef _UI_TYPES_PAINT_H
#define _UI_TYPES_PAINT_H

#include <gfx/color.h>
#include <gfx/font.h>

/** Filled circle parts */
typedef enum {
	/** Upper-left half */
	ui_fcircle_upleft = 0x1,
	/** Lower-right half */
	ui_fcircle_lowright = 0x2,
	/** Entire circle */
	ui_fcircle_entire =
	    ui_fcircle_upleft |
	    ui_fcircle_lowright
} ui_fcircle_part_t;

/** Box characters for a particular box style.
 *
 * The first coordinate selects top, center, bottom,
 * the second coordinate selects left, center, right.
 * The top/bottom, left/right characters correpsond to corners,
 * the characters with one center coordinate correspond to edges:
 *
 *    0   1   2
 * 0 '+' '-' '+'
 * 1 ' |' ' ' '|'
 * 2 '+' '-' '+'
 *
 */
typedef struct {
	const char *c[3][3];
} ui_box_chars_t;

/** Horizontal brace characters for a particular box style. */
typedef struct {
	const char *start;
	const char *middle;
	const char *end;
} ui_brace_chars_t;

/** Box style */
typedef enum {
	/** Single box */
	ui_box_single,
	/** Double box */
	ui_box_double
} ui_box_style_t;

/** UI text formatting */
typedef struct {
	/** Text font */
	gfx_font_t *font;
	/** Standard color */
	gfx_color_t *color;
	/** Highlight color */
	gfx_color_t *hgl_color;
	/** Horizontal alignment */
	gfx_halign_t halign;
	/** Width available for the text */
	gfx_coord_t width;
	/** Vertical alignment */
	gfx_valign_t valign;
} ui_text_fmt_t;

#endif

/** @}
 */
