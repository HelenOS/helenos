/*
 * Copyright (c) 2020 Jiri Svoboda
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
