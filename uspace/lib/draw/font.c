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
#include <malloc.h>

#include "font.h"
#include "font/embedded.h"
#include "drawctx.h"

void font_init(font_t *font, font_decoder_type_t decoder, char *path, uint16_t points)
{
	font->points = points;

	switch (decoder) {
	case FONT_DECODER_EMBEDDED:
		font->decoder = &fd_embedded;
		break;
	default:
		font->decoder = NULL;
		break;
	}

	if (font->decoder) {
		font->decoder->init(path, &font->glyph_count, &font->decoder_data);

		if (font->glyph_count > 0) {
			font->glyphs = (surface_t **) malloc(sizeof(surface_t *) * font->glyph_count);
		} else {
			font->glyphs = NULL;
		}

		if (font->glyphs) {
			for (size_t i = 0; i < font->glyph_count; ++i) {
				font->glyphs[i] = NULL;
			}
		} else {
			font->glyph_count = 0;
		}
	} else {
		font->glyph_count = 0;
		font->glyphs = NULL;
		font->decoder_data = NULL;
	}
}

void font_release(font_t *font)
{
	if (font->glyphs) {
		for (size_t i = 0; i < font->glyph_count; ++i) {
			if (font->glyphs[i]) {
				surface_destroy(font->glyphs[i]);
			}
		}
		free(font->glyphs);
	}
	
	if (font->decoder) {
		font->decoder->release(font->decoder_data);
	}
}

void font_get_box(font_t *font, char *text, sysarg_t *width, sysarg_t *height)
{
	assert(width);
	assert(height);

	(*width) = 0;
	(*height) = 0;

	if (!text) {
		return;
	}

	while (*text) {
		uint16_t glyph_idx = font->decoder->resolve(*text, font->decoder_data);
		if (glyph_idx < font->glyph_count) {
			if (!font->glyphs[glyph_idx]) {
				font->glyphs[glyph_idx] =
				    font->decoder->render(glyph_idx, font->points);
			}

			surface_t *glyph = font->glyphs[glyph_idx];
			if (glyph) {
				sysarg_t w;
				sysarg_t h;
				surface_get_resolution(glyph, &w, &h);
				(*width) += w;
				(*height) = (*height) < h ? h : (*height);
			}
		}
		++text;
	}
}

void font_draw_text(font_t *font, drawctx_t *context, source_t *source,
    const char *text, sysarg_t x, sysarg_t y)
{
	assert(context);
	assert(source);

	drawctx_save(context);
	drawctx_set_compose(context, compose_over);

	while (*text) {
		uint16_t glyph_idx = font->decoder->resolve(*text, font->decoder_data);
		if (glyph_idx < font->glyph_count) {
			if (!font->glyphs[glyph_idx]) {
				font->glyphs[glyph_idx] =
				    font->decoder->render(glyph_idx, font->points);
			}

			surface_t *glyph = font->glyphs[glyph_idx];
			if (glyph) {
				sysarg_t w;
				sysarg_t h;
				surface_get_resolution(glyph, &w, &h);

				transform_t transform;
				transform_identity(&transform);
				transform_translate(&transform, x, y);
				source_set_transform(source, transform);
				source_set_mask(source, glyph, false);
				drawctx_transfer(context, x, y, w, h);

				x += w;
			}
		}
		++text;
	}

	drawctx_restore(context);
	source_set_mask(source, NULL, false);
}

/** @}
 */
