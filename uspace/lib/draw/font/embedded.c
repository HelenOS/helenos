/*
 * Copyright (c) 2012 Petr Koupy
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

#include <assert.h>
#include <sys/types.h>
#include <malloc.h>

#include "../gfx/font-8x16.h"
#include "embedded.h"
#include "../drawctx.h"

static void fde_init(char *path, uint16_t *glyph_count, void **data)
{
	assert(glyph_count);
	assert(data);

	(*glyph_count) = FONT_GLYPHS;
	(*data) = NULL;
}

static uint16_t fde_resolve(const wchar_t chr, void *data)
{
	return fb_font_glyph(chr);
}

static surface_t *fde_render(uint16_t glyph, uint16_t points)
{
	surface_t *template = surface_create(FONT_WIDTH, FONT_SCANLINES, NULL, 0);
	if (!template) {
		return NULL;
	}
	for (unsigned int y = 0; y < FONT_SCANLINES; ++y) {
		for (unsigned int x = 0; x < FONT_WIDTH; ++x) {
			pixel_t p = (fb_font[glyph][y] & (1 << (7 - x))) ? 
			    PIXEL(255, 0, 0, 0) : PIXEL(0, 0, 0, 0);
			surface_put_pixel(template, x, y, p);
		}
	}

	source_t source;
	source_init(&source);
	source_set_texture(&source, template, false);

	transform_t transform;
	transform_identity(&transform);
	if (points != FONT_SCANLINES) {
		double ratio = ((double) points) / ((double) FONT_SCANLINES);
		transform_scale(&transform, ratio, ratio);
		source_set_transform(&source, transform);
	}

	double width = FONT_WIDTH;
	double height = FONT_SCANLINES;
	transform_apply_linear(&transform, &width, &height);
	surface_t *result =
	    surface_create((sysarg_t) (width + 0.5), (sysarg_t) (height + 0.5), NULL, 0);
	if (!result) {
		surface_destroy(template);
		return NULL;
	}

	drawctx_t context;
	drawctx_init(&context, result);
	drawctx_set_source(&context, &source);
	drawctx_transfer(&context, 0, 0,
	    (sysarg_t) (width + 0.5), (sysarg_t) (height + 0.5));

	surface_destroy(template);

	return result;
}

static void fde_release(void *data)
{
	/* no-op */
}

font_decoder_t fd_embedded = {
	.init = fde_init,
	.resolve = fde_resolve,
	.render = fde_render,
	.release = fde_release
};

/** @}
 */
