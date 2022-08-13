/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libgfxfont
 * @{
 */
/**
 * @file Font structure
 *
 */

#ifndef _GFX_PRIVATE_FONT_H
#define _GFX_PRIVATE_FONT_H

#include <adt/list.h>
#include <errno.h>
#include <stdint.h>
#include <types/gfx/bitmap.h>
#include <types/gfx/context.h>
#include <types/gfx/font.h>
#include <types/gfx/typeface.h>
#include <riff/chunk.h>

/** Font
 *
 * This is private to libgfxfont.
 *
 * Font bitmap contains all the glyphs packed side by side (in order of
 * @c gfx_font_t.glyphs). This is to conserve space and number of bitmaps
 * used. The baselines of the glyphs are not mutually aligned.
 * For each glyph @c gfx_glyph_t.origin designates
 * pen start point (and thus the position of the baseline).
 */
struct gfx_font {
	/** Containing typeface */
	struct gfx_typeface *typeface;
	/** Containing font info */
	struct gfx_font_info *finfo;
	/** Font metrics */
	gfx_font_metrics_t metrics;
	/** Glyphs */
	list_t glyphs;
	/** Font bitmap */
	gfx_bitmap_t *bitmap;
	/** Bitmap rectangle */
	gfx_rect_t rect;
};

/** Font info
 *
 * This is an entry in the list of fonts in typeface
 */
struct gfx_font_info {
	/** Containing typeface */
	struct gfx_typeface *typeface;
	/** Link to @c typeface->fonts */
	link_t lfonts;
	/** Font properties */
	gfx_font_props_t props;
	/** Font or @c NULL if font is not present in memory */
	struct gfx_font *font;
	/** Font chunk from which font can be read */
	riff_rchunk_t fontck;
};

extern errno_t gfx_font_splice_at_glyph(gfx_font_t *, gfx_glyph_t *,
    gfx_rect_t *);
extern errno_t gfx_font_info_load(gfx_typeface_t *, riff_rchunk_t *);
extern errno_t gfx_font_bitmap_pack(gfx_coord_t, gfx_coord_t, uint32_t *,
    void **, size_t *);
extern errno_t gfx_font_bitmap_unpack(gfx_coord_t, gfx_coord_t, void *, size_t,
    uint32_t *);
extern errno_t gfx_font_load(gfx_font_info_t *);
extern errno_t gfx_font_save(gfx_font_info_t *, riffw_t *);

#endif

/** @}
 */
