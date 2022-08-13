/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
