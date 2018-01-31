/*
 * Copyright (c) 2012 Petr Koupy
 * Copyright (c) 2014 Martin Sucha
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

/** @addtogroup draw
 * @{
 */
/**
 * @file
 */

#include <stdint.h>
#include <errno.h>
#include <stdlib.h>

#include "../gfx/font-8x16.h"
#include "embedded.h"
#include "../drawctx.h"
#include "bitmap_backend.h"

static errno_t fde_resolve_glyph(void *unused, const wchar_t chr,
    glyph_id_t *glyph_id)
{
	bool found = false;
	uint16_t glyph = fb_font_glyph(chr, &found);
	if (!found)
		return ENOENT;
	
	*glyph_id = glyph;
	return EOK;
}

static errno_t fde_load_glyph_surface(void *unused, glyph_id_t glyph_id,
    surface_t **out_surface)
{
	surface_t *surface = surface_create(FONT_WIDTH, FONT_SCANLINES, NULL, 0);
	if (!surface)
		return ENOMEM;
	
	for (unsigned int y = 0; y < FONT_SCANLINES; ++y) {
		for (unsigned int x = 0; x < FONT_WIDTH; ++x) {
			pixel_t p = (fb_font[glyph_id][y] & (1 << (7 - x))) ? 
			    PIXEL(255, 0, 0, 0) : PIXEL(0, 0, 0, 0);
			surface_put_pixel(surface, x, y, p);
		}
	}
	
	*out_surface = surface;
	return EOK;
}

static errno_t fde_load_glyph_metrics(void *unused, glyph_id_t glyph_id,
    glyph_metrics_t *gm)
{
	/* This is simple monospaced font, so fill this data statically */
	gm->left_side_bearing = 0;
	gm->width = FONT_WIDTH;
	gm->right_side_bearing = 0;
	gm->ascender = FONT_ASCENDER;
	gm->height = FONT_SCANLINES;
	
	return EOK;
}

static void fde_release(void *data)
{
	/* no-op */
}

bitmap_font_decoder_t fd_embedded = {
	.resolve_glyph = fde_resolve_glyph,
	.load_glyph_surface = fde_load_glyph_surface,
	.load_glyph_metrics = fde_load_glyph_metrics,
	.release = fde_release
};

font_metrics_t font_metrics = {
	.ascender = FONT_ASCENDER,
	.descender = (FONT_SCANLINES - FONT_ASCENDER),
	.leading = 0
};

errno_t embedded_font_create(font_t **font, uint16_t points)
{
	return bitmap_font_create(&fd_embedded, NULL, FONT_GLYPHS, font_metrics,
	    points, font);
}

/** @}
 */
