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

#ifndef _GFX_PRIVATE_GLYPH_BMP_H
#define _GFX_PRIVATE_GLYPH_BMP_H

#include <gfx/coord.h>
#include <types/gfx/glyph_bmp.h>

/** Glyph bitmap
 *
 * Glyph bitmap open for editing. This is used to edit glyph bitmap.
 * Updating the entire font bitmap whenever the glyph is resized could
 * be costly. This allows to postpone the update until we are done editing.
 *
 * This is private to libgfxfont.
 */
struct gfx_glyph_bmp {
	/** Containing glyph */
	struct gfx_glyph *glyph;
	/** Rectangle covered by bitmap */
	gfx_rect_t rect;
	/** Pixel array */
	int *pixels;
};

extern void gfx_glyph_bmp_find_used_rect(gfx_glyph_bmp_t *, gfx_rect_t *);

#endif

/** @}
 */
