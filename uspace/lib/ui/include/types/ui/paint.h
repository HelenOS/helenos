/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
	/** Justification width (for gfx_halign_justify) */
	gfx_coord_t justify_width;
	/** Vertical alignment */
	gfx_valign_t valign;
} ui_text_fmt_t;

#endif

/** @}
 */
