/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
	/** Justification width (for gfx_halign_justify) */
	gfx_coord_t justify_width;
	/** Vertical alignment */
	gfx_valign_t valign;
	/** Underline */
	bool underline;
} gfx_text_fmt_t;

#endif

/** @}
 */
