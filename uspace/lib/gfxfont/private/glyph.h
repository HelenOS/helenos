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

/** @addtogroup libgfxfont
 * @{
 */
/**
 * @file Glyph structure
 *
 */

#ifndef _GFX_PRIVATE_GLYPH_H
#define _GFX_PRIVATE_GLYPH_H

#include <adt/list.h>
#include <gfx/coord.h>
#include <types/gfx/glyph.h>
#include <riff/chunk.h>

/** Glyph
 *
 * This is private to libgfxfont.
 */
struct gfx_glyph {
	/** Containing font */
	struct gfx_font *font;
	/** Link to list of glyphs in the font */
	link_t lglyphs;
	/** Glyph metrics */
	gfx_glyph_metrics_t metrics;
	/** Text patterns (of gfx_glyph_pattern_t) */
	list_t patterns;
	/** Rectangle within font bitmap containing the glyph */
	gfx_rect_t rect;
	/** Glyph origin within font bitmap (pen start point) */
	gfx_coord2_t origin;
};

/** Glyph pattern.
 *
 * Glyph is set if pattern is found in text.
 */
struct gfx_glyph_pattern {
	/** Containing glyph */
	struct gfx_glyph *glyph;
	/** Link to gfx_glyph.patterns */
	link_t lpatterns;
	/** Pattern text */
	char *text;
};

extern errno_t gfx_glyph_transfer(gfx_glyph_t *, gfx_coord_t, gfx_bitmap_t *,
    gfx_rect_t *);
extern errno_t gfx_glyph_load(gfx_font_t *, riff_rchunk_t *);
extern errno_t gfx_glyph_save(gfx_glyph_t *, riffw_t *);

#endif

/** @}
 */
