/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup nterm
 * @{
 */
/**
 * @file
 */

#ifndef FONTEDIT_H
#define FONTEDIT_H

#include <gfx/glyph.h>
#include <gfx/glyph_bmp.h>
#include <gfx/typeface.h>
#include <ui/ui.h>
#include <ui/window.h>

/** Font editor */
typedef struct {
	/** UI */
	ui_t *ui;
	/** Window */
	ui_window_t *window;
	/** Graphic context */
	gfx_context_t *gc;
	/** Window width */
	int width;
	/** Window height */
	int height;
	/** Pen color (1 = set, 0 = reset) */
	int pen_color;
	/** File name */
	const char *fname;
	/** Typeface */
	gfx_typeface_t *typeface;
	/** Font */
	gfx_font_t *font;
	/** Glyph */
	gfx_glyph_t *glyph;
	/** Glyph bitmap */
	gfx_glyph_bmp_t *gbmp;
	/** Glyph used as source for copy/paste */
	gfx_glyph_t *src_glyph;
} font_edit_t;

#endif

/** @}
 */
